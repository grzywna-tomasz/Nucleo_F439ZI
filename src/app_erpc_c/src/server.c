#include "FreeRTOS.h"
#include "task.h"
#include "erpc_port.h"
#include "c_example_server.h"
#include "erpc_common.h"
#include "transport_lwip_tcp.h"
#include "erpc_server_setup.h"
#include "lwip.h"

#define SERVER_STACK_SIZE (512)
#define SERVER_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

#define SERVER_PORT (50000U)
static void Server_Task(void *pvParameters);

static StackType_t Server_Stack[SERVER_STACK_SIZE];
static StaticTask_t Server_TaskBuffer;
static TaskHandle_t Server_Handle = NULL;

void Server_Init(void)
{
    Server_Handle = xTaskCreateStatic(Server_Task, "ServerTask", SERVER_STACK_SIZE, NULL, SERVER_TASK_PRIORITY, Server_Stack, &Server_TaskBuffer);
}

static void Server_Task(void *pvParameters)
{
    erpc_status_t status;
    erpc_mbf_t message_buffer_factory = erpc_mbf_dynamic_init();
    erpc_transport_t transport_pointer = (erpc_transport_t)TransportLwipTcp_Init(SERVER_PORT, Server_Handle);
    erpc_service_t service = create_Communication_service();
    erpc_server_t server = erpc_server_init(transport_pointer, message_buffer_factory);
    erpc_add_service_to_server(server, service);

    while(1)
    {
        status = erpc_server_poll(server);
        if (status != kErpcStatus_Success)
        {
            // TODOTODO - handle error with DET
        }
    }
}

status SendRequest(uint8_t test_variable)
{
    Lwip_SendUdp("Received test variable\n", sizeof("Received test variable\n"));
    return ERPC_OK;
}