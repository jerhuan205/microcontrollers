/*
 * dac.h
 *
 *  Created on: Oct 20, 2024
 *      Author: jerem
 */
#include "main.h"

#ifndef SRC_DAC_H_
#define SRC_DAC_H_

// SPI-DAC Configuration Defintions
#define DAC_PORT GPIOA
#define AF5 0x5 	// AF5 is correlated to SPI for portA

// DAC Input & Output Calculation Definitions
#define DAC_HEADER 0x3 << 12
#define TEST_A52 0xA52 | DAC_HEADER	// 0b1010_0101_0010
#define MAX_BITS 4095
#define MAX_VOLT 3300
#define DIGIT1 1000
#define DIGIT2 100
#define DIGIT3 10
#define CALIBRATION 1.0091

// DAC Functions
void DAC_Config(void);
void DAC_Write(uint16_t voltage);
uint16_t DAC_Volt_Conv(uint16_t user_volt);

#endif /* SRC_DAC_H_ */
