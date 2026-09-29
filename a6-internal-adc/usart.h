/*
 * USART.h
 *
 *  Created on: Nov 7, 2024
 *      Author: jerem
 */

#ifndef SRC_USART_H_
#define SRC_USART_H_

#include "main.h"

// Configuration constants
#define AF7			0x7
#define BAUD_RATE 	0xD1	// 24 MHz Clk / 115.2 kbps
#define NVIC_31		0x1F

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

#endif /* SRC_USART_H_ */
