#include "main.h"
//#include "string.h"

void SystemClock_Config(void);

// Configuration constants
#define AF7	0x7
#define BAUD_RATE	0x23
#define NVIC_31	0x1F

// String constants
#define NULL_TERM	'\0'

// USART Escape code constants
#define ESC 0x1b
#define RED_CODE "[31m"
#define GRE_CODE "[32m"
#define BLU_CODE "[34m"
#define WHI_CODE "[37m"

// USART Functions
void USART_Init(void);
void USART_Print_Char(char input);
void USART_Print_Str(char* string);
void USART_Send_ESC_Code(char* code);
void USART_Part2(void);

void USART_Init(){
	/*----------------------ENABLE CLOCKS----------------------*/
	RCC->AHB2ENR	|= RCC_AHB2ENR_GPIOAEN;
	RCC->APB1ENR1	|= RCC_APB1ENR1_USART2EN;

	/*---------------------CONFIGURE GPIOA---------------------*/
	// AF mode, output push-pull, low speed, no pull-up/down,
	GPIOA->MODER	&= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
	GPIOA->MODER	|=  (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
	GPIOA->OTYPER	&= ~(GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);
	GPIOA->OSPEEDR	&= ~(GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3);
	GPIOA->PUPDR	&= ~(GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3);

	GPIOA->AFR[0]	&= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
	GPIOA->AFR[0]	|= ((AF7 << GPIO_AFRL_AFSEL2_Pos) | (AF7 << GPIO_AFRL_AFSEL3_Pos));

	/*---------------------CONFIGURE USART---------------------*/
	// Transmission:
	// Program the M bits in USART_CR1 to define the word length.
	USART2->CR1		&= ~(USART_CR1_M1);
	// Select the desired baud rate using the USART_BRR register.
	USART2->BRR		|=  (BAUD_RATE);
	// Program the number of stop bits in USART_CR2.
	USART2->CR2		&= ~(USART_CR2_STOP);
	// Set oversampling to 16
	USART2->CR1 	&= ~(USART_CR1_OVER8);
	// Turn off parity bits
	USART2->CR1 	&= ~(USART_CR1_PCE);
	// Set the TE bit in USART_CR1 to send an idle frame as first transmission.
	USART2->CR1		|=  (USART_CR1_TE);

	// Reception:
	// Set the RE bit USART_CR1. This enables the receiver which begins searching for a start bit.
	USART2->CR1 	|= (USART_CR1_RE);
	// Enable the USART by writing the UE bit in USART_CR1 register to 1.
	USART2->CR1 	|= (USART_CR1_UE);
	// Enable interrupt for when register is ready to read
	USART2->CR1 	|= (USART_CR1_RXNEIE);

	/*---------------------CONFIGURE NVIC----------------------*/
	NVIC->ISER[1] |= (1 << (USART2_IRQn & NVIC_31));
}

void USART_Print_Char(char input) {
	// Wait for Transmission Complete bit, then write to Transmit reg
	while(!(USART2->ISR & USART_ISR_TC));
	USART2->TDR = input;
}

void USART_Print_Str(char* string) {
	// Print each char of the string excluding '\0'
	for(uint8_t i = 0; string[i] != NULL_TERM; i++){
		USART_Print_Char(string[i]);
	}
}

void USART_Send_ESC_Code(char* code) {
	// Send ESC then the code string
	USART_Print_Char(ESC);
	USART_Print_Str(code);
}

void USART_Part2(void) {
	// Escape code to clear the entire screen and move cursor back to top left
	USART_Send_ESC_Code("[2J");
	USART_Send_ESC_Code("[H");
	// 1. Escape codes to move the cursor:
	USART_Send_ESC_Code("[3B");	// down 3 lines and ...
	USART_Send_ESC_Code("[5C");	// to the right 5 spaces
	// 2. Text “All good students read the”
	USART_Print_Str("All good students read the");
	// 3. Escape codes to move the cursor down 1 line and to the left 21 spaces
	USART_Send_ESC_Code("[1B");
	USART_Send_ESC_Code("[21D");
	// 4. Escape code to change the text to blinking mode
	USART_Send_ESC_Code("[5m");
	// 5. Text “Reference Manual”
	USART_Print_Str("Reference Manual");
	// 6. Escape code to move cursor back to the top left position
	USART_Send_ESC_Code("[H");
	// 7. Escape code to remove character attributes (disable blinking text)
	USART_Send_ESC_Code("[0m");
	// 8. Text “Input: ”
	USART_Print_Str("Input: ");
}

int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	USART_Init();

	/* Enable global interrupts */
	__enable_irq();

	/*/ Part 1: Sending a single char repeatedly
	while (1)
	{
		USART_Print_Char('a');
		USART_Part1();
	}*/

	// Part 2 & 3: Using VT100 Escape Codes
	USART_Part2();
	while(1);
}

// Part 3: Echo Characters
void USART2_IRQHandler(void) {
	char input = USART2->RDR;
	switch(input) {

	case 'R':
		USART_Send_ESC_Code(RED_CODE);
		break;

	case 'G':
		USART_Send_ESC_Code(GRE_CODE);
		break;

	case 'B':
		USART_Send_ESC_Code(BLU_CODE);
		break;

	case 'W':
		USART_Send_ESC_Code(WHI_CODE);
		break;

	default:
		USART_Print_Char(input);
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
