#include "main.h"

void SystemClock_Config(void);

#define ARR_VAL 800 - 1
#define CCR_VAL 200 - 1
#define NVIC_31	0x1F
#define LAST_BIT GPIO_ODR_OD0

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  // Configure GPIOC as output of the Timer (Using PC0)
  RCC->AHB2ENR	|=  (RCC_AHB2ENR_GPIOCEN);	// Enable Clock for PortC
  GPIOC->MODER	&= ~(GPIO_MODER_MODE0);		// Clear Bits...
  GPIOC->MODER	|=	(GPIO_MODER_MODE0_0);	// ...Output Mode
  GPIOC->OTYPER &= ~(GPIO_OTYPER_OT0);		// Use Push/Pull
  GPIOC->OSPEEDR&= ~(GPIO_OSPEEDR_OSPEED0);	// Use Low Speed
  GPIOC->PUPDR 	&= ~(GPIO_PUPDR_PUPD0);		// Use No Pull Up / Pull Down
  // Set PC0 Initially High, Start High for Correct +Duty Cycle
  GPIOC->ODR |= (GPIO_ODR_OD0);

  // Configure Timer2 to create 5kHz Sq. Wave, 25% Duty Cycle
  RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;		// Enable clock of Timer2
  TIM2->ARR 	 = ARR_VAL;					// Set ARR to 800-1 for a full period
  TIM2->CCR1 	 = CCR_VAL;					// Set CCR1 to 200-1 for the low time
  TIM2->CCER 	|= TIM_CCER_CC1E;			// Enable CC1 for Timer
  TIM2->DIER 	|= TIM_DIER_CC1IE | TIM_DIER_UIE;	// Enable CC1 & ARR interrupts
  TIM2->SR 		&= ~(TIM_SR_CC1IF | TIM_SR_UIF);	// Initialize flags to 0

  // Enable Global Interrupts
  __enable_irq();

  // Enable Interrupts for Timer2 in NVIC
  NVIC->ISER[0] = (1 << (TIM2_IRQn & NVIC_31));

  // Enable the Timer
  TIM2->CR1 |= TIM_CR1_CEN;

  while (1);
}

// Interrupt Subroutine for TIM2
void TIM2_IRQHandler(void)
{
	// Since we are only using CCR & ARR to toggle PC0 high and low at their respective counts,
	// we dont need to check if CCR1_flag or ARR_flag was triggered since they both just toggle.
	// However, we do need to clear the both Status Register flags after Subroutine is over.

	// Toggle just the last bit, PC0
	GPIOC->ODR ^= LAST_BIT;

	// Clear Flags for Both ARR and CC1
	TIM2->SR &= ~(TIM_SR_CC1IF | TIM_SR_UIF);
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
