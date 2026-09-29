#include "main.h"
void SystemClock_Config(void);
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  // Step 1 Enable Clocks
  RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
  // Set ADC clock to HCLK / 1 synchronous
  ADC123_COMMON->CCR = (1 << ADC_CCR_CKMODE_Pos);

  // Power up ADC / Voltage regulator
  ADC1->CR &= ~(ADC_CR_DEEPPWD);
  ADC1->CR |=  (ADC_CR_ADVREGEN);
  // wait for 20 us
  for(uint32_t i = 0; i < 50000; i++);

  // Configure DifSel
  // Single ended mode for channel 5 (PA0)
  ADC1->DIFSEL &= ~(ADC_DIFSEL_DIFSEL_5);

  // Calibrate the ADC
  // ensure the adc is disabled and single ended calibration
  ADC1->CR &= ~(ADC_CR_ADEN | ADC_CR_ADCALDIF);
  ADC1->CR |= ADC_CR_ADCAL;
  while(ADC1->CR & ADC_CR_ADCAL);

  // Enable ADC
  // clear ready bit with a 1
  ADC1->ISR |= (ADC_ISR_ADRDY);
  ADC1->CR	|= (ADC_CR_ADEN);
  // wait for ADRDY to be a 1
  while(!(ADC1->ISR & ADC_ISR_ADRDY));

  // Configure Sequence
  // Single Channel (S) in sequence
  ADC1->SQR1 = (5 << ADC_SQR1_SQ1_Pos);

  // Configure resolution / data alignment
  // Single conversion mode, 12-bit, right aligned
  ADC1->CFGR = 0;

  // Configure interrupts if wanted

  // Configure GPIO pin (PA0)

  // Start a conversion
  ADC1->CR |= ADC_CR_ADSTART;







  while (1)
  {
  }
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
