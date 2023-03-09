/*
 * CAN_Transmit_Receive.c
 *
 *  Created on: 11 juli 2021
 *      Author: Daniel Mårtensson
 */

/* Layer */
#include "Hardware.h"

/* This is a call back function e.g listener, that will be called once SAE J1939 data is going to be sent */
static void (*Callback_Function_Send)(uint32_t, uint8_t, uint8_t[]);
static void (*Callback_Function_Read)(uint32_t*, uint8_t[], bool*);

/* Platform independent library headers for CAN */
#if PROCESSOR_CHOICE == DUE_CAN
    #include <due_can.h>
#elif PROCESSOR_CHOICE == ESP32_CAN
    #include <esp32_can.h>
#elif PROCESSOR_CHOICE == TEENSY_T4
    #include <FlexCAN_T4.h>
#elif PROCESSOR_CHOICE == INTERNAL_CALLBACK
/* Nothing here because else statement should not be running */
#else
/* Internal fields */
static bool internal_new_message[256] = {false};
static uint8_t internal_data[256*8] = {0};
static uint8_t internal_DLC[256] = {0};
static uint32_t internal_ID[256] = {0};
static uint8_t buffer_index_transmit = 0;
static uint8_t buffer_index_receive = 0;

/* Internal functions */
static ENUM_J1939_STATUS_CODES Internal_Transmit(uint32_t ID, uint8_t data[], uint8_t DLC) {
	internal_ID[buffer_index_transmit] = ID;
	internal_DLC[buffer_index_transmit] = DLC;
	for(uint8_t i = 0; i < 8; i++)
		if(i < DLC)
			internal_data[buffer_index_transmit*8 + i] = data[i];
		else
			internal_data[buffer_index_transmit*8 + i] = 0x0;
	internal_new_message[buffer_index_transmit] = true;
	buffer_index_transmit++;									/* When this is 256, then it will be come 0 again */
	return STATUS_SEND_OK;
}

static void Internal_Receive(uint32_t *ID, uint8_t data[], bool *is_new_message) {
	/* Do a quick check if we are going to read message that have no data */
	if(internal_new_message[buffer_index_receive] == false){
		*is_new_message = false;
		return;
	}

	*ID = internal_ID[buffer_index_receive];
	for(uint8_t i = 0; i < 8; i++)
		if(i < internal_DLC[buffer_index_receive])
			data[i] = internal_data[buffer_index_receive*8 + i];
	*is_new_message = internal_new_message[buffer_index_receive];
	/* Reset */
	internal_new_message[buffer_index_receive] = false;
	internal_DLC[buffer_index_receive] = 0;
	buffer_index_receive++;										/* When this is 256, then it will be come 0 again */
}
#endif

//both use can_common so the exact same code will work for both.
#if (PROCESSOR_CHOICE == DUE_CAN) || (PROCESSOR_CHOICE == ESP32_CAN)
    static CAN_COMMON *canBus = &CAN0;
    CAN_FRAME extFrames[32];
    uint32_t read_idx = 0;
    uint32_t write_idx = 0;

    void CAN_Set_Bus(CAN_COMMON *which)
    {
        canBus = which;
    }

    void gotExtFrame (CAN_FRAME *frame)
    {
        extFrames[write_idx] = *frame;
        write_idx = (write_idx + 1) & 31;
    }

    void CAN_Setup_Filter()
    {
                                //MB  ID MSK EXT
        canBus->_setFilterSpecific(0, 0, 0, true); //watch for any extended frame
        canBus->_setFilterSpecific(1, 0, 0, false); //watch for any standard frame
        canBus->setCallback(0, gotExtFrame); //auto callback function on ext frames
    }
#endif

#if (PROCESSOR_CHOICE == TEENSY_T4)
    FlexCAN_T4_Base* canBus = _CAN0;
    CAN_message_t extFrames[32];
    uint32_t read_idx = 0;
    uint32_t write_idx = 0;

    void CAN_Set_Bus(FlexCAN_T4_Base *which)
    {
        canBus = which;
    }

    void gotExtFrame (const CAN_message_t &frame)
    {
        extFrames[write_idx] = frame;
        write_idx = (write_idx + 1) & 31;
    }

    void CAN_Setup_Filter()
    {
        FlexCAN_T4* can = dynamic_cast<FlexCAN_T4 *>(canBus);
        if (!can) return;
        can->setMB((FLEXCAN_MAILBOX)0,RX,EXT);
        can->setMBFilter(REJECT_ALL);
        can->enableMBInterrupts();
        can->onReceive(MB0, gotExtFrame);
        can->setMBUserFilter(MB0,0x00,0x00); //hopefully let all extended frames through into this MB
        can->mailboxStatus();
    }
#endif

ENUM_J1939_STATUS_CODES CAN_Send_Message(uint32_t ID, uint8_t data[]) {
	ENUM_J1939_STATUS_CODES status = STATUS_SEND_BUSY;
    #if (PROCESSOR_CHOICE == DUE_CAN) || (PROCESSOR_CHOICE == ESP32_CAN)
        CAN_FRAME frame;
        frame.length = 8;
        frame.rtr = 0;
        frame.extended = 1;
        frame.id = ID;
        memcpy(&frame.data.value, data, 8);
        canBus->sendFrame(frame);
        status = STATUS_SEND_OK;
    #elif PROCESSOR_CHOICE == TEENSY_T4
        CAN_message_t frame;
        frame.len = 8;
        frame.flags.extended = true;
        frame.id = ID;
        memcpy(frame.buf, data, 8);
        canBus->write(frame);
        status = STATUS_SEND_OK;
	#elif PROCESSOR_CHOICE == INTERNAL_CALLBACK
    /* Call our callback function */
    Callback_Function_Send(ID, 8, data);
    status = STATUS_SEND_OK;
	#else
	/* If no processor are used, use internal feedback for debugging */
	status = Internal_Transmit(ID, data, 8);
	#endif
	return status;
}

/* Send a PGN request
 * PGN: 0x00EA00 (59904)
 */
ENUM_J1939_STATUS_CODES CAN_Send_Request(uint32_t ID, uint8_t PGN[]) {
	ENUM_J1939_STATUS_CODES status = STATUS_SEND_BUSY;
    #if (PROCESSOR_CHOICE == DUE_CAN) || (PROCESSOR_CHOICE == ESP32_CAN)
        CAN_FRAME frame;
        frame.length = 3;
        frame.rtr = 0;
        frame.extended = 1;
        frame.id = ID;
        memcpy(&frame.data.value, PGN, 3);
        canBus->sendFrame(frame);
        status = STATUS_SEND_OK;
    #elif PROCESSOR_CHOICE == TEENSY_T4
        CAN_message_t frame;
        frame.len = 3;
        frame.flags.extended = true;
        frame.id = ID;
        memcpy(frame.buf, data, 3);
        canBus->write(frame);
        status = STATUS_SEND_OK;
	#elif PROCESSOR_CHOICE == INTERNAL_CALLBACK
    /* Call our callback function */
    Callback_Function_Send(ID, 3, PGN);
    status = STATUS_SEND_OK;
	#else
	/* If no processor are used, use internal feedback for debugging */
	status = Internal_Transmit(ID, PGN, 3);
	#endif
	return status;
}

/* Read the current CAN-bus message. Returning false if the message has been read before, else true */
bool CAN_Read_Message(uint32_t *ID, uint8_t data[]) {
	bool is_new_message = false;
    #if (PROCESSOR_CHOICE == DUE_CAN) || (PROCESSOR_CHOICE == ESP32_CAN)
       if (read_idx == write_idx) return false;
       *ID = extFrames[read_idx].id;
       memcpy(data, &extFrames[read_idx].data.value, 8);
       read_idx = (read_idx + 1) & 31;
       return true;
	#elif PROCESSOR_CHOICE == INTERNAL_CALLBACK
    Callback_Function_Read(ID, data, &is_new_message);
	#else
	/* If no processor are used, use internal feedback for debugging */
	Internal_Receive(ID, data, &is_new_message);
	#endif
	return is_new_message;
}

void CAN_Set_Callback_Functions(void (*Callback_Function_Send_)(uint32_t, uint8_t, uint8_t[]), void (*Callback_Function_Read_)(uint32_t*, uint8_t[], bool*)){
	Callback_Function_Send = Callback_Function_Send_;
	Callback_Function_Read = Callback_Function_Read_;
}

void CAN_Delay(uint8_t milliseconds) {
	#if (PROCESSOR_CHOICE == DUE_CAN) || (PROCESSOR_CHOICE == ESP32_CAN) || (PROCESSOR_CHOICE == TEENSY_T4)
        delay(milliseconds);
	#elif PROCESSOR_CHOICE == INTERNAL_CALLBACK
	/* Storing start time */
	clock_t start_time = clock();

	/* looping till required time is not achieved */
	while (clock() < start_time + milliseconds) {
		;
	}
	#else
	/* Nothing */
	#endif
}
