/*
 * keypad.c
 *
 *  Created on: Sep 30, 2024
 *      Author: jerem
 */
#include "main.h"
#include "keypad.h"

const int8_t BUTTONS[NUM_ROWS][NUM_COLS] =
{
		{ KEY_1, KEY_2, KEY_3, KEY_A },
		{ KEY_4, KEY_5, KEY_6, KEY_B },
		{ KEY_7, KEY_8, KEY_9, KEY_C },
		{ STAR , KEY_0, HASH , KEY_D }
};

void keypad_init()
{
	/* Initialize all configured peripherals */
	RCC->AHB2ENR	|= RCC_AHB2ENR_GPIOAEN;		// Enable GPIOA Clock	(LED Display)
	RCC->AHB2ENR	|= RCC_AHB2ENR_GPIOBEN;		// 		  GPIOB Clock	(COL Outputs)
	RCC->AHB2ENR	|= RCC_AHB2ENR_GPIOCEN;		// 		  GPIOC Clock	(ROW Inputs)

	/* Configure PA4, PA5, PA6, PA7		(LED Display) */
	// output mode, push-pull, low speed, no pull up/down resistor
	GPIOA->MODER	&= ~(GPIO_MODER_MODE4 | GPIO_MODER_MODE5 | GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
	GPIOA->MODER	|= 	(GPIO_MODER_MODE4_0 | GPIO_MODER_MODE5_0 | GPIO_MODER_MODE6_0 | GPIO_MODER_MODE7_0);
	GPIOA->OTYPER	&= ~(GPIO_OTYPER_OT4 | GPIO_OTYPER_OT5 | GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);
	GPIOA->OSPEEDR	&= ~(GPIO_OSPEEDR_OSPEED4 | GPIO_OSPEEDR_OSPEED5 | GPIO_OSPEEDR_OSPEED6 | GPIO_OSPEEDR_OSPEED7);
	GPIOA->PUPDR	&= ~(GPIO_PUPDR_PUPD4 | GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7);
	GPIOA->ODR		&= ~(LED_MASK);		// Set LEDs to 0

	/* Configure PB12, PB13, PB14, PB15 (COL outputs) */
	// output mode, push-pull, low speed, no pull up/down resistor
	GPIOB->MODER	&= ~(GPIO_MODER_MODE12 | GPIO_MODER_MODE13 | GPIO_MODER_MODE14 | GPIO_MODER_MODE15);
	GPIOB->MODER	|=  (GPIO_MODER_MODE12_0 | GPIO_MODER_MODE13_0 | GPIO_MODER_MODE14_0 | GPIO_MODER_MODE15_0);
	GPIOB->OTYPER	&= ~(GPIO_OTYPER_OT12 | GPIO_OTYPER_OT13 | GPIO_OTYPER_OT14 | GPIO_OTYPER_OT15);
	GPIOB->OSPEEDR	&= ~(GPIO_OSPEEDR_OSPEED12 | GPIO_OSPEEDR_OSPEED13 | GPIO_OSPEEDR_OSPEED14 | GPIO_OSPEEDR_OSPEED15);
	GPIOB->PUPDR	&= ~(GPIO_PUPDR_PUPD12 | GPIO_PUPDR_PUPD13 | GPIO_PUPDR_PUPD14 | GPIO_PUPDR_PUPD15);
	GPIOB->ODR		&= ~(COL_MASK);
	GPIOB->ODR		|=  (COL_MASK);		// Set COLs to 1

	/* Configure PC0, PC1, PC2, PC3 	(ROW inputs) */
	// input mode, push-pull, low speed, pull-down resistor connected
	GPIOC->MODER	&= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1 | GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
	GPIOC->OTYPER	&= ~(GPIO_OTYPER_OT0 | GPIO_OTYPER_OT1 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);
	GPIOC->OSPEEDR	&= ~(GPIO_OSPEEDR_OSPEED0 | GPIO_OSPEEDR_OSPEED1 | GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3);
	GPIOC->PUPDR	&= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1 | GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3);
	GPIOC->PUPDR	|=  (GPIO_PUPDR_PUPD0_1 | GPIO_PUPDR_PUPD1_1 | GPIO_PUPDR_PUPD2_1 | GPIO_PUPDR_PUPD3_1);
}

int8_t keypad_read()
{
	int8_t row_shorted = 0;
	int8_t col_shorted = 0;
	int8_t num_shorted = 0;

	// Read all rows for a 0
	if (GPIOC->IDR == 0)
	{
		return NO_PRESS;
	}

	// Loop through the columns
	for (int8_t cur_col=0; cur_col<NUM_COLS; cur_col++)
	{
		// Alternate assertions for COL pins by first clearing them...
		GPIOB->ODR &= ~(COL_MASK);

		// ... Then shift by the number of iterated columns
		GPIOB->ODR |= GPIO_ODR_OD12 << cur_col;

		for (int8_t delay=0; delay<5; delay++);

		// Loop through the rows
		for (int8_t cur_row=0; cur_row<NUM_ROWS; cur_row++)
		{
			// Read the ROW pins, shifted by the number of iterated rows
			int8_t is_shorted = GPIOC->IDR >> cur_row;

			// Check if the row was shorted
			if (is_shorted)
			{
				col_shorted = cur_col;
				row_shorted = cur_row;
				num_shorted++;
			}
		}
	}

	// Set COLs to 1
	GPIOB->ODR |= (COL_MASK);

	// Check if there wasn't a short
	if (num_shorted == 0)
	{
		return NO_PRESS;
	}

	// Return the Button pressed using the positions of the shorts
	return BUTTONS[row_shorted][col_shorted];
}
