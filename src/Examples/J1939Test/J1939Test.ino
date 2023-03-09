#include <esp32_can.h>

/* Include Open SAE J1939 */
#include <Open_SAE_J1939.h>

/* Create our J1939 structure */
J1939 j1939 = {0};

void setup() {
  delay(3000);
  Serial.begin(115200);
  CAN1.enable();
  CAN1.begin(500000);

  Serial.println("Started, initialized CAN layer");

  //these two lines associate a bus with the J1939 driver and set up filters
  //sufficient for it to grab only extended traffic while allowing standard frames
  //to flow elsewhere
  CAN_Set_Bus(&CAN1);
  CAN_Setup_Filter();

  /* Load your ECU information */
  //this is likely to crash because we don't actually store this info
  //Open_SAE_J1939_Startup_ECU(&j1939);

  /* Request Software Identification from ECU 2 to ECU 1 */
  SAE_J1939_Send_Request(&j1939, 0xA2, PGN_SOFTWARE_IDENTIFICATION);
  Serial.println("Requested software ID from other side");
} 

void loop()
{
    //If there is a new message then display it
    if (Open_SAE_J1939_Listen_For_Messages(&j1939))
    {
        Serial.printf("ID: %x Data: %02x %02x %02x %02x %02x %02x %02x %02x\n", j1939.ID, j1939.data[0], j1939.data[1], j1939.data[2],
            j1939.data[3], j1939.data[4], j1939.data[5], j1939.data[6], j1939.data[7]);
    }
}
