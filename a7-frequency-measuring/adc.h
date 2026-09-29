/*
 * ADC.h
 *
 *  Created on: Nov 12, 2024
 *      Author: jerem
 */
#include "main.h"

#ifndef SRC_ADC_H_
#define SRC_ADC_H_

// Configuration Constants
#define SMPR_12p5 ADC_SMPR1_SMP5_1
#define SMPR_47p5 ADC_SMPR1_SMP5_2
#define SMPR_640p5 ADC_SMPR1_SMP5

// General Constants
#define ZERO 0
#define MIN_DIGITAL_VALUE ZERO
#define MAX_DIGITAL_VALUE 4095
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
uint32_t ADC_Calc_Avg(uint16_t samples[]);
uint16_t ADC_Calib_N_Conv_Value(uint16_t digital_value);
void ADC_Conv_Volts_To_Str(uint16_t voltage, char result[]);
void USART_Display_ADC_Values(char* min, char* max, char* avg);

#endif /* SRC_ADC_H_ */
