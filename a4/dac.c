/*
 * dac.c
 *
 *  Created on: Oct 20, 2024
 *      Author: jerem
 */
#include "main.h"
#include "dac.h"

void DAC_Config(void)
{
	// Enable Clock for PortA
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

	/* Configure PA4 = CS, PA5 = SCLK, PA7 = MOSI		(SPI Communication) */
	// af mode, push-pull, low speed, no pull up/down resistor
	DAC_PORT->MODER		&= ~(GPIO_MODER_MODE4 | GPIO_MODER_MODE5 | GPIO_MODER_MODE7);
	DAC_PORT->MODER		|=  (GPIO_MODER_MODE4_1 | GPIO_MODER_MODE5_1 | GPIO_MODER_MODE7_1);
	DAC_PORT->OTYPER	&= ~(GPIO_OTYPER_OT4 | GPIO_OTYPER_OT5 | GPIO_OTYPER_OT7);
	DAC_PORT->OSPEEDR	&= ~(GPIO_OSPEEDR_OSPEED4| GPIO_OSPEEDR_OSPEED5 | GPIO_OSPEEDR_OSPEED7);
	DAC_PORT->PUPDR		&= ~(GPIO_PUPDR_PUPD4 | GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD7);

	// Set alt. func. bits to 0x5 for AF5 (SPI)
	DAC_PORT->AFR[0] &= ~(GPIO_AFRL_AFSEL4 | GPIO_AFRL_AFSEL5 | GPIO_AFRL_AFSEL7);
	DAC_PORT->AFR[0] |= ((AF5 << GPIO_AFRL_AFSEL4_Pos) | (AF5 << GPIO_AFRL_AFSEL5_Pos) | (AF5 << GPIO_AFRL_AFSEL7_Pos));

	// Enable Clock for SPI
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

	// Set the Baud Rate to fpClk/2
	SPI1->CR1 &= ~(SPI_CR1_BR);

	// CPOL & CPHA Clock Timing
	SPI1->CR1 &= ~(SPI_CR1_CPOL); // Idle Low
	SPI1->CR1 &= ~(SPI_CR1_CPHA); // 1st clk transition is first data capture

	// Simplex (using mainly Transmit-Only commnication) Mode
	SPI1->CR1 &= ~(SPI_CR1_RXONLY);

	// Send MSB first
	SPI1->CR1 &= ~(SPI_CR1_LSBFIRST);

	// No Software Slave Management (SSM)
	SPI1->CR1 &= ~(SPI_CR1_SSM | SPI_CR1_SSI);

	// Set Mode to Master
	SPI1->CR1 |= SPI_CR1_MSTR;

	// Configure DS bits for 16-bit data length of transfer
	SPI1->CR2 |= SPI_CR2_DS;

	// Configure SSOE (software-select output enable) bit to be 1
	SPI1->CR2 |= SPI_CR2_SSOE;

	// Set NSSP bit for pulse mode
	SPI1->CR2 |= SPI_CR2_NSSP;

	// Enable SPI Peripheral
	SPI1->CR1 |= SPI_CR1_SPE;
}

void DAC_Write(uint16_t voltage)
{
	// Append the 4-bit header to the voltage, allows DAC to function
	voltage |= DAC_HEADER;

	// Wait for Tx to be empty before writing to the data register
	while(!(SPI1->SR & SPI_SR_TXE));
	SPI1->DR = voltage;

	// Wait for Tx to be empty again and non-busy bus
	while ((!(SPI1->SR & SPI_SR_TXE)) && (SPI1->SR & SPI_SR_BSY));
}

uint16_t DAC_Volt_Conv(uint16_t in_volt)
{
	// Return the max bits if the input voltage is higher than the allowed voltage
	if (in_volt > MAX_VOLT)
	{
		return MAX_BITS;
	}
	// Otherwise, just return the converted voltage value within the range of 0-4095
	in_volt = (in_volt * MAX_BITS * CALIBRATION) / MAX_VOLT;

	// Return an appended 4-bit header to the voltage, allows DAC to function
//	return in_volt |= DAC_HEADER;
	return in_volt;
}
