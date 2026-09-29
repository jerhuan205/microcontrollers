#include "main.h"
#include "keypad.h"
#include "dac.h"

void SystemClock_Config(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  // Configure the Keypad & DAC
  KEYPAD_Config();
  DAC_Config();

  // Have an array to store the keypad inputs and array index
  uint16_t digits[3] = {0, 0, 0};
  uint16_t dig_ind = 0;

  // Initialize the DAC output to 0
  DAC_Write(0);

  // Have a way to store the keypad value
  int8_t key = 0;

  /* Infinite loop */
  while (1)
  {
	  // Part 1-4: Verifying timing of CS signal & Data transmission of 0x3A52
	  // DAC_Write(TEST_A52);

	  // Part 5-6: Controlling the DAC with Keypad
	  // Continuously read the keypad until it is pressed
	  while((key = KEYPAD_Read() )== NO_PRESS)
	  {
		  // Delay in between key reads
		  for(int delay=0; delay<2000; delay++);
		  key = KEYPAD_Read();
	  }

	  // During a key's press, update our history of digits and increment index
	  digits[dig_ind] = key;
	  dig_ind++;

	  // Wait until the key has been released
	  while (KEYPAD_Read() != NO_PRESS);

	  // When there have been 3 presses stored...
	  if (dig_ind >= 3)
	  {
		  // ... Calculate the result in milli-Volts and write to the DAC
		  uint16_t result = DIGIT1*digits[0] + DIGIT2*digits[1] + DIGIT3*digits[2];
		  DAC_Write(DAC_Volt_Conv(result));

		  // Reset our history of stored digits as well as our index
		  for(int i=0; i<3; i++) {digits[i] = 0;}
		  dig_ind = 0;
	  }
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
