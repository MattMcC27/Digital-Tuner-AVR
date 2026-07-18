/*
 * font.h
 *
 * Created: 5/1/2026 7:16:07 PM
 *  Author: matth
 */ 


#ifndef FONT_H_
#define FONT_H_



#include <stdint.h>
#define FONT_HIGHT 16
#define FONT_WIDTH 12 //in pixels
#define FONT_CHAR_COUNT 22 //total chars
#define FONT_CHAR_SIZE 24 //bytes of 1 char

extern const uint8_t* font_table[FONT_CHAR_COUNT];

const uint8_t* font_num(int n);

const uint8_t* font_alph(char c);


#endif /* FONT_H_ */