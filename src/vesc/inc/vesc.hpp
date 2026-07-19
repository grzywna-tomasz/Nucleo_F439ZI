#pragma once

#include "std_types.h"
#include "can.hpp"

Std_ReturnType Vesc_Init(void);

class Vesc_Status1
{
public:
    /* ERPM = RPM * number of motor poles */
    int32_t getErpm() const
    {return m_ERPM;}
    /* 0.1 A/LSB */
    int16_t getCurrent() const
    {return m_current;}
    /* 0.1 %/LSB */
    int16_t getDutyCycle() const
    {return m_dutyCycle;}
    /* Decode Status1 frame */
    void status1Decode(uint8_t* data, uint8_t data_len);
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
    void status2Decode(uint8_t* data, uint8_t data_len);
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
    void status3Decode(uint8_t* data, uint8_t data_len);
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
    void status4Decode(uint8_t* data, uint8_t data_len);
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
    void status5Decode(uint8_t* data, uint8_t data_len);
private:
    int32_t m_tachometerValue {0};  /* TODOLater */ 
    int16_t m_voltageInput {0};     /* 0.1 V/LSB */
};

class VescMotorController
{
public:
    VescMotorController(uint8_t controller_id)
    : m_controllerId {controller_id}
    {}
    void setERPM(int32_t erpm_speed);
    void setCurrent(int32_t current);
    void sendPeriodic(void);
private:
    uint8_t m_controllerId;
    Std_ReturnType send4Bytes(int32_t value, uint16_t message_id);
    uint16_t m_periodicMode {ERPM_MODE};
    uint32_t m_periodicValue;
    /* This modes are at the same time IDs of VESC frames */
    static constexpr uint16_t ERPM_MODE = 0x300;
    static constexpr uint16_t CURRENT_MODE = 0x200;
};

class Vesc : public VescMotorController, public Vesc_Status1, public Vesc_Status2, public Vesc_Status3, public Vesc_Status4, public Vesc_Status5
{
public:
    Vesc(uint8_t controller_id, uint8_t motor_poles)
    : VescMotorController(controller_id), m_motorPoles {motor_poles}
    {}
    
    void decodeStatus(CanData_t& msg);
    int32_t getSpeed();
    void setSpeed(int32_t speed_rpm);
private:
    
    uint8_t m_motorPoles;
};