/*
 * Save_Load_Struct.c
 *
 *  Created on: 22 sep. 2021
 *      Author: Daniel Mårtensson
 */

#include "Hardware.h"

/* Layers */
#include <stdio.h>

bool Save_Struct(uint8_t data[], uint32_t data_length, char file_name[]){
#if (PROCESSOR_CHOICE == DUE_CAN) || (PROCESSOR_CHOICE == ESP32_CAN)
    //nothing yet
#else
	/* Write a file */
	FILE *file = NULL;
	file = fopen(file_name, "wb");
	fwrite(data, 1, data_length, file);
	fclose(file);
	return true;
#endif
}

bool Load_Struct(uint8_t data[], uint32_t data_length, char file_name[]){
#if (PROCESSOR_CHOICE == DUE_CAN) || (PROCESSOR_CHOICE == ESP32_CAN)
    //nothing yet    
#else
	/* Read a file */
	FILE *file = NULL;
	file = fopen(file_name, "rb");
	if(file == NULL)
		file = fopen(file_name, "wb");
	fread(data, 1, data_length, file);
	fclose(file);
	return true;
#endif
}
