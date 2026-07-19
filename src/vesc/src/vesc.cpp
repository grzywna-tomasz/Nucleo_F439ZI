#include "std_utils.hpp"
#include "vesc.hpp"

extern "C" {
    #include "FreeRTOS.h"
    #include "task.h"
    #include "timers.h"
    #include "c_example_server.h"
}

static constexpr uint32_t VESC_STATUS_MSG_MASK {0xFF00};

/* TODO - where to put it so it will be properly? Some header */
extern CanDriverInstance<5> CanDriver1;
CanListener<8> Vesc_CanListener;
Vesc Vesc_Controller1(32, 1);
std::vector<Vesc*> controllersVector {&Vesc_Controller1};

static constexpr uint32_t VESC_STACK_SIZE {256U};
static constexpr uint32_t VESC_TASK_PRIORITY {tskIDLE_PRIORITY + 1};
StackType_t Vesc_Stack[VESC_STACK_SIZE];
StaticTask_t Vesc_TaskBuffer;
TaskHandle_t Vesc_TaskHandle = NULL;
TimerHandle_t Vesc_TimerHandle;
StaticTimer_t Vesc_TimerBuffer;

void Vesc_Status1::status1Decode(uint8_t* data, uint8_t data_len)
{
    if ((nullptr == data) || (8 != data_len))
    {
        /* TODO DET */
        return;
    }
    else
    {
        uint8_t* local_ptr = data;
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_ERPM);
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_current);
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_dutyCycle);
    }
}

void Vesc_Status2::status2Decode(uint8_t* data, uint8_t data_len)
{
    if ((nullptr == data) || (8 != data_len))
    {
        /* TODO DET */
        return;
    }
    else
    {
        uint8_t* local_ptr = data;
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_AmpHours);
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_AmpHoursCharged);
    }
}

void Vesc_Status3::status3Decode(uint8_t* data, uint8_t data_len)
{
    if ((nullptr == data) || (8 != data_len))
    {
        /* TODO DET */
        return;
    }
    else
    {
        uint8_t* local_ptr = data;
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_WattHours);
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_WatHoursCharged);

    }
}

void Vesc_Status4::status4Decode(uint8_t* data, uint8_t data_len)
{
    if ((nullptr == data) || (8 != data_len))
    {
        /* TODO DET */
        return;
    }
    else
    {
        uint8_t* local_ptr = data;
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_FETTemp);
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_motorTemp);
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_inputCurrent);
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_PIDPosition);

    }
}

void Vesc_Status5::status5Decode(uint8_t* data, uint8_t data_len)
{
    if ((nullptr == data) || (8 != data_len))
    {
        /* TODO DET */
        return;
    }
    else
    {
        uint8_t* local_ptr = data;
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_tachometerValue);
        StdUtils_BufferToValueDifferenEndian(local_ptr, m_voltageInput);
    }
}

void VescMotorController::setERPM(int32_t erpm_speed)
{
    m_periodicMode = ERPM_MODE;
    m_periodicValue = erpm_speed;
    xTimerReset(Vesc_TimerHandle, 0);
}

void VescMotorController::setCurrent(int32_t current)
{
    m_periodicMode = CURRENT_MODE;
    m_periodicValue = current;
    xTimerReset(Vesc_TimerHandle, 0);
}

Std_ReturnType VescMotorController::send4Bytes(int32_t value, uint16_t message_id)
{
    CanData_t msg;
    msg.id = message_id + m_controllerId;
    msg.data_len = 4;
    uint8_t *local_data_ptr = msg.data;
    StdUtils_ValueToBufferDifferenEndian(local_data_ptr, value);
    return Vesc_CanListener.sendMessage(msg);
}

void VescMotorController::sendPeriodic(void)
{
    send4Bytes(m_periodicValue, m_periodicMode);
}
  
void Vesc::decodeStatus(CanData_t& msg)
{
    switch (msg.id & VESC_STATUS_MSG_MASK)
    {
        case 0x900:
            status1Decode(msg.data, msg.data_len);
            break;
        case 0xE00:
            status2Decode(msg.data, msg.data_len);
            break;
        case 0xF00:
            status3Decode(msg.data, msg.data_len);
            break;
        case 0x1000:
            status4Decode(msg.data, msg.data_len);
            break;
        case 0x1B00:
            status5Decode(msg.data, msg.data_len);
            break;
    }
}

int32_t Vesc::getSpeed()
{
    return getErpm() * m_motorPoles;
}

void Vesc::setSpeed(int32_t speed_rpm)
{
    setERPM(speed_rpm * m_motorPoles);
}

void Vesc_Task(void *pvParams)
{
    while(1)
    {
        CanData_t msg;
        Vesc_CanListener.waitForMsg(msg);
        Vesc_Controller1.decodeStatus(msg);
    }
}

void Vesc_TimerCallback(TimerHandle_t xTimer)
{
    Vesc_Controller1.sendPeriodic();
}

Std_ReturnType Vesc_Init(void)
{
    Std_ReturnType ret_val {E_NOT_OK};
    
    Vesc_TaskHandle = xTaskCreateStatic(Vesc_Task, "VescTask", VESC_STACK_SIZE, (void *) 0, VESC_TASK_PRIORITY, Vesc_Stack, &Vesc_TaskBuffer);
    Vesc_TimerHandle = xTimerCreateStatic("VescSend", pdMS_TO_TICKS(50), pdTRUE, 0, Vesc_TimerCallback, &Vesc_TimerBuffer);

    if ((Vesc_TaskHandle) && (Vesc_TimerHandle))
    {
        ret_val = Vesc_CanListener.init(&CanDriver1);
    }

    return ret_val;
}

extern "C" 
{
Erpc_Status_t Vesc_SetSpeed(uint32_t rpm, uint8_t controller_id)
{
    Vesc_Controller1.setSpeed(rpm);
    return ERPC_OK;
}

int32_t Vesc_GetSpeedRPM(uint8_t controller_id)
{
    return Vesc_Controller1.getSpeed();
}

int16_t Vesc_GetInputCurrent(uint8_t controller_id)
{
    return Vesc_Controller1.getInputCurrent();
}

int16_t Vesc_GetInputVoltage(uint8_t controller_id)
{
    return Vesc_Controller1.getInputVoltage();
}

int16_t Vesc_GetMotorCurrent(uint8_t controller_id)
{
    return Vesc_Controller1.getCurrent();
}

int16_t Vesc_GetFETTemp(uint8_t controller_id)
{
    return Vesc_Controller1.getFETTemp();
}

int16_t Vesc_GetMotorCurrentPositionDeg(uint8_t controller_id)
{
    return Vesc_Controller1.getPIDPosition() * 2 / 100;
}

int16_t Vesc_GetCurrentDutyCycle(uint8_t controller_id)
{
    return Vesc_Controller1.getDutyCycle();
}

Erpc_Status_t Vesc_EnableMotorFreespin(uint8_t controller_id)
{
    Vesc_Controller1.setCurrent(0);
    return ERPC_OK;
}

Erpc_Status_t Vesc_ApplyStopCurrent(uint8_t controller_id, uint32_t stop_current_mA)
{
    return ERPC_OK;
}

}