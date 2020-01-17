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
#include "fifo.h"

// initialize the FIFO buffer
// argument1: pointer to FIFO_8 struct
// argument2: pointer to uint8_t array that contains the FIFO's data
// argument3: size of array (number of elements in FIFO buffer)
/* EXAMPLE:
 *
 * uint8_t data[100];
 * FIFO_8 my_fifo;
 * FIFO_8_init(&my_fifo, data, 100);
 *
 * */
void FIFO_8_init(FIFO_8* fifo, uint8_t* data_ptr, uint16_t length){
	fifo->data = data_ptr;
	fifo->length = length;
	fifo->current_level = 0;
	fifo->read_index = 0;
	fifo->write_index = 0;
}

// put one element in the FIFO buffer
// argument1: pointer to FIFO
// argument2: data to be written to FIFO
// return value: status, FIFO_OK or FIFO_OVERFLOW if the buffer is already full
uint8_t FIFO_8_put(FIFO_8* fifo, uint8_t input_data){
	// check for FIFO buffer overflow
	if( (fifo->write_index + 1 == fifo->read_index) ||
		(fifo->read_index == 0 && fifo->write_index + 1 == fifo->length) ){
		return FIFO_OVERFLOW;
	}
	// put new element in, increment write index
	fifo->data[fifo->write_index] = input_data;
	++(fifo->write_index);
	// wrap around
	if( fifo->write_index >= fifo->length ){
		fifo->write_index = 0;
	}
	// increment number of elements currently in FIFO buffer
	++(fifo->current_level);
	return FIFO_OK;
}

// retrieve one element from the FIFO buffer
// put one element in the FIFO buffer
// argument1: pointer to FIFO
// argument2: pointer to variable where to store the value read from the FIFO buffer
// return value: status, FIFO_OK or FIFO_OVERFLOW
uint8_t FIFO_8_get(FIFO_8* fifo, uint8_t* output_data){
	// check for FIFO buffer overflow
	if( fifo->read_index == fifo->write_index ){
		*output_data = 0;
		return FIFO_OVERFLOW;
	}
	// retrieve element from FIFO buffer, increment read index
	*output_data = fifo->data[fifo->read_index];
	++(fifo->read_index);
	// wrap around
	if( fifo->read_index >= fifo->length ){
		fifo->read_index = 0;
	}
	// decrement number of elements currently in FIFO
	--(fifo->current_level);
	return FIFO_OK;
}

// get number of elements currently inside the FIFO buffer
uint16_t FIFO_8_get_num_elements(FIFO_8* fifo){
	return fifo->current_level;
}
// get number of free spaces for elements inside the FIFO buffer
uint16_t FIFO_8_get_available_space(FIFO_8* fifo){
	return (fifo->length - fifo->current_level);
}
