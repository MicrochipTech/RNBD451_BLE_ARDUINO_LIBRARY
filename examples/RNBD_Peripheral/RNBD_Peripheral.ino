/*
© 2025 Microchip Technology Inc. and its subsidiaries. All rights reserved.

Subject to your compliance with these terms, you may use this Microchip software and any derivatives 
exclusively with Microchip products. You are responsible for complying with third party license terms 
applicable to your use of third party software (including open source software) that may accompany this 
Microchip software. SOFTWARE IS “AS IS.” NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR 
STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,
MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL 
MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, 
DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER 
CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP’S TOTAL LIABILITY ON ALL 
CLAIMS RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY 
TO MICROCHIP FOR THIS SOFTWARE
*/

#include <Arduino.h>
#include "rnbd.h"
#include "rnbd_interface.h"

#define DEFAULT_BAUDRATE 115200
#define SERIAL_BAUDRATE 115200
#define BLEserial Serial1
#define USBserial Serial
#define RST_PIN A3

bool Err;
bool initialize = false;
const char DevName[] = "RNBD451_PERIPHERAL";
uint8_t service_uuid = 0xC0;

typedef enum {
  /* TODO: Define states used by the application state machine. */
  RNBD_INIT,
  RNBD_FACTORY_RESET,
  RNBD_CMD,
  RNBD_CMD1,
  RNBD_SET_NAME,
  RNBD_SET_PROFILE,
  RNBD_REBOOT,  
} STATES;

typedef struct
{
  /* The application's current state */
  STATES state;

} RNBD_STATE;

RNBD_STATE rnbd_state;
BLE BLE_RNBD;

void setup() {
  BLE_RNBD.setReset(RST_PIN);
  BLE_RNBD.initBleStream(&BLEserial);
  //Arduino UART Serial
  USBserial.begin(SERIAL_BAUDRATE);  
  //RNBD UART Serial
  BLEserial.begin(DEFAULT_BAUDRATE);  
  delay(1000);
  USBserial.println("RNBD451 PERIPHERAL");
  rnbd_state.state = RNBD_INIT;
  initialize = true;
}

void loop() {

  if (initialize == true) {
    RNBD_PERIPHERAL();
  } else {
    serial_transfer();
  }
}

void serial_transfer() {

  // read from RNBD451 and Print on Arduino Zero
  if (BLEserial.available()) {
    String BU_data = BLEserial.readString();
    USBserial.println(BU_data);
  }

  // read from Arduino Zero and Print on RNBD451
  if (USBserial.available()) {
    String AR_data = USBserial.readString();
    BLEserial.print(AR_data);
  }
}

void RNBD_PERIPHERAL() {

  switch (rnbd_state.state) {
    case RNBD_INIT:
      {
        Err = BLE_RNBD.RNBD_Init();
        if (Err) {
          Err = false;
          USBserial.println("RNBD451_INITIALIZING");
          rnbd_state.state = RNBD_CMD;
        }
      }
      break;
    case RNBD_CMD:
      {
        Err = BLE_RNBD.RNBD_EnterCmdMode();
        if (Err) {
          Err = false;
          USBserial.println("Entered CMD Mode");
          rnbd_state.state = RNBD_FACTORY_RESET;
        }
      }
      break;
    case RNBD_FACTORY_RESET:
      {
        Err = BLE_RNBD.RNBD_FactoryReset();
        RNBD.DelayMs(1000);
        if (Err) {
          Err = false;
          USBserial.println("Factory Reset Done");
          rnbd_state.state = RNBD_CMD1;
        }
      }
      break;
    case RNBD_CMD1:
      {
        Err = BLE_RNBD.RNBD_EnterCmdMode();
        if (Err) {
          Err = false;
          USBserial.println("Entered CMD Mode");
          rnbd_state.state = RNBD_SET_NAME;
        }
      }
      break;
    case RNBD_SET_NAME:
      {
        Err = BLE_RNBD.RNBD_SetName(DevName, strlen(DevName));
        if (Err) {
          Err = false;
          USBserial.println("Device Name Set");
          rnbd_state.state = RNBD_SET_PROFILE;
        }
      }
      break;
    case RNBD_SET_PROFILE:
      {
        Err = BLE_RNBD.RNBD_SetServiceBitmap(service_uuid);
        if (Err) {
          Err = false;
          USBserial.println("Service Bitmap Set");
          rnbd_state.state = RNBD_REBOOT;
        }
      }
      break;
    case RNBD_REBOOT:
      {
        Err = BLE_RNBD.RNBD_RebootCmd();
        RNBD.DelayMs(1500);
        if (Err) {
          Err = false;
          USBserial.println("Reboot Completed");
          initialize = false;
          serialFlush();
          USBserial.println("!!! Started Advertising - Scan and Connect using MBD App !!!");
        }
      }
      break;    
  }  
}

void serialFlush(){
  while(BLEserial.available() > 0) {
    char rnbd_temp = BLEserial.read();
  }
  while(USBserial.available() > 0) {
    char serial_temp = USBserial.read();
  }
}