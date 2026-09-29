/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "USART.h"

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

// Configuration Constants
#define SMPR_12p5 ADC_SMPR1_SMP5_1
#define SMPR_47p5 ADC_SMPR1_SMP5_2
#define SMPR_640p5 ADC_SMPR1_SMP5

// General Constants
#define ZERO 0
#define ON 1
#define TWENTY_MICROS	480	// With 24MHz Clk, found to be 476, rounded to be 480
#define TABLE_SIZE		200
#define DELAY 2500000

// Calibration Constants
#define M 810
#define B 8696
//#define M 805
//#define B 2720

// Digit Conversion
#define MAXIMUM_DIGITS	5
#define NULLTERM_DIGIT	4
#define THIRD_DIGIT		3
#define SECOND_DIGIT	2
#define DECIMAL_DIGIT	1
#define FIRST_DIGIT		0
#define TEN_THOUSAND 	10000
#define LEAST_SIG_DIGIT 10
#define MAKE_CHAR		'0'
#define DECIMAL_POINT	'.'

// Peripheral-specific Functions
void ADC_Init(void);
uint16_t ADC_Calc_Min(uint16_t samples[]);
uint16_t ADC_Calc_Max(uint16_t samples[]);
uint16_t ADC_Calc_Avg(uint16_t samples[]);
uint16_t ADC_Calib_N_Conv_Value(uint16_t digital_value);
void ADC_Conv_Volts_To_Str(uint16_t voltage, char result[]);
void USART_Display_ADC_Values(char* min, char* max, char* avg);

// Initializes the ADC Peripheral, alongside interrupts and routed GPIO pin
void ADC_Init(void) {
	/*----------------------ENABLE CLOCKS----------------------*/
	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

	/*---------------------CONFIGURE ADC1----------------------*/
	// Set ADC common clock to HCLK / 1, synchronous
	ADC123_COMMON->CCR = (1 << ADC_CCR_CKMODE_Pos);

	// Power up ADC & Enable Voltage regulator
	ADC1->CR	&= ~(ADC_CR_DEEPPWD);
	ADC1->CR	|=  (ADC_CR_ADVREGEN);
	// Wait for 20 micro sec
	for(uint32_t i = ZERO; i < TWENTY_MICROS; i++);

	// Configure DifSel
	// Single ended mode for channel 5 (PA0)
	ADC1->DIFSEL &= ~(ADC_DIFSEL_DIFSEL_5);

	// Calibrate the ADC, first ensure disabled and single ended calibration
	ADC1->CR	&= ~(ADC_CR_ADEN | ADC_CR_ADCALDIF);
	ADC1->CR	|= ADC_CR_ADCAL;
	while(ADC1->CR & ADC_CR_ADCAL);	// Wait for ADCAL == 0

	// Enable ADC
	ADC1->ISR 	|= (ADC_ISR_ADRDY);	// Clear ready bit with a 1
	ADC1->CR	|= (ADC_CR_ADEN);
	while(!(ADC1->ISR & ADC_ISR_ADRDY));	// Wait for ADRDY to be a 1

	// Configure Sequence: Single Channel (S) in sequence
	ADC1->SQR1	= (5 << ADC_SQR1_SQ1_Pos);

	// 12.5 clock sampling on channel 5
	// TODO:
//	ADC1->SMPR1 = (ZERO << ADC_SMPR1_SMP5_Pos);
//	ADC1->SMPR1 = SMPR_12p5;
//	ADC1->SMPR1 = SMPR_47p5;
	ADC1->SMPR1 = SMPR_640p5;

	// Configure resolution / data alignment: Single conversion mode, 12-bit, right aligned
	ADC1->CFGR	= ZERO;

	/*------------------CONFIGURE INTERRUPTS-------------------*/
	// Configure interrupts for peripheral, in table, and globally
	ADC1->IER 		|= (ADC_IER_EOCIE);
	NVIC->ISER[0] 	 = (1 << (ADC1_2_IRQn & NVIC_31));
	__enable_irq();

	/*---------------------CONFIGURE GPIOA---------------------*/
	// Configure GPIO pin for analog (PA0)
	GPIOA->MODER	|=	(GPIO_MODER_MODE0);
	GPIOA->ASCR		|= 	(GPIO_ASCR_ASC0);

	// Start a conversion
	ADC1->CR 	|= ADC_CR_ADSTART;
}

// Finds the minimum of all sampled ADC values
uint16_t ADC_Calc_Min(uint16_t samples[]) {
	uint16_t min = 4095;
	for(uint32_t i = ZERO; i < TABLE_SIZE; i++) {
		if(samples[i] < min) {
			min = samples[i];
		}
	}
	return min;
}

// Finds the maximum of all sampled ADC values
uint16_t ADC_Calc_Max(uint16_t samples[]) {
	uint16_t max = ZERO;
	for(uint32_t i = ZERO; i < TABLE_SIZE; i++) {
		if(samples[i] > max) {
			max = samples[i];
		}
	}
	return max;
}

// Calculates the average of all sampled ADC values
uint16_t ADC_Calc_Avg(uint16_t samples[]) {
	uint32_t sum = ZERO;
	for(uint32_t i = ZERO; i < TABLE_SIZE; i++) {
		sum += samples[i];
	}
	return (uint16_t) (sum / TABLE_SIZE);
}

// Calibrates the ADC value to uV then converts to V
uint16_t ADC_Calib_N_Conv_Value(uint16_t digital_value) {
	// Calibrate the digital value of ADC to micro-Volts
	uint32_t u_volts = M*digital_value + B;

	// Discard 4 decimal digits from the micro-Volts and return it
	uint16_t d_volts = u_volts / TEN_THOUSAND;
	return d_volts;
}

// Converts the voltage to a string by passing in an array of chars
void ADC_Conv_Volts_To_Str(uint16_t voltage, char result[]) {
	// Decimal digits for string representation 0.00
	// 											^ ^^
	uint8_t decimal_index = THIRD_DIGIT;
	while (decimal_index > DECIMAL_DIGIT) {
		// Set the 10th's & 100th's digit as chars at decimal_indices 3 & 2
		result[decimal_index] = ((voltage % LEAST_SIG_DIGIT) + MAKE_CHAR);
		// Decrement the decimal index
		decimal_index-= 1;
		// Get rid of the least significant digit by taking quotient of TEN
		voltage /= LEAST_SIG_DIGIT;
	}
	// Set the 1's digit to the array decimal index 0 as char & format digits
	result[FIRST_DIGIT] = voltage + MAKE_CHAR;
	result[DECIMAL_DIGIT] = DECIMAL_POINT;
	result[NULLTERM_DIGIT] = NULL_TERM;
}

// Displays the min, max, and average calculated voltages to Terminal
void USART_Display_ADC_Values(char* min, char* max, char* avg) {
	// Print Min on 1st line
	USART_Print_Str("Min: ");
	USART_Print_Str(min);
	USART_Print_Str(" V");

	// Print max on 2nd line by ...
	USART_Send_ESC_Code("[H");	// resetting cursor to upper left,
	USART_Send_ESC_Code("[1B"); // then down 1 line
	USART_Print_Str("Max: ");
	USART_Print_Str(max);
	USART_Print_Str(" V");

	// Print average on 3rd line by ...
	USART_Send_ESC_Code("[H");	// resetting cursor to upper left,
	USART_Send_ESC_Code("[2B");	// then down 2 lines
	USART_Print_Str("Avg: ");
	USART_Print_Str(avg);
	USART_Print_Str(" V");
}

// ADC Converted value saved to a global variable
uint16_t ADC_value = ZERO;

// Global flag is set when ISR is run
uint8_t ADC_flag = ZERO;

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	ADC_Init();
	USART_Init();

	// Escape code to clear the entire screen and move cursor back to top left
	USART_Send_ESC_Code("[2J");
	USART_Send_ESC_Code("[H");

	uint16_t samples[TABLE_SIZE];
	for(uint8_t i = ZERO; i< TABLE_SIZE; i++) {
		samples[i] = ZERO;
	}
	uint8_t index = 0;

	// Infinite loop checks for global flag set by ADC ISR
	while (1)
	{
		// When flag is set...
		if (ADC_flag) {
			// Save converted value to samples array, reset flag, increment index
			samples[index] = ADC_value;
			ADC_flag = ZERO;
			index++;
		}

		// If we sampled 20 times...
		if (index >= TABLE_SIZE) {
			// Calculate the min, max, and average ADC digital values
			uint16_t digit_min = ADC_Calc_Min(samples);
			uint16_t digit_max = ADC_Calc_Max(samples);
			uint16_t digit_avg = ADC_Calc_Avg(samples);

			// Calibrate these digital values to micro-Volts then Volts
			uint16_t calib_min = ADC_Calib_N_Conv_Value(digit_min);
			uint16_t calib_max = ADC_Calib_N_Conv_Value(digit_max);
			uint16_t calib_avg = ADC_Calib_N_Conv_Value(digit_avg);

			// Convert the Volts to a string
			char min_string[MAXIMUM_DIGITS];
			char max_string[MAXIMUM_DIGITS];
			char avg_string[MAXIMUM_DIGITS];
			ADC_Conv_Volts_To_Str(calib_min, min_string);
			ADC_Conv_Volts_To_Str(calib_max, max_string);
			ADC_Conv_Volts_To_Str(calib_avg, avg_string);

			// Display all 3 strings to Terminal
			USART_Display_ADC_Values(min_string, max_string, avg_string);

			// Software delay before restarting process at the top left of screen
			for(uint32_t i = ZERO; i < 100000/*DELAY*/; i++);
			USART_Send_ESC_Code("[H");

			for(uint32_t i = ZERO; i < 1000000/*DELAY*/; i++);

			// Reset the sample index to zero
			index = ZERO;
		}
		// Restart ADC to perform another sample and conversion
		ADC1->CR |= (ADC_CR_ADSTART);
	}
}

// ADC1&2 Interrupt Subroutine
void ADC1_2_IRQHandler(void) {
	// If the End of Conversion flag is raised ...
	if(ADC1->ISR & ADC_ISR_EOC) {
		// Save converted value into global variable and enable a global flag
		ADC_value = ADC1->DR;
		ADC_flag = ON;
	}

	// Clear the interrupt flag
	ADC1->ISR &= ~(ADC_ISR_EOC);
}

// TODO: OLD
//void ADC_Change_to_Volts_Str(uint32_t digital_value, char result[]) {
//	// Calibrate the digital value of ADC to micro-Volts
//	uint32_t voltage = M*digital_value + B;
//	uint8_t i = 0;
//	uint8_t decimal_index = 3;
//
//	result[1] = '.';
//	while(i < 7) {
//		// Discard 4 decimal digits from the 'voltage' in micro-Volts
//		// Decimal digits for string representation 0.00
//		// 											  ^^
//		if(i == 4 || i == 5) {
//			//Set decimal index at voltage
//			result[decimal_index] = ((voltage % 10) + '0');
//			//Decrement result placeholder
//			decimal_index-= 1;
//		}
//		//If we are on the last digit store it at 0th index
//		else if(i == 6) {
//			result[ZERO] = ((voltage) + '0');
//		}
//		// Discard decimal digits each iteration
//		voltage = voltage / 10;
//		i++;
//	}
//	result[4] = '\0';
//	return;
//}

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
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_9;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
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
