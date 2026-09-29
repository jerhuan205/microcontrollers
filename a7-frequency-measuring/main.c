#include "main.h"
#include "USART.h"
#include "ADC.h"
#include "string.h"
#include <stdio.h>

// TODO: sometimes at 1-2 Hz, it will just crash, no frequency shown, no sweep state, nothing

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

void TIM2_Init(void) {
	// Setup Clock for Timer
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

	/*----------------------- Configure upcounter to count in response to a rising edge: ------------------*/
	// Setup Timer 2 in Capture Mode
	TIM2->ARR = TIM_ARR_ARR;
	// Remap Input Capture 4 to be COMP1_OUT
	TIM2->OR1 = (TIM2_OR1_TI4_RMP_0);
	// configure channel 2 to detect rising edges on the TI2 input, CC2S = '01 in TIM2_CCMR1 reg
	TIM2->CCMR2 &= ~(TIM_CCMR2_CC4S);
	TIM2->CCMR2 |= 	(TIM_CCMR2_CC4S_0); // Using Signal TI4
		// TODO: configure input filter by writing IC2F bits in TIM_CCMR1 & IC4PSC prescaler
	TIM2->CCMR2 &= ~(TIM_CCMR2_IC4PSC); // No Prescaler
	// select rising edge polarity by writing CC2P=0 and CC2NP=0 and CC2NP=0 in the TIMx_CCER register.
	TIM2->CCER &= ~(TIM_CCER_CC4NP | TIM_CCER_CC4P);	// Trigger on rising edge
	TIM2->CCER |= TIM_CCER_CC4E;	// Enable Capture for 4
	// Enable the interrupt for Capture Channel 4
	TIM2->DIER |= TIM_DIER_CC4IE;
	// Enable Interrupts for Timer2
	NVIC->ISER[0] |= (1 << (TIM2_IRQn & 0x1F));

	// Begin Timer
	TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM3_Init(void) {
	// Setup Clock for Timer
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;

	// Setup Timer 3 With a Default of 1000 samples on a 500 Hz Signal
	TIM3->ARR = TIM_ARR_ARR;
	TIM3->CCR1 = 240000;
	TIM3->CCMR1 &= ~(TIM_CCMR1_CC1S);
	TIM3->CCER |= TIM_CCER_CC1E;
	TIM3->DIER |= TIM_DIER_CC1IE;

	NVIC->ISER[0] |= (1 << (TIM3_IRQn & 0x1F));
}

void GPIO_Test(void) {
	/* Configure for TIM2 IRQ output */
	// output mode, push-pull, low speed, no pull up/down resistor
	GPIOB->MODER	&= ~(GPIO_MODER_MODE0);
	GPIOB->MODER	|=  (GPIO_MODER_MODE0_0);
	GPIOB->OTYPER	&= ~(GPIO_OTYPER_OT0);
	GPIOB->OSPEEDR	&= ~(GPIO_OSPEEDR_OSPEED0);
	GPIOB->PUPDR	&= ~(GPIO_PUPDR_PUPD0);
}

void COMP1_Init(void) {
	// Configure comparator COMP1
	// Configure clocks first
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;	// clocl for comp regs
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;	// clock for gpio regs

	// Enable GPIOB for PB2 & PB0
	// 	PB2: Positive Input
	// 	PB1: Negative Input
	// 	PB0: COMP1_OUT
	// Set PB2 to analog mode
	GPIOB->MODER |= GPIO_MODER_MODE2;
	COMP1->CSR |= (COMP_CSR_INPSEL);	// PB2 as positive input (INPSEL = 0b01)
	COMP1->CSR |= (COMP_CSR_INMSEL_2);	// DAC is the Reference Voltage
	GPIOB->MODER |= GPIO_MODER_MODE1;
//	COMP1->CSR |= (0b110 << COMP_CSR_INMSEL_Pos);	// TODO: Reference Voltage is PB1
	COMP1->CSR |= (COMP_CSR_HYST);	// High Hysteresis
	COMP1->CSR |= (COMP_CSR_PWRMODE); // Ultra low speed

	// Enable comparator COMP1
	COMP1->CSR |= COMP_CSR_EN;
}

void DAC1_Init(void) {
	RCC->APB1ENR1 |= RCC_APB1ENR1_DAC1EN;
	/* Configure for DAC1 output */
	RCC->AHB2ENR 	|= RCC_AHB2ENR_GPIOAEN;
	GPIOA->MODER	|= (GPIO_MODER_MODE4);

	// Setup the Reference Voltage DAC
	// Using Normal Mode with No Buffer. Outputs to Peripherals
	DAC1->MCR &= ~(DAC_MCR_MODE1);
	DAC1->MCR |= (0b011 << DAC_MCR_MODE1_Pos);	// only to on-chip peripherals
//	DAC1->MCR |= (0b010 << DAC_MCR_MODE1_Pos); // For Testing, DAC Output is PA4
	// Note: Assuming a 1-1 Conversion From ADC Value to DAC Value
	uint32_t default_1p5 = (0 + 4095) >> 1;
	// Store Value in DAC
	DAC1->DHR12R1 &= ~(DAC_DHR12R1_DACC1DHR);
	DAC1->DHR12R1 |= default_1p5;
	DAC1->CR |= DAC_CR_TSEL1;		// Type of selection is software trigger
	DAC1->CR |= DAC_CR_TEN1;		// Enable DAC channel 1 trigger
	DAC1->CR |= DAC_CR_EN1;			// Enable the DAC
	DAC1->SWTRIGR |= DAC_SWTRIGR_SWTRIG1;	// trigger dac1 via software by setting
}

//// The Number of Timer Clocks Per Second
//// Clock is at 24 MHz, so CPS is 24000000
#define MCU_SPEED 24000000
uint32_t old_count = 0;
uint32_t current_period = 0;
uint32_t sample_rate_clock_count = 240000;
uint32_t old_frequency = 0;
uint8_t flag_calculate_freq = 0;

// ADC Converted value saved to a global variable
uint16_t ADC_value = ZERO;

// Global flag is set when ISR is run
uint8_t ADC_flag = ZERO;

void old_sweep() {
	/*uint16_t compared_min = ADC_Calib_N_Conv_Value(comp_min);
		char comp_min_string[MAXIMUM_DIGITS];
		ADC_Conv_Volts_To_Str(compared_min, comp_min_string);
		USART_Print_Str(comp_min_string);
		USART_Send_ESC_Code("[H");
		USART_Send_ESC_Code("[3B");
		uint16_t compared_max = ADC_Calib_N_Conv_Value(comp_max);
		char comp_max_string[MAXIMUM_DIGITS];
		ADC_Conv_Volts_To_Str(compared_max, comp_max_string);
		USART_Print_Str(comp_max_string);*/

	//	uint32_t sum = 0;
	//	uint32_t count = 0;
	//	while (count < TABLE_SIZE) {
	//		if (ADC_flag) {
	//			sum += ADC_value;
	//			ADC_flag = ZERO;
	//			count++;
	//		}
	//	}
	//	sum = sum / TABLE_SIZE;
	//
	//	// Note: Assuming a 1-1 Conversion From ADC Value to DAC Value
	//	// Store Value in DAC
	//	DAC1->DHR12R1 &= ~(DAC_DHR12R1_DACC1DHR);
	//	DAC1->DHR12R1 |= sum;
	//
	//	uint16_t calib_sum = ADC_Calib_N_Conv_Value(sum);
	//	char sum_string[MAXIMUM_DIGITS];
	//	ADC_Conv_Volts_To_Str(calib_sum, sum_string);
	//	USART_Print_Str(sum_string);
	//
	//	// Software delay before restarting process at the top left of screen
	//	USART_Send_ESC_Code("[H");
}

void sweep_signal() {
	TIM3->CR1 |= TIM_CR1_CEN;
	uint16_t samples[TABLE_SIZE];
	for(uint8_t i = ZERO; i< TABLE_SIZE; i++) {
		samples[i] = ZERO;
	}
	uint8_t index = 0;

	while (index < TABLE_SIZE) {
		// When flag is set...
		if (ADC_flag) {
			// Save converted value to samples array, reset flag, increment index
			samples[index] = ADC_value;
			ADC_flag = ZERO;
			index++;
		}
	}
	TIM3->CR1 &= ~(TIM_CR1_CEN);

	// Find the min and max digital values
	uint16_t digit_min = ADC_Calc_Min(samples);
	uint16_t digit_max = ADC_Calc_Max(samples);
	// Note: Assuming a 1-1 Conversion From ADC Value to DAC Value
	// Store Value in DAC
	uint32_t digit_sum = digit_min + digit_max;
	DAC1->DHR12R1 &= ~(DAC_DHR12R1_DACC1DHR);
	DAC1->DHR12R1 |= (digit_sum) >> 1;			// Divide the sum by 2
	DAC1->SWTRIGR |= DAC_SWTRIGR_SWTRIG1;	// trigger dac1 via software by setting

	// TODO: TESTING WHAT THESE WILL PRINT TO USART
	// Calibrate these digital values to micro-Volts then convert to Volts
	uint16_t calib_min = ADC_Calib_N_Conv_Value(digit_min);
	uint16_t calib_max = ADC_Calib_N_Conv_Value(digit_max);
	// Convert the Volts to a string
	char min_string[MAXIMUM_DIGITS];
	char max_string[MAXIMUM_DIGITS];
	ADC_Conv_Volts_To_Str(calib_min, min_string);
	ADC_Conv_Volts_To_Str(calib_max, max_string);
	// Print the strings
	USART_Print_Str(min_string);
	USART_Send_ESC_Code("[H");
	USART_Send_ESC_Code("[1B");
	USART_Print_Str(max_string);

	// Add the micro_volts together
	uint32_t min_u_volts = M*digit_min + B;
	uint32_t max_u_volts = M*digit_max + B;
	uint32_t sum_u_volts = min_u_volts + max_u_volts;
	// Discard 4 decimal digits from the micro-Volts to turn into Volts
	uint16_t sum_d_volts = sum_u_volts / TEN_THOUSAND;
	sum_d_volts /= 2;
	// Convert the Volts to a string
	char sum_string[MAXIMUM_DIGITS];
	ADC_Conv_Volts_To_Str(sum_d_volts, sum_string);
	USART_Send_ESC_Code("[H");
	USART_Send_ESC_Code("[2B");
	USART_Print_Str(sum_string);
}

typedef enum {
	ST_SWEEP,
	ST_DISPLAY
} state_type;

uint8_t fail = 0;

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

	// Enable Interrupts globally
	__enable_irq();

	/* Initialize all configured peripherals */
	DAC1_Init();
	COMP1_Init();
	TIM2_Init();
	USART_Init();

	ADC_Init();
	TIM3_Init();

	// Escape code to clear the entire screen and move cursor back to top left
	USART_Send_ESC_Code("[2J");
	USART_Send_ESC_Code("[H");

	// Current state of the program
	state_type state = ST_SWEEP;
	TIM2->CR1 |= TIM_CR1_CEN;

	while(1) {
		switch(state) {
		case ST_SWEEP:
		{
			USART_Send_ESC_Code("[H");
			sweep_signal();
			state = ST_DISPLAY;
			break;
		}
		case ST_DISPLAY:
		{
			while(!flag_calculate_freq) {
//				TIM3->CNT = 0;
//				TIM3->CCR2 = 300000;
//				TIM3->CCMR1 &= ~(TIM_CCMR1_CC2S);
//				TIM3->CCER |= TIM_CCER_CC2E;
//				TIM3->DIER |= TIM_DIER_CC2IE;
//
//				TIM3->ARR = TIM_ARR_ARR;
//				TIM3->CCR1 = 240000;
//				TIM3->CCMR1 &= ~(TIM_CCMR1_CC1S);
//				TIM3->CCER &= ~TIM_CCER_CC1E;
//				TIM3->DIER &= ~TIM_DIER_CC1IE;
//
//				TIM3->CR1 |= TIM_CR1_CEN;
//
//				while(fail < 7) {
//
//				}
				USART_Send_ESC_Code("[H");
				USART_Send_ESC_Code("[4B");
				USART_Send_ESC_Code("[0J");
				USART_Print_Str("Stuck");

			}
			if (flag_calculate_freq) {
				//			USART_Send_ESC_Code("[2J");
				USART_Send_ESC_Code("[H");
				USART_Send_ESC_Code("[4B");
				USART_Send_ESC_Code("[0J");
				USART_Print_Str("Frequency: ");

				// Declare Variable to hold integer frequency
				uint32_t current_frequency;
				// Calculate the frequency
				current_frequency = (MCU_SPEED / current_period) + 1;

				uint32_t decimal_value = 10;
				while (current_frequency >= decimal_value) {
					decimal_value *= 10;
				}

				// Write Each Digit of the Base 10 Value
				while (decimal_value >= 10) {
					decimal_value /= 10;
					uint32_t intermediate = current_frequency / decimal_value;
					current_frequency-= decimal_value * intermediate;
					USART_Print_Char('0' + intermediate);
				}
				USART_Print_Str("Hz");

				USART_Send_ESC_Code("[H");
				USART_Send_ESC_Code("[5B");
				uint32_t copy = current_period;
				decimal_value = 10;
				while (copy >= decimal_value) {
					decimal_value *= 10;
				}

				// Write Each Digit of the Base 10 Value
				while (decimal_value >= 10) {
					decimal_value /= 10;
					uint32_t intermediate = copy / decimal_value;
					copy-= decimal_value * intermediate;
					USART_Print_Char('0' + intermediate);
				}

				flag_calculate_freq = 0;
				// Software delay before restarting process at the top left of screen
			}
			USART_Send_ESC_Code("[H");
			state = ST_SWEEP;
			break;
		}
		default:
		{
			state = ST_SWEEP;
			break;
		}
		}
	}
}

// Handler for Tim2 (Comparator Capture)
void TIM2_IRQHandler(void) {
	if (TIM2->SR & TIM_SR_CC4IF) {
		// Store Current Frequency Value
		uint32_t new_count = TIM2->CCR4;
		if (new_count > old_count) {
			current_period = new_count - old_count;
		} else {
			current_period = old_count - new_count;
		}
		old_count = new_count;
		flag_calculate_freq = 1;
		TIM2->SR &= ~(TIM_SR_CC4IF);
	}
}


// Handler for Tim3 (Signal ADC Sampling)
void TIM3_IRQHandler(void) {
	if (TIM3->SR & TIM_SR_CC1IF) {
		// Increment the Compare Register to Wait for Next Sample
		TIM3->CCR1 += 240000;
		// Begin the Next Sample
		ADC1->CR |= ADC_CR_ADSTART;
		// Reset the CC1 Flag to Re-Enable Interrupts
		TIM3->SR &= ~(TIM_SR_CC1IF);
	}
}

// ADC1&2 Interrupt Subroutine
void ADC1_2_IRQHandler(void) {
	// If the End of Conversion flag is raised ...
	if(ADC1->ISR & ADC_ISR_EOC) {
		// Save converted value into global variable and enable a global flag
		ADC_value = ADC1->DR;
		ADC_flag = ON;

		// Clear the interrupt flag
		ADC1->ISR &= ~(ADC_ISR_EOC);
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
