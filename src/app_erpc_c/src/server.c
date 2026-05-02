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
static void Errors_FreeMemoryIfMallocFailed(list_Erpc_ErrorInfo_t_1_t *out_errors);
static Erpc_Status_t Errors_GetAdditionalDataForAllErrors(list_Erpc_ErrorInfo_t_1_t *out_errors);

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

Erpc_Status_t Errors_GetAll(list_Erpc_ErrorInfo_t_1_t * out_errors)
{
    Erpc_Status_t operation_status = ERPC_OK;
    if (out_errors)
    {
        out_errors->elementsCount = Det_GetErrorCount();
        if (0 < out_errors->elementsCount)
        {
            out_errors->elements = (Erpc_ErrorInfo_t *)erpc_malloc(out_errors->elementsCount * sizeof(Erpc_ErrorInfo_t));
            if (out_errors->elements)
            {
                Errors_GetAdditionalDataForAllErrors(out_errors);
            }
            else
            {
                operation_status = ERPC_NOT_OK;
            }
        }
    }
    else
    {
        operation_status = ERPC_NOT_OK;
    }

    if (ERPC_NOT_OK == operation_status)
    {
        Errors_FreeMemoryIfMallocFailed(out_errors);
    }

    return operation_status;
}

Erpc_Status_t Errors_ClearAll(void)
{
    Det_ClearErrors();
    return ERPC_OK;
}

static void Errors_FreeMemoryIfMallocFailed(list_Erpc_ErrorInfo_t_1_t *out_errors)
{
    if (out_errors)
    {
        if (out_errors->elements)
        {
            for (uint32_t i = 0; i < out_errors->elementsCount; ++i)
            {
                if (out_errors->elements[i].additional_data.elements)
                {
                    erpc_free(out_errors->elements[i].additional_data.elements);
                }
            }
            erpc_free(out_errors->elements);
        }
        erpc_free(out_errors);
    }
    Det_Warning(DET_MALLOC_FAILED_ERPC_SERVER, DET_MULTIPLE_TIME_REPORT_ERROR);
}

static Erpc_Status_t Errors_GetAdditionalDataForAllErrors(list_Erpc_ErrorInfo_t_1_t *out_errors)
{
    Erpc_Status_t operation_status = ERPC_OK;
    for (uint32_t i = 0; i < out_errors->elementsCount; ++i)
    {
        uint8_t *additional_data_ptr;
        uint32_t additional_data_length;
        out_errors->elements[i].error_id = Det_GetErrorId(i);
        additional_data_ptr = Det_GetErrorAdditionaldataPtr(i, &additional_data_length);
        out_errors->elements[i].additional_data.elementsCount = additional_data_length;
        if (0 < additional_data_length)
        {
            out_errors->elements[i].additional_data.elements = (uint8_t *)erpc_malloc(additional_data_length * sizeof(uint8_t));
            if (out_errors->elements[i].additional_data.elements)
            {
                memcpy(out_errors->elements[i].additional_data.elements, additional_data_ptr, additional_data_length);
            }
            else
            {
                operation_status = ERPC_NOT_OK;
                break;
            }
        }
    }
    return operation_status;
}

Erpc_SwVersion_t ErpcVersion_Get(void)
{
    return ERPC_INTERFACE_VERSION;
}
