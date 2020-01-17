/*
 *	FIFO ringbuffer library by Marcel Meyer Garcia
 *	a very good explanation can be found at https://www.mikrocontroller.net/articles/FIFO

 MIT License

Copyright (c) 2019 Marcel Meyer Garcia

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#ifndef __FIFO_H_
#define __FIFO_H_

#include <stdint.h>

// define return values
#define FIFO_OK 0
#define FIFO_OVERFLOW 1

// declare FIFO structure
typedef struct FIFO_8{
	uint8_t* data; // pointer to array of data
	uint16_t read_index;
	uint16_t write_index;
	uint16_t length; // total size of FIFO
	uint16_t current_level; // current number of elements in FIFO
}FIFO_8;

// function declarations
void FIFO_8_init(FIFO_8* fifo, uint8_t* data_ptr, uint16_t length);
uint8_t FIFO_8_put(FIFO_8* fifo, uint8_t input_data);
uint8_t FIFO_8_get(FIFO_8* fifo, uint8_t* output_data);
uint16_t FIFO_8_get_num_elements(FIFO_8* fifo);
uint16_t FIFO_8_get_available_space(FIFO_8* fifo);

#endif /* FIFO_H_ */
