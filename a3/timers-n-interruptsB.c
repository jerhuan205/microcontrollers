#include "main.h"

void SystemClock_Config(void);

// Part B definitions
#define ARR_MAX 0xFFFFFFFF
#define CCR_INC 400
#define NVIC_31	0x1F
#define LAST_BIT GPIO_ODR_OD0
#define TIM_OUTPUTPORT GPIOC
#define ISR_OUTPUTPORT GPIOB
#define CLK_OUTPUTPORT GPIOA

// Part C definitions
#define CCR_BARELY 50
#define CCR_BREAK 49

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  /* LAB-PASTED CODE - START -------------------------------------------------*/
  // Enable MCO, select MSI (4 MHz source)
  RCC->CFGR = ((RCC->CFGR & ~(RCC_CFGR_MCOSEL)) | (RCC_CFGR_MCOSEL_0));

  // Configure MCO output on PA8
  RCC->AHB2ENR   |=  (RCC_AHB2ENR_GPIOAEN);
  CLK_OUTPUTPORT->MODER   &= ~(GPIO_MODER_MODE8);		// alternate function mode
  CLK_OUTPUTPORT->MODER   |=  (2 << GPIO_MODER_MODE8_Pos);
  CLK_OUTPUTPORT->OTYPER  &= ~(GPIO_OTYPER_OT8);		// Push-pull output
  CLK_OUTPUTPORT->PUPDR   &= ~(GPIO_PUPDR_PUPD8);		// no resistor
  CLK_OUTPUTPORT->OSPEEDR |=  (GPIO_OSPEEDR_OSPEED8);	// high speed
  CLK_OUTPUTPORT->AFR[1]  &= ~(GPIO_AFRH_AFSEL8);		// select MCO function
  /* LAB-PASTED CODE - END ---------------------------------------------------*/

  // Configure GPIOB to measure ISR processing time (Using PB0)
  RCC->AHB2ENR	|=  (RCC_AHB2ENR_GPIOBEN);	// Enable Clock for PortB
  ISR_OUTPUTPORT->MODER	 	&= ~(GPIO_MODER_MODE0);		// Clear Bits...
  ISR_OUTPUTPORT->MODER	 	|=	(GPIO_MODER_MODE0_0);	// ...Output Mode
  ISR_OUTPUTPORT->OTYPER 	&= ~(GPIO_OTYPER_OT0);		// Use Push/Pull
  ISR_OUTPUTPORT->OSPEEDR	&= ~(GPIO_OSPEEDR_OSPEED0);	// Use Low Speed
  ISR_OUTPUTPORT->PUPDR	 	&= ~(GPIO_PUPDR_PUPD0);		// Use No Pull Up / Pull Down

  //Set PB0 initially low.
  ISR_OUTPUTPORT->ODR &= ~(GPIO_ODR_OD0);

  // Configure GPIOC as output of the Timer (Using PC0)
  RCC->AHB2ENR	|=  (RCC_AHB2ENR_GPIOCEN);	// Enable Clock for PortC
  TIM_OUTPUTPORT->MODER		&= ~(GPIO_MODER_MODE0);		// Clear Bits...
  TIM_OUTPUTPORT->MODER		|=	(GPIO_MODER_MODE0_0);	// ...Output Mode
  TIM_OUTPUTPORT->OTYPER	&= ~(GPIO_OTYPER_OT0);		// Use Push/Pull
  TIM_OUTPUTPORT->OSPEEDR	&= ~(GPIO_OSPEEDR_OSPEED0);	// Use Low Speed
  TIM_OUTPUTPORT->PUPDR 	&= ~(GPIO_PUPDR_PUPD0);		// Use No Pull Up / Pull Down

  // Set PC0 initially high for correct +Duty Cycle
  TIM_OUTPUTPORT->ODR |= (GPIO_ODR_OD0);

  // Configure Timer2 to create 5kHz Sq. Wave, 50% Duty Cycle
  RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;		// Enable clock of Timer2
  TIM2->ARR 	 = ARR_MAX;					// Set ARR to run continuously
  TIM2->CCR1 	 = CCR_INC;					// Set CCR1 to 400 initially
  TIM2->CCER 	|= TIM_CCER_CC1E;			// Enable CC1 for Timer
  TIM2->DIER 	|= TIM_DIER_CC1IE;			// Enable CCI interrupt
  TIM2->SR 		&= ~(TIM_SR_CC1IF);			// Initialize CC1 flag to 0

  // Enable Global Interrupts
  __enable_irq();

  // Enable Interrupts for Timer
  NVIC->ISER[0] = (1 << (TIM2_IRQn & NVIC_31));

  // Enable the Timer
  TIM2->CR1 |= TIM_CR1_CEN;

  while (1);
}

// Interrupt Subroutine for TIM2
void TIM2_IRQHandler(void)
{
	// ISR time: drive pin high to measure
	ISR_OUTPUTPORT->BSRR = (GPIO_PIN_0);

		// TIM sq. wave: toggle just the last bit, PC0
		TIM_OUTPUTPORT->ODR ^= LAST_BIT;

		// Increment CC1 for the next half period mark
		TIM2->CCR1 += CCR_INC;

		// Clear flag for CC1
		TIM2->SR &= ~(TIM_SR_CC1IF);

	// ISR time: Drive pin low before exiting
	ISR_OUTPUTPORT->BRR = (GPIO_PIN_0);
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
