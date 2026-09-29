/*
 * keypad.c
 *
 *  Created on: Sep 30, 2024
 *      Author: jerem
 */
#include "main.h"
#include "keypad.h"

const int8_t KEYS[NUM_ROWS][NUM_COLS] =
{
		{ KEY_1, KEY_2, KEY_3, KEY_A },
		{ KEY_4, KEY_5, KEY_6, KEY_B },
		{ KEY_7, KEY_8, KEY_9, KEY_C },
		{ STAR , KEY_0, HASH , KEY_D }
};

void KEYPAD_Config()
{
	/* Initialize all configured peripherals */
	RCC->AHB2ENR	|= RCC_AHB2ENR_GPIOBEN;		// Enable GPIOB Clock	(COL Outputs)
	RCC->AHB2ENR	|= RCC_AHB2ENR_GPIOCEN;		// 		  GPIOC Clock	(ROW Inputs)

	/* Configure PB12, PB13, PB14, PB15 (COL outputs) */
	// output mode, push-pull, low speed, no pull up/down resistor
	COL_PORT->MODER		&= ~(GPIO_MODER_MODE12 |
							 GPIO_MODER_MODE13 |
							 GPIO_MODER_MODE14 |
							 GPIO_MODER_MODE15);
	COL_PORT->MODER		|=  (GPIO_MODER_MODE12_0 |
							 GPIO_MODER_MODE13_0 |
							 GPIO_MODER_MODE14_0 |
							 GPIO_MODER_MODE15_0);
	COL_PORT->OTYPER	&= ~(GPIO_OTYPER_OT12 |
							 GPIO_OTYPER_OT13 |
							 GPIO_OTYPER_OT14 |
							 GPIO_OTYPER_OT15);
	COL_PORT->OSPEEDR	&= ~(GPIO_OSPEEDR_OSPEED12 |
							 GPIO_OSPEEDR_OSPEED13 |
							 GPIO_OSPEEDR_OSPEED14 |
							 GPIO_OSPEEDR_OSPEED15);
	COL_PORT->PUPDR		&= ~(GPIO_PUPDR_PUPD12 |
							 GPIO_PUPDR_PUPD13 |
							 GPIO_PUPDR_PUPD14 |
							 GPIO_PUPDR_PUPD15);
	COL_PORT->ODR		&= ~(COL_MASK);
	COL_PORT->ODR		|=  (COL_MASK);		// Set COLs to 1

	/* Configure PC0, PC1, PC2, PC3 	(ROW inputs) */
	// input mode, push-pull, low speed, pull-down resistor connected
	ROW_PORT->MODER		&= ~(GPIO_MODER_MODE0 |
							 GPIO_MODER_MODE1 |
							 GPIO_MODER_MODE2 |
							 GPIO_MODER_MODE3);
	ROW_PORT->OTYPER	&= ~(GPIO_OTYPER_OT0 |
							 GPIO_OTYPER_OT1 |
							 GPIO_OTYPER_OT2 |
							 GPIO_OTYPER_OT3);
	ROW_PORT->OSPEEDR	&= ~(GPIO_OSPEEDR_OSPEED0 |
							 GPIO_OSPEEDR_OSPEED1 |
							 GPIO_OSPEEDR_OSPEED2 |
							 GPIO_OSPEEDR_OSPEED3);
	ROW_PORT->PUPDR		&= ~(GPIO_PUPDR_PUPD0 |
							 GPIO_PUPDR_PUPD1 |
							 GPIO_PUPDR_PUPD2 |
							 GPIO_PUPDR_PUPD3);
	ROW_PORT->PUPDR		|=  (GPIO_PUPDR_PUPD0_1 |
							 GPIO_PUPDR_PUPD1_1 |
							 GPIO_PUPDR_PUPD2_1 |
							 GPIO_PUPDR_PUPD3_1);
}

int8_t KEYPAD_Read()
{
	int8_t row_shorted = 0;
	int8_t col_shorted = 0;
	int8_t num_shorted = 0;

	// Read all rows for a 0
	if (ROW_PORT->IDR == 0)
	{
		return NO_PRESS;
	}

	// Loop through the columns
	for (int8_t cur_col=0; cur_col<NUM_COLS; cur_col++)
	{
		// Alternate assertions for COL pins by first clearing them...
		COL_PORT->ODR &= ~(COL_MASK);

		// ... Then shift by the number of iterated columns
		COL_PORT->ODR |= GPIO_ODR_OD12 << cur_col;

		for (int8_t delay=0; delay<5; delay++);

		// Loop through the rows
		for (int8_t cur_row=0; cur_row<NUM_ROWS; cur_row++)
		{
			// Read the ROW pins, shifted by the number of iterated rows
			int8_t is_shorted = ROW_PORT->IDR >> cur_row;

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
	COL_PORT->ODR |= (COL_MASK);

	// Check if there wasn't a short
	if (num_shorted == 0)
	{
		return NO_PRESS;
	}

	// Return the Button pressed using the positions of the shorts
	return KEYS[row_shorted][col_shorted];
}
