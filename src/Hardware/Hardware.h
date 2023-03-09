/*
 * Hardware.h
 *
 *  Created on: 6 nov. 2021
 *      Author: Daniel Mårtensson
 */

#ifndef HARDWARE_HARDWARE_H_
#define HARDWARE_HARDWARE_H_

/* Select your processor choice here */
#define DUE_CAN 1
#define ESP32_CAN 2
#define TEENSY_T4 3
#define INTERNAL_CALLBACK 6
#define PROCESSOR_CHOICE ESP32_CAN

/* C Standard library */
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#if PROCESSOR_CHOICE == ESP32_CAN || PROCESSOR_CHOICE == DUE_CAN
#include <can_common.h>
#endif

/* Enums */
#include "../SAE_J1939/SAE_J1939_Enums/Enum_DM14_DM15.h"
#include "../SAE_J1939/SAE_J1939_Enums/Enum_Send_Status.h"

#ifdef __cplusplus
extern "C" {
#endif

ENUM_J1939_STATUS_CODES CAN_Send_Message(uint32_t ID, uint8_t data[]);
ENUM_J1939_STATUS_CODES CAN_Send_Request(uint32_t ID, uint8_t PGN[]);
bool CAN_Read_Message(uint32_t *ID, uint8_t data[]);
void CAN_Delay(uint8_t milliseconds);
void CAN_Set_Callback_Functions(void (*Callback_Function_Send_)(uint32_t, uint8_t, uint8_t[]), void (*Callback_Function_Read_)(uint32_t*, uint8_t[], bool*));
void FLASH_EEPROM_RAM_Memory(uint16_t *number_of_requested_bytes, uint8_t pointer_type, uint8_t *command, uint32_t *pointer, uint8_t *pointer_extension, uint16_t *key, uint8_t raw_binary_data[]);
bool Save_Struct(uint8_t data[], uint32_t data_length, char file_name[]);
bool Load_Struct(uint8_t data[], uint32_t data_length, char file_name[]);

#if PROCESSOR_CHOICE == ESP32_CAN || PROCESSOR_CHOICE == DUE_CAN
    void CAN_Set_Bus(CAN_COMMON *which);
    void CAN_Setup_Filter();
#endif

#if PROCESSOR_CHOICE == TEENSY_T4
    void CAN_Set_Bus(FlexCAN_T4_Base *which);
    void CAN_Setup_Filter();
#endif


#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_HARDWARE_H_ */
