#include "erpc_transport.hpp"
#include "erpc_common.h"
#include "erpc_message_buffer.hpp"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include <cstring>
#include "std_utils.h"

extern "C" {
#include "transport_lwip_tcp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "det.h"
}

static constexpr uint32_t ERPC_HEADER_SIZE = 6;
static constexpr uint32_t ERPC_HEADER_CRC_INDEX = 0;
static constexpr uint32_t ERPC_HEADER_CRC_SIZE = 2;
static constexpr uint32_t ERPC_PAYLOAD_LENGTH_INDEX = 2;
static constexpr uint32_t ERPC_PAYLOAD_LENGTH_SIZE = 2;
static constexpr uint32_t ERPC_PAYLOAD_CRC_INDEX = 4;
static constexpr uint32_t ERPC_PAYLOAD_CRC_SIZE = 2;
static constexpr uint32_t ERPC_MAX_RX_MSG = 0x500u;
static constexpr uint32_t ERPC_MAX_TX_MSG = 0x500u;

using namespace erpc;

class TransportLwipTcp: public Transport
{
public:
    ~TransportLwipTcp()
    {
        if (m_clientPcb)
        {
            tcp_close(m_clientPcb);
        }

        if (m_listenPcb)
        {
            tcp_close(m_listenPcb);
        }
    }

    void init(uint16_t port)
    {
        m_listenPcb = tcp_new();
        if ((m_listenPcb != nullptr) && (tcp_bind(m_listenPcb, IP_ADDR_ANY, port) == ERR_OK))
        {
            m_listenPcb = tcp_listen(m_listenPcb);
            tcp_accept(m_listenPcb, acceptTcpCallbackStatic);
            m_listenPcb->callback_arg = this;
        }
    }

    erpc_status_t send(MessageBuffer* message)
    {
        if (!m_clientPcb || !message)
        {
            return kErpcStatus_Fail;
        }

        uint16_t payload_size = message->getUsed();

        erpc_status_t status = kErpcStatus_Success;

        if (ERPC_MAX_TX_MSG < payload_size + ERPC_HEADER_SIZE)
        {
            status = kErpcStatus_BufferOverrun;
        }
        else
        {
            uint8_t *payload = m_txBuffer + ERPC_HEADER_SIZE;

            message->read(0, payload, payload_size);
    
            uint16_t payload_crc = m_crc16.computeCRC16(payload, payload_size);
            StdUtils_Uint16ToBuffer(m_txBuffer + ERPC_PAYLOAD_LENGTH_INDEX, payload_size);
            StdUtils_Uint16ToBuffer(m_txBuffer + ERPC_PAYLOAD_CRC_INDEX, payload_crc);
    
            uint16_t header_crc = calculateHeaderCrc16(m_txBuffer);
            StdUtils_Uint16ToBuffer(m_txBuffer + ERPC_HEADER_CRC_INDEX, header_crc);
    
            if (tcp_write(m_clientPcb, m_txBuffer, payload_size + ERPC_HEADER_SIZE, TCP_WRITE_FLAG_COPY) != ERR_OK)
            {
                status = kErpcStatus_Fail;
            }
            else
            {
                if (tcp_output(m_clientPcb) != ERR_OK)
                {
                    status = kErpcStatus_Fail;
                }
            }
        }

        return status;
    }

    erpc_status_t receive(MessageBuffer* message)
    {
        if (!message)
        {
            return kErpcStatus_InvalidArgument;
        }

        /* Wait for reception of message */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint16_t payload_size = StdUtils_BufferToUint16(m_rxBuffer + ERPC_PAYLOAD_LENGTH_INDEX);
        erpc_status_t status;

        if (message->getFree() < payload_size)
        {
            status = kErpcStatus_BufferOverrun;
        }
        else
        {
            status = message->write(0, m_rxBuffer + ERPC_HEADER_SIZE, payload_size);
            if (status == kErpcStatus_Success)
            {
                message->setUsed(payload_size);
            }
        }
        return status;
    }

    void setTaskToNotify(TaskHandle_t taskToNotify)
    {
        m_TaskToNotify = taskToNotify;
    }

private:
    struct tcp_pcb* m_listenPcb {nullptr};
    struct tcp_pcb* m_clientPcb {nullptr};
    uint8_t m_rxBuffer[ERPC_MAX_RX_MSG];
    uint8_t m_txBuffer[ERPC_MAX_TX_MSG];
    uint32_t m_rxLen {0};
    Crc16 m_crc16 {};
    TaskHandle_t m_TaskToNotify {nullptr};

    typedef enum CompleteMessageCheck
    {
        kCompleteMessageCheck_Incomplete,
        kCompleteMessageCheck_Complete,
        kCompleteMessageCheck_CrcFailed
    } CompleteMessageCheck;

    uint16_t calculateHeaderCrc16(uint8_t* data)
    {
        return m_crc16.computeCRC16(data + ERPC_PAYLOAD_LENGTH_INDEX, ERPC_PAYLOAD_LENGTH_SIZE) +
               m_crc16.computeCRC16(data + ERPC_PAYLOAD_CRC_INDEX, ERPC_PAYLOAD_CRC_SIZE);
    }

    static err_t acceptTcpCallbackStatic(void* arg, struct tcp_pcb* newpcb, err_t err)
    {
        TransportLwipTcp* self = static_cast<TransportLwipTcp *>(arg);
        return self->acceptTcpCallback(newpcb, err);
    }

    err_t acceptTcpCallback(struct tcp_pcb* newpcb, err_t err)
    {
        if (!newpcb || (err != ERR_OK))
        {
            return ERR_VAL;
        }

        m_clientPcb = newpcb;
        tcp_recv(m_clientPcb, receiveTcpCallbackStatic);
        m_clientPcb->callback_arg = this;

        return ERR_OK;
    }

    static err_t receiveTcpCallbackStatic(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err)
    {
        TransportLwipTcp* self = static_cast<TransportLwipTcp *>(arg);
        return self->receiveTcpCallback(tpcb, p, err);
    }

    err_t receiveTcpCallback(struct tcp_pcb* tpcb, struct pbuf* p, err_t err)
    {
        if (!p)
        {
            return ERR_VAL;
        }

        pbuf_copy_partial(p, m_rxBuffer + m_rxLen, p->tot_len, 0);

        m_rxLen += p->tot_len;

        CompleteMessageCheck message_completion = verifyIfMessageIsComplete();

        if (kCompleteMessageCheck_Complete == message_completion)
        {
            m_rxLen = 0;
            xTaskNotifyGive(m_TaskToNotify);
        }
        else if (kCompleteMessageCheck_CrcFailed == message_completion)
        {
            /* There is issue with CRC, drop the message to make room for new message */
            Det_Error(DET_TRANSPORT_LWIP_TCP_CRC_ERROR, DET_MULTIPLE_TIME_REPORT_ERROR);
            m_rxLen = 0;
        }
        else
        {
            /* Message is incomplete, wait for next part of message */
        }

        tcp_recved(tpcb, p->tot_len);
        pbuf_free(p);

        return ERR_OK;
    }

    CompleteMessageCheck verifyIfMessageIsComplete()
    {
        CompleteMessageCheck message_complete = kCompleteMessageCheck_Complete;
        /* Verify header CRC */
        uint16_t computed_crc = calculateHeaderCrc16(m_rxBuffer);
        uint16_t received_crc = StdUtils_BufferToUint16(m_rxBuffer + ERPC_HEADER_CRC_INDEX);
        if (computed_crc != received_crc)
        {
            message_complete = kCompleteMessageCheck_CrcFailed;
        }
        else
        {
            uint16_t payload_size = StdUtils_BufferToUint16(m_rxBuffer + ERPC_PAYLOAD_LENGTH_INDEX);
    
            if (m_rxLen < payload_size + ERPC_HEADER_SIZE)
            {
                message_complete = kCompleteMessageCheck_Incomplete;
            }

            /* Verify payload CRC */
            computed_crc = m_crc16.computeCRC16(m_rxBuffer + ERPC_HEADER_SIZE, payload_size);
            received_crc = StdUtils_BufferToUint16(m_rxBuffer + ERPC_PAYLOAD_CRC_INDEX);
            if (computed_crc != received_crc)
            {
                message_complete = kCompleteMessageCheck_CrcFailed;
            }
        }
        return message_complete;
    }

};

static TransportLwipTcp *TransportLwipTcp_Struct = new TransportLwipTcp();

extern "C" {

void * TransportLwipTcp_Init(uint16_t port, TaskHandle_t task_to_notify)
{ 
    TransportLwipTcp_Struct->init(port);
    TransportLwipTcp_Struct->setTaskToNotify(task_to_notify);

    return static_cast<void *>(TransportLwipTcp_Struct);
}

}
