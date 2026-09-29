/*
 * keypad.h
 *
 *  Created on: Sep 30, 2024
 *      Author: jerem
 */
#include "main.h"

#ifndef SRC_KEYPAD_H_
#define SRC_KEYPAD_H_

// Button Dimension Definitions
#define NUM_ROWS 4
#define NUM_COLS 4

// Button-Value Definitions
#define NO_PRESS -1
#define KEY_0 0
#define KEY_1 1
#define KEY_2 2
#define KEY_3 3
#define KEY_4 4
#define KEY_5 5
#define KEY_6 6
#define KEY_7 7
#define KEY_8 8
#define KEY_9 9
#define KEY_A 10
#define KEY_B 11
#define KEY_C 12
#define KEY_D 13
#define STAR  14
#define HASH  15

// Initialized Value for COLs as Outputs
#define LED_MASK (GPIO_ODR_OD4 | GPIO_ODR_OD5 | GPIO_ODR_OD6 | GPIO_ODR_OD7)
#define COL_MASK (GPIO_ODR_OD12 | GPIO_ODR_OD13 | GPIO_ODR_OD14 | GPIO_ODR_OD15)

// Keypad Functions
void keypad_init();

int8_t keypad_read();

#endif /* SRC_KEYPAD_H_ */
