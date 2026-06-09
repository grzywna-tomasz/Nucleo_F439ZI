#pragma once

#include <vector>

#ifdef __cplusplus
extern "C" {
#endif
#include "std_types.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32f4xx_hal.h"
#ifdef __cplusplus
}
#endif

constexpr uint8_t CAN_DATA_SIZE {8U};

typedef struct
{
    uint16_t id;
    uint8_t data[CAN_DATA_SIZE];
    uint8_t data_len;
} CanData_t;

/* Forward declaration needed by ICanDriverInstance */
class ICanListener;

class ICanDriverInstance {
public:
    ICanDriverInstance(uint8_t max_listeners, CAN_HandleTypeDef& hcan, uint8_t* tx_queue_storage_area, uint32_t tx_queue_size);
    Std_ReturnType init();
    Std_ReturnType addListener(ICanListener *listener);
    void rxMsgDispatcher(CAN_HandleTypeDef *hcan);
    Std_ReturnType queueMessage(CanData_t& data);
    void sendFromQueue(void);
    const CAN_HandleTypeDef& getHalHandle() const;
    uint8_t getUniqueId() const;
private:
    uint8_t m_UniqueId;
    std::vector<ICanListener *> m_listeners;
    static uint8_t m_CurrentId;

    StaticQueue_t m_TxQueue;
    QueueHandle_t m_TxQueueHandle;
    uint8_t* const m_TxQueueStorageArea;
    const uint32_t m_TxQueueStorageAreaSize;
    CAN_HandleTypeDef& m_hcan;
};

/* TxQueueSize - numer of elements that can be stored in queue */
template <uint16_t TxQueueSize>
class CanDriverInstance: public ICanDriverInstance {
public:
    CanDriverInstance(uint8_t max_listeners, CAN_HandleTypeDef& hcan);
private:
    uint8_t m_TxQueueStorageArea[TxQueueSize * sizeof(CanData_t)];
};

class ICanListener {
public:
    ICanListener(uint8_t* rx_queue_storage_area, uint32_t rx_queue_size);
    Std_ReturnType init(ICanDriverInstance* driver_instance);
    /* Used to verify if message to be consumed by this listener */
    virtual bool msgForThisListener(uint16_t id) const;
    /* Provide container for message to be received. Timeout in ms after which message drop */
    Std_ReturnType waitForMsg(CanData_t& msg, uint32_t timeout=portMAX_DELAY);
    /* Interface used for accessing RX queue handle */
    QueueHandle_t getCanQueueHandle() const;
    /* Just for identification and logging purpose */
    uint8_t getUniqueId() const;
    /* Send message to assigned driver */
    Std_ReturnType sendMessage(CanData_t& data);
private:
    static uint8_t m_UniqueId;
    ICanDriverInstance* m_assignedDriver; 

    StaticQueue_t m_RxQueue;
    uint8_t* m_RxQueueStorageArea;
    const uint32_t m_RxQueueStorageAreaSize;
    QueueHandle_t m_RxQueueHandle;
};

/* RxQueueSize - numer of elements that can be stored in queue */
template <uint16_t RxQueueSize>
class CanListener : public ICanListener {
public:
    CanListener();
private:
    uint8_t m_RxQueueStorageArea[RxQueueSize * sizeof(CanData_t)];
};

template <uint16_t RxQueueSize>
class CanListenerMinMax : public CanListener<RxQueueSize> {
public:
    CanListenerMinMax(uint16_t min, uint16_t max)
    : m_minId {min}, m_maxId {max}
    {}
    bool msgForThisListener(uint16_t id) const override;
private:
    uint16_t m_minId;
    uint16_t m_maxId;
};

#include "can.tpp"
