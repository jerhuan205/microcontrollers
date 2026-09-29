/*
 * ADC.c
 *
 *  Created on: Nov 12, 2024
 *      Author: jerem
 */
#include "main.h"
#include "USART.h"
#include "ADC.h"

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
	for(uint32_t i = 0; i < 480/*TWENTY_MICROS*/; i++);

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

	// 2.5 clock cycle sampling on channel 5
//	ADC1->SMPR1 = ~(ADC_SMPR1_SMP5);
	ADC1->SMPR1 = ~(SMPR_640p5);

	// Configure resolution / data alignment: Single conversion mode, 12-bit, right aligned
	/*ADC1->CFGR	&= ~(ADC_CFGR_CONT | ADC_CFGR_RES | ADC_CFGR_ALIGN);*/
	ADC1->CFGR	= 0;

	/*------------------CONFIGURE INTERRUPTS-------------------*/
	// Configure interrupts for peripheral and in table
	ADC1->IER 		|= (ADC_IER_EOCIE);
	NVIC->ISER[0] 	 = (1 << (ADC1_2_IRQn & 0x1F/*NVIC_31*/));

	/*---------------------CONFIGURE GPIOA---------------------*/
	// Configure GPIO pin for analog (PA0)
	GPIOA->MODER	|=	(GPIO_MODER_MODE0);
	GPIOA->ASCR		|= 	(GPIO_ASCR_ASC0);

	// Start a conversion
	ADC1->CR 	|= ADC_CR_ADSTART;
}

// Finds the minimum of all sampled ADC values
uint16_t ADC_Calc_Min(uint16_t samples[]) {
	uint16_t min = MAX_DIGITAL_VALUE;
	for(uint32_t i = ZERO; i < TABLE_SIZE; i++) {
		if(samples[i] < min) {
			min = samples[i];
		}
	}
	return min;
}

// Finds the maximum of all sampled ADC values
uint16_t ADC_Calc_Max(uint16_t samples[]) {
	uint16_t max = MIN_DIGITAL_VALUE;
	for(uint32_t i = ZERO; i < TABLE_SIZE; i++) {
		if(samples[i] > max) {
			max = samples[i];
		}
	}
	return max;
}

// Calculates the average of all sampled ADC values
uint32_t ADC_Calc_Avg(uint16_t samples[]) {
	uint32_t sum = ZERO;
	for(uint32_t i = ZERO; i < TABLE_SIZE; i++) {
		sum += samples[i];
	}
	return (uint32_t) (sum / TABLE_SIZE);
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
