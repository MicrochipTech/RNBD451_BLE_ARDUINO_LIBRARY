/*
    (c) 2023 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include <Arduino.h>

#include "rnbd.h"
#include "rnbd_interface.h"

#define DEFAULT_BAUDRATE 115200
#define SERIAL_BAUDRATE 115200

bool Err;
bool initialize = false;
const char DevName[] = "RNBD451_CENTRAL";
uint8_t service_uuid = 0xC0;


typedef enum {
  /* TODO: Define states used by the application state machine. */
  RNBD_INIT,
  RNBD_FACTORY_RESET,
  RNBD_CMD,
  RNBD_CMD1,
  RNBD_CMD2,
  RNBD_CMD3,
  RNBD_SET_NAME,
  RNBD_SET_PROFILE,
  RNBD_SET_APPEARANCE,
  RNBD_BLE_SCAN_AND_CONNECT,
  RNBD_REBOOT,
  RNBD_REBOOT1,
  RNBD_SERVICE_UUID,
  RNBD_SERVICE_CHARACTERISTICS,
  RNBD_SERVICE_CHARACTERISTICS1,
  RNBD_SERVICE_CHARACTERISTICS2,
  RNBD_SEND_HR_DATA,
  RNBD_WAIT,

} STATES;

typedef struct
{
  /* The application's current state */
  STATES state;

} RNBD_STATE;

RNBD_STATE rnbd_state;



void setup() {
  pinMode(RESET_PIN, OUTPUT);
  // initialize both serial ports:
  Serial.begin(SERIAL_BAUDRATE);       //Arduino UART Serial
  RNBDserial.begin(DEFAULT_BAUDRATE);  //RNBD UART Serial
  delay(1000);
  RNBDserial.setTimeout(2000);
  Serial.println("RNBD451 CENTRAL");
  rnbd_state.state = RNBD_INIT;
  initialize = true;
}

void loop() {

  if (initialize == true) 
  {
    RNBD_DP_INIT();
  } 
  else 
  {
    serial_transfer();
  }
}

void serial_transfer() {

  // read from RNBD451 and Print on Arduino Zero
  if (RNBDserial.available()) {
    String BU_data = RNBDserial.readString();
    Serial.println(BU_data);
  }

  // read from Arduino Zero and Print on RNBD451
  if (Serial.available()) {
    String AR_data = Serial.readString();
    RNBDserial.print(AR_data);
  }
}

void RNBD_DP_INIT() {

  switch (rnbd_state.state) {
    case RNBD_INIT:
      {
        Err = RNBD_Init();
        if (Err) {
          Err = false;
          Serial.println("RNBD451_INITIALIZING");
          rnbd_state.state = RNBD_CMD;
        }
      }
      break;
    case RNBD_CMD:
      {
        Err = RNBD_EnterCmdMode();
        if (Err) {
          Err = false;
          Serial.println("Entered CMD Mode");
          rnbd_state.state = RNBD_FACTORY_RESET;
        }
      }
      break;
    case RNBD_FACTORY_RESET:
      {
        Err = RNBD_FactoryReset();
        RNBD.DelayMs(1000);
        if (Err) {
          Err = false;
          Serial.println("Factory Reset Done");
          rnbd_state.state = RNBD_CMD1;
        }
      }
      break;
    case RNBD_CMD1:
      {
        Err = RNBD_EnterCmdMode();
        if (Err) {
          Err = false;
          Serial.println("Entered CMD Mode");
          rnbd_state.state = RNBD_SET_NAME;
        }
      }
      break;
    case RNBD_SET_NAME:
      {
        Err = RNBD_SetName(DevName, strlen(DevName));
        if (Err) {
          Err = false;
          Serial.println("Device Name Set");
          rnbd_state.state = RNBD_SET_PROFILE;
        }
      }
      break;
    case RNBD_SET_PROFILE:
      {
        Err = RNBD_SetServiceBitmap(service_uuid);
        if (Err) {
          Err = false;
          Serial.println("Service Bitmap Set");
          rnbd_state.state = RNBD_REBOOT;
        }
      }
      break;
    case RNBD_REBOOT:
      {
        Err = RNBD_RebootCmd();
        RNBD.DelayMs(1500);
        if (Err) {
          Err = false;
          Serial.println("Reboot Completed");
          rnbd_state.state = RNBD_CMD2;
        }
      }
      break;
    case RNBD_CMD2:
      {
        Err = RNBD_EnterCmdMode();
        if (Err) {
          Err = false;
          Serial.println("Entered CMD Mode");
          rnbd_state.state = RNBD_BLE_SCAN_AND_CONNECT;
        }
      }
      break;
    case RNBD_BLE_SCAN_AND_CONNECT:
      {
        Err = RNBD_StartScanning();
        if (Err) {
          Err = false;
          Serial.println("Scanning...");
        }
        RNBD.DelayMs(200);
        Err = RNBD_StopScanning();
        if (Err) {
          Err = false;
          Serial.println("Stopped Scanning...");
        }

        while (RNBDserial.available() == 0) {}       //wait for data available
        String scan_data = RNBDserial.readString();  //read until timeout
        Serial.println("!!! SCAN DATA!!!");
        Serial.println(scan_data);
        RNBD.DelayMs(300);

        //Serial.println(scan_data.indexOf("FEDA"));
        int feda_pos = scan_data.indexOf("FEDA");
        if (feda_pos != -1) 
        {
          //Serial.println(scan_data.lastIndexOf('%',feda_pos));
          int addr_start_pos = scan_data.lastIndexOf('%', feda_pos);
          //Serial.println(scan_data.indexOf('%',feda_pos));
          int addr_end_pos = scan_data.indexOf('%', feda_pos);

          String peri_data = scan_data.substring(addr_start_pos, addr_end_pos);
          //Serial.println(peri_data);
          String peri_addr = peri_data.substring(1, 13);  //For getting MAC Addrerss
          //Serial.println(peri_addr);

          //Serial.println(peri_data.indexOf(',',17));
          int name_end_pos = peri_data.indexOf(',', 17);
          String peri_name = peri_data.substring(17, name_end_pos);  //For getting Device Name
          Serial.println("Connecting to Device Name: " + peri_name + ";  MAC Address: " + peri_addr);

          int connection_mac_len = peri_addr.length() + 1;
          char connection_mac[connection_mac_len];
          peri_addr.toCharArray(connection_mac, connection_mac_len);
          Err = RNBD_BLEConnect(connection_mac, connection_mac_len);

          //Serial1.write(connection_cmd);
          delay(3000);

          if (Err) {
            Err = false;
            initialize = false;
            //serialFlush();
            rnbd_state.state = RNBD_WAIT;
            Serial.println("!!! Device Connected !!!");
          }

        }
        else 
        {
          rnbd_state.state = RNBD_BLE_SCAN_AND_CONNECT;
          initialize = true;
          Serial.println("!!! Device Not Connected !!!");
        }
      }
      break;
      case RNBD_WAIT:
      {
        //DO NOTHING
      }
      break;
  }
}

void serialFlush() {
  while (RNBDserial.available() > 0) {
    char rnbd_temp = RNBDserial.read();
  }
  while (Serial.available() > 0) {
    char serial_temp = Serial.read();
  }
}