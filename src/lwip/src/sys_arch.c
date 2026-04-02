#include "arch/cc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "lwip/sys.h"
#include "lwip/err.h"
#include "arch/sys_arch.h"

void sys_init(void)
{

}

sys_thread_t sys_thread_new(const char* name, lwip_thread_fn function, void* arg, int stacksize, int prio)
{
    sys_thread_t ret = NULL;

    UBaseType_t freertos_stack_words = (UBaseType_t)(stacksize / sizeof(StackType_t));

    if (pdPASS != xTaskCreate((TaskFunction_t)function, name, freertos_stack_words, arg, (UBaseType_t)prio, &ret))
    {
        ret = NULL;
    }

    return ret;
}

// u32_t sys_now(void)
// {
//     return (u32_t)(xTaskGetTickCount() * 1000U / configTICK_RATE_HZ);
// }

void sys_msleep(u32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

sys_prot_t sys_arch_protect(void)
{
    taskENTER_CRITICAL();
    return 0;
}

void sys_arch_unprotect(sys_prot_t lev)
{
    LWIP_UNUSED_ARG(lev);
    taskEXIT_CRITICAL();
}

u32_t sys_arch_mbox_fetch(sys_mbox_t* mbox, void** msg, u32_t timeout)
{
    u32_t ret = SYS_ARCH_TIMEOUT;
    TickType_t start = xTaskGetTickCount();
    TickType_t ticks = (timeout == 0U) ? portMAX_DELAY : pdMS_TO_TICKS(timeout);

    if (pdPASS == xQueueReceive(*mbox, msg, ticks))
    {
        TickType_t end = xTaskGetTickCount();
        ret = (u32_t)((end - start) * 1000U / configTICK_RATE_HZ);
    }
    else
    {
        if (NULL != msg)
        {
            *msg = NULL;
        }
    }

    return ret;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t* mbox, void** msg)
{
    u32_t ret = SYS_MBOX_EMPTY;

    if (pdPASS == xQueueReceive(*mbox, msg, 0U))
    {
        ret = 0U;
    }

    return ret;
}

u32_t sys_arch_sem_wait(sys_sem_t* sem, u32_t timeout)
{
    u32_t ret = SYS_ARCH_TIMEOUT;
    TickType_t start = xTaskGetTickCount();
    TickType_t ticks = (timeout == 0U) ? portMAX_DELAY : pdMS_TO_TICKS(timeout);

    if (pdTRUE == xSemaphoreTake(*sem, ticks))
    {
        TickType_t end = xTaskGetTickCount();
        ret = (u32_t)((end - start) * 1000U / configTICK_RATE_HZ);
    }
    return ret;
}

void sys_mbox_free(sys_mbox_t* mbox)
{
    vQueueDelete(*mbox);
}

err_t sys_mbox_trypost(sys_mbox_t* mbox, void* msg)
{
    return (pdPASS == xQueueSend(*mbox, &msg, 0)) ? ERR_OK : ERR_MEM;
}

err_t sys_mbox_new(sys_mbox_t* mbox, int size)
{
    err_t ret = ERR_MEM;

    if (size <= 0)
    {
        size = TCPIP_MBOX_SIZE;
    }

    *mbox = xQueueCreate((UBaseType_t)size, (UBaseType_t)sizeof(void*));

    if (*mbox != NULL)
    {
        ret = ERR_OK;
    }

    return ret;
}

int sys_mbox_valid(sys_mbox_t* mbox)
{
    return *mbox != NULL;
}

void sys_mbox_set_invalid(sys_mbox_t* mbox)
{
    *mbox = NULL;
}

void sys_sem_signal(sys_sem_t* sem)
{
    xSemaphoreGive(*sem);
}

err_t sys_sem_new(sys_sem_t* sem, u8_t count)
{
    err_t ret = ERR_MEM;

    *sem = xSemaphoreCreateBinary();

    if (NULL != *sem)
    {
        if (count != 0U)
        {
            xSemaphoreGive(*sem);
        }

        ret = ERR_OK;
    }

    return ret;
}

int sys_sem_valid(sys_sem_t* sem)
{
    return *sem != NULL;
}

void sys_sem_set_invalid(sys_sem_t* sem)
{
    *sem = NULL;
}

void sys_sem_free(sys_sem_t* sem)
{
    vSemaphoreDelete(*sem);
}

void sys_mutex_unlock(sys_mutex_t* mutex)
{
    xSemaphoreGive(*mutex);
}

void sys_mutex_lock(sys_mutex_t* mutex)
{
    xSemaphoreTake(*mutex, portMAX_DELAY);
}

err_t sys_mutex_new(sys_mutex_t* mutex)
{
    *mutex = xSemaphoreCreateMutex();
    return (*mutex != NULL) ? ERR_OK : ERR_MEM;
}
