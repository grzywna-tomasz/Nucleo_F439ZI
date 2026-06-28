#include "can.hpp"
#include "std_utils.hpp"

extern "C" {
    #include "FreeRTOS.h"
    #include "task.h"
}

static constexpr uint32_t VESC_STATUS_MSG_MASK {0xFF00};

class Vesc_Status1
{
public:
    /* ERPM = RPM * number of motor poles */
    int32_t getErpm(uint8_t* data) const
    {return m_ERPM;}
    /* 0.1 A/LSB */
    int16_t getCurrent(uint8_t* data) const
    {return m_current;}
    /* 0.1 %/LSB */
    int16_t getDutyCycle(uint8_t* data) const
    {return m_dutyCycle;}
    /* Decode Status1 frame */
    void status1Decode(uint8_t* data, uint8_t data_len)
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
private:
    int32_t m_ERPM {0};         /* ERPM = RPM * number of motor poles */
    int16_t m_current {0};      /* 0.1 A/LSB */
    int16_t m_dutyCycle {0};    /* 0.001 /LSB */
};

class Vesc_Status2
{
public:
    /* 0.1 mAh/LSB */
    int32_t getAmpHours() const
    {return m_AmpHours;}
    /* 0.1 mAh/LSB */
    int32_t getAmpHoursCharged() const
    {return m_AmpHoursCharged;}
    /* Decode Status2 frame */
    void status2Decode(uint8_t* data, uint8_t data_len)
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
private:
    int32_t m_AmpHours {0};            /* 0.0001 Ah/LSB */
    int32_t m_AmpHoursCharged {0};    /* 0.0001 Ah/LSB */
};

class Vesc_Status3
{
public:
    /* 0.1 mWh/LSB */
    int32_t getWattHours() const
    {return m_WattHours;}
    /* 0.1 mWh/LSB */    
    int32_t getWattHoursCharged() const
    {return m_WatHoursCharged;}
    /* Decode Status 3 frame */
    void status3Decode(uint8_t* data, uint8_t data_len)
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
private:
    int32_t m_WattHours {0};           /* 0.0001 Wh/LSB */
    int32_t m_WatHoursCharged {0};   /* 0.0001 Wh/LSB */
};

class Vesc_Status4
{
public:
    /* 0.1 A/LSB */
    int16_t getInputCurrent() const
    {return m_inputCurrent;}
    /* 0.1 C/LSB */
    int16_t getMotorTemp() const
    {return m_motorTemp;}
    /* 0.1 C/LSB */
    int16_t getFETTemp() const
    {return m_FETTemp;}
    /* TODOLater */
    int16_t getPIDPosition() const
    {return m_PIDPosition;}
    /* Decode  */
    void status4Decode(uint8_t* data, uint8_t data_len)
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
private:
    int16_t m_inputCurrent {0}; /* 0.1 A/LSB */
    int16_t m_motorTemp {0};    /* 0.1 C/LSB */
    int16_t m_FETTemp {0};      /* 0.1 C/LSB */
    int16_t m_PIDPosition {0};  /* TODOLater */
};

class Vesc_Status5
{
public:
    /* TODOLater */
    int32_t getTachometer() const
    {return m_tachometerValue;}
    /* 0.1 V/LSB */
    int16_t getInputVoltage() const
    {return m_voltageInput;}
    void status5Decode(uint8_t* data, uint8_t data_len)
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
private:
    int32_t m_tachometerValue {0};  /* TODOLater */ 
    int16_t m_voltageInput {0};     /* 0.1 V/LSB */
};

class VescAllStatuses : public Vesc_Status1, public Vesc_Status2, public Vesc_Status3, public Vesc_Status4, public Vesc_Status5
{
public:
    VescAllStatuses(uint8_t controller_id)
    : m_controllerId {controller_id}
    {}
    void decodeStatus(CanData_t& msg)
    {
        switch (msg.id & (~m_controllerId))
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
private:
    uint8_t m_controllerId;
};

/* TODO - where to put it so it will be properly? Some header */
extern CanDriverInstance<5> CanDriver1;

static constexpr uint32_t VESC_STACK_SIZE {256U};
static constexpr uint32_t VESC_TASK_PRIORITY {tskIDLE_PRIORITY + 1};
StackType_t Vesc_Stack[VESC_STACK_SIZE];
StaticTask_t Vesc_TaskBuffer;
TaskHandle_t Vesc_TaskHandle = NULL;

CanListener<8> Vesc_CanListener;
VescAllStatuses Vesc_Controller1(32);

void Vesc_Task(void *pvParams)
{
    std::vector<VescAllStatuses*> controllersVector {&Vesc_Controller1};
    while(1)
    {
        CanData_t msg;
        Vesc_CanListener.waitForMsg(msg);
        for (VescAllStatuses*& controller : controllersVector)
        {
            controller->decodeStatus(msg);
        }
    }
}

Std_ReturnType Vesc_Init(void)
{
    Std_ReturnType ret_val {E_NOT_OK};
    
    Vesc_TaskHandle = xTaskCreateStatic(Vesc_Task, "VescTask", VESC_STACK_SIZE, (void *) 0, VESC_TASK_PRIORITY, Vesc_Stack, &Vesc_TaskBuffer);

    if (Vesc_TaskHandle)
    {
        ret_val = Vesc_CanListener.init(&CanDriver1);
    }

    return ret_val;
}
