#include "FreeRTOS.h"
#include "task.h"
#include "erpc_port.h"
#include "c_example_server.h"
#include "erpc_common.h"
#include "transport_lwip_tcp.h"
#include "erpc_server_setup.h"
#include "lwip.h"
#include "app.h"
#include "det.h"
#include "std_utils.h"
#include <string.h>

#define SERVER_STACK_SIZE (512)
#define SERVER_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

#define SERVER_PORT (50000U)
static void Server_Task(void *pvParameters);
static void Errors_GetAllFreeMemoryIfMallocFailed(list_Erpc_ErrorInfo_t_1_t *error_list);
static Std_ReturnType Errors_GetAllProcessAllErrors(list_Erpc_ErrorInfo_t_1_t *error_list);

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
            uint8_t error_data[sizeof(status) + APP_VERSION_SIZEOF];
            StdUtils_Uint16ToBuffer(error_data, APP_VERSION);
            *(error_data + APP_VERSION_SIZEOF) = status;
            Det_WarningWithData(DET_SERVER_FAILED_TO_POLL, DET_MULTIPLE_TIME_REPORT_ERROR, error_data, sizeof(error_data));
        }
    }
}

Erpc_Status_t Unused_TestRequest(uint8_t test_variable)
{
    Lwip_SendUdp("Received test variable\n", sizeof("Received test variable\n"));
    return ERPC_OK;
}

uint16_t AppVersion_Get(void)
{
    return APP_VERSION;
}

list_Erpc_ErrorInfo_t_1_t * Errors_GetAll(void)
{
    Std_ReturnType operation_status = E_OK;
    list_Erpc_ErrorInfo_t_1_t *error_list = (list_Erpc_ErrorInfo_t_1_t *)erpc_malloc(sizeof(list_Erpc_ErrorInfo_t_1_t));
    if (error_list)
    {
        error_list->elementsCount = Det_GetErrorCount();
        if (0 < error_list->elementsCount)
        {
            error_list->elements = (Erpc_ErrorInfo_t *)erpc_malloc(error_list->elementsCount * sizeof(Erpc_ErrorInfo_t));
            if (error_list->elements)
            {
                Errors_GetAllProcessAllErrors(error_list);
            }
            else
            {
                operation_status = E_NOT_OK;
            }
        }
    }
    else
    {
        operation_status = E_NOT_OK;
    }

    if (E_NOT_OK == operation_status)
    {
        Errors_GetAllFreeMemoryIfMallocFailed(error_list);
    }
    /* TODO check out why server is hanging if NULL is returned. Maybe add reinitialization of the server? */
    return error_list;
}

Erpc_Status_t Errors_ClearAll(void)
{
    Det_ClearErrors();
    return ERPC_OK;
}

static void Errors_GetAllFreeMemoryIfMallocFailed(list_Erpc_ErrorInfo_t_1_t *error_list)
{
    if (error_list)
    {
        if (error_list->elements)
        {
            for (uint32_t i = 0; i < error_list->elementsCount; ++i)
            {
                if (error_list->elements[i].additional_data.elements)
                {
                    erpc_free(error_list->elements[i].additional_data.elements);
                    error_list->elements[i].additional_data.elements = NULL;
                }
            }
            erpc_free(error_list->elements);
            error_list->elements = NULL;
        }
        erpc_free(error_list);
        error_list = NULL;
    }
    Det_Warning(DET_MALLOC_FAILED_ERPC_SERVER, DET_MULTIPLE_TIME_REPORT_ERROR);
}

static Std_ReturnType Errors_GetAllProcessAllErrors(list_Erpc_ErrorInfo_t_1_t *error_list)
{
    Std_ReturnType operation_status = E_OK;
    for (uint32_t i = 0; i < error_list->elementsCount; ++i)
    {
        uint8_t *additional_data_ptr;
        uint32_t additional_data_length;
        error_list->elements[i].error_id = Det_GetErrorId(i);
        additional_data_ptr = Det_GetErrorAdditionaldataPtr(i, &additional_data_length);
        error_list->elements[i].additional_data.elementsCount = additional_data_length;
        if (0 < additional_data_length)
        {
            error_list->elements[i].additional_data.elements = (uint8_t *)erpc_malloc(additional_data_length * sizeof(uint8_t));
            if (error_list->elements[i].additional_data.elements)
            {
                memcpy(error_list->elements[i].additional_data.elements, additional_data_ptr, additional_data_length);
            }
            else
            {
                operation_status = E_NOT_OK;
                break;
            }
        }
    }
    return operation_status;
}