/*
 * USART.c
 *
 *  Created on: Nov 7, 2024
 *      Author: jerem
 */
#include "main.h"
#include "USART.h"

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

// TODO: NO NEED?
//	// Enable interrupt for when register is ready to read
//	USART2->CR1 	|= (USART_CR1_RXNEIE);
//
//	/*---------------------CONFIGURE NVIC----------------------*/
//	NVIC->ISER[1] |= (1 << (USART2_IRQn & NVIC_31));
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
