#include <Arduino.h>

#include "rnbd.h"
#include "rnbd_interface.h"

#define DEFAULT_BAUDRATE 115200
#define SERIAL_BAUDRATE 115200

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



void setup() {
  pinMode(RESET_PIN, OUTPUT);
  // initialize both serial ports:
  Serial.begin(SERIAL_BAUDRATE);       //Arduino UART Serial
  RNBDserial.begin(DEFAULT_BAUDRATE);  //RNBD UART Serial
  delay(1000);
  Serial.println("RNBD451 PERIPHERAL");
  rnbd_state.state = RNBD_INIT;
  initialize = true;
}

void loop() {

  if (initialize == true) {
    RNBD_DP_INIT();
  } else {
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
          initialize = false;
          serialFlush();
          Serial.println("!!! Started Advertising !!!");
        }
      }
      break;    
  }  
}

void serialFlush(){
  while(RNBDserial.available() > 0) {
    char rnbd_temp = RNBDserial.read();
  }
  while(Serial.available() > 0) {
    char serial_temp = Serial.read();
  }
}