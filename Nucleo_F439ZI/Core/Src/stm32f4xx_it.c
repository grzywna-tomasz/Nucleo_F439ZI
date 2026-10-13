/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
extern TIM_HandleTypeDef htim4;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

volatile uint32_t fault_r0_r12[13];
volatile uint32_t fault_lr;
volatile uint32_t fault_pc;
volatile uint32_t fault_xpsr;

volatile uint32_t fault_hfsr;
volatile uint32_t fault_cfsr;
volatile uint32_t fault_bfar;
volatile uint32_t fault_mmar;
volatile uint32_t fault_dfsr;
volatile uint32_t fault_afsr;
volatile uint32_t fault_shcsr;
volatile uint32_t fault_icsr;
volatile uint32_t fault_exc_return;
volatile uint32_t fault_stack_pointer;
volatile uint32_t fault_msp;
volatile uint32_t fault_psp;
volatile uint32_t fault_control;
volatile uint32_t fault_primask;
volatile uint32_t fault_basepri;
volatile uint32_t fault_faultmask;
volatile uint32_t fault_ipsr;
volatile uint32_t fault_frame_valid;
volatile uint32_t fault_fp_context_valid;
volatile uint32_t fault_fpccr;
volatile uint32_t fault_fpcar;
volatile uint32_t fault_fpscr;
volatile uint32_t fault_s0_s15[16];

__attribute__((naked))
void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4       \n"
        "ite eq           \n"
        "mrseq r0, msp    \n"
        "mrsne r0, psp    \n"
        "mov r1, lr       \n"
        "mrs r2, msp      \n"
        "mrs r3, psp      \n"
        "ldr r12, =fault_r0_r12 \n"
        "add r12, r12, #16 \n"
        "stmia r12, {r4-r11} \n"
        "b HardFault_C    \n"
    );
}

__attribute__((used, noinline, noreturn))
void HardFault_C(uint32_t *stack, uint32_t exc_return, uint32_t msp,
                 uint32_t psp)
{
    const uint32_t stacking_error_mask =
        (1u << 3) | (1u << 4) | (1u << 5) |
        (1u << 11) | (1u << 12) | (1u << 13);

    fault_exc_return = exc_return;
    fault_stack_pointer = (uint32_t)stack;
    fault_msp = msp;
    fault_psp = psp;
    fault_control = __get_CONTROL();
    fault_primask = __get_PRIMASK();
    fault_basepri = __get_BASEPRI();
    fault_faultmask = __get_FAULTMASK();
    fault_ipsr = __get_IPSR();
    fault_hfsr = SCB->HFSR;
    fault_cfsr = SCB->CFSR;
    fault_bfar = SCB->BFAR;
    fault_mmar = SCB->MMFAR;
    fault_dfsr = SCB->DFSR;
    fault_afsr = SCB->AFSR;
    fault_shcsr = SCB->SHCSR;
    fault_icsr = SCB->ICSR;
    fault_fpccr = FPU->FPCCR;
    fault_fpcar = FPU->FPCAR;

    if ((fault_cfsr & stacking_error_mask) == 0u)
    {
      uint32_t *core_stack = stack;

      fault_frame_valid = 1u;
      fault_fp_context_valid = 0u;
      if ((exc_return & (1u << 4)) == 0u)
      {
        if ((fault_fpccr & FPU_FPCCR_LSPACT_Msk) == 0u)
        {
          for (uint32_t i = 0u; i < 16u; ++i)
          {
            fault_s0_s15[i] = stack[i];
          }
          fault_fpscr = stack[16];
          fault_fp_context_valid = 1u;
        }
        core_stack += 18;
      }

      for (uint32_t i = 0u; i < 4u; ++i)
      {
        fault_r0_r12[i] = core_stack[i];
      }
      fault_r0_r12[12] = core_stack[4];
      fault_lr   = core_stack[5];
      fault_pc   = core_stack[6];
      fault_xpsr = core_stack[7];
    }
    else
    {
      fault_frame_valid = 0u;
      fault_fp_context_valid = 0u;
    }

    __BKPT(0);

    while (1) {}
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles Flash global interrupt.
  */
void FLASH_IRQHandler(void)
{
  /* USER CODE BEGIN FLASH_IRQn 0 */

  /* USER CODE END FLASH_IRQn 0 */
  HAL_FLASH_IRQHandler();
  /* USER CODE BEGIN FLASH_IRQn 1 */

  /* USER CODE END FLASH_IRQn 1 */
}

/**
  * @brief This function handles CAN1 TX interrupts.
  */
void CAN1_TX_IRQHandler(void)
{
  /* USER CODE BEGIN CAN1_TX_IRQn 0 */

  /* USER CODE END CAN1_TX_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan1);
  /* USER CODE BEGIN CAN1_TX_IRQn 1 */

  /* USER CODE END CAN1_TX_IRQn 1 */
}

/**
  * @brief This function handles CAN1 RX0 interrupts.
  */
void CAN1_RX0_IRQHandler(void)
{
  /* USER CODE BEGIN CAN1_RX0_IRQn 0 */

  /* USER CODE END CAN1_RX0_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan1);
  /* USER CODE BEGIN CAN1_RX0_IRQn 1 */

  /* USER CODE END CAN1_RX0_IRQn 1 */
}

/**
  * @brief This function handles TIM4 global interrupt.
  */
void TIM4_IRQHandler(void)
{
  /* USER CODE BEGIN TIM4_IRQn 0 */

  /* USER CODE END TIM4_IRQn 0 */
  HAL_TIM_IRQHandler(&htim4);
  /* USER CODE BEGIN TIM4_IRQn 1 */

  /* USER CODE END TIM4_IRQn 1 */
}

/**
  * @brief This function handles CAN2 TX interrupts.
  */
void CAN2_TX_IRQHandler(void)
{
  /* USER CODE BEGIN CAN2_TX_IRQn 0 */

  /* USER CODE END CAN2_TX_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan2);
  /* USER CODE BEGIN CAN2_TX_IRQn 1 */

  /* USER CODE END CAN2_TX_IRQn 1 */
}

/**
  * @brief This function handles CAN2 RX0 interrupts.
  */
void CAN2_RX0_IRQHandler(void)
{
  /* USER CODE BEGIN CAN2_RX0_IRQn 0 */

  /* USER CODE END CAN2_RX0_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan2);
  /* USER CODE BEGIN CAN2_RX0_IRQn 1 */

  /* USER CODE END CAN2_RX0_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
