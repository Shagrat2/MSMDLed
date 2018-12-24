#define MY_SIGNING_ATSHA204_PIN  17 // A3 - 26, PC3 (Default)
#define MY_RF24_CE_PIN 8 // 12, PB0
#define MY_OTA_FLASH_SS 7 // 11, PD7

#define BUTTON   2  // номер вывода кнопки 1 равен 12
#define CH1_PIN  6
#define CH2_PIN  5
#define CH3_PIN  9

//#define M25P40 // Flash type

// No change
#define MY_DEBUG

#define MY_OTA_FIRMWARE_FEATURE

#define MY_RADIO_NRF24
#define MY_CORE_ONLY
#define MY_SIGNING_ATSHA204

#ifdef M25P40
  #define MY_OTA_FLASH_JDECID 0x2020
#endif

#include <SPI.h>
#include <MySensors.h>

#ifdef M25P40
  #define SPIFLASH_BLOCKERASE_32K   0xD8
#endif

bool testSha204()
{
  uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
  uint8_t ret_code;
  if (Serial) {
    Serial.print("SHA204: ");
  }
  atsha204_init(MY_SIGNING_ATSHA204_PIN);
  ret_code = atsha204_wakeup(rx_buffer);

  if (ret_code == SHA204_SUCCESS) {
    ret_code = atsha204_getSerialNumber(rx_buffer);
    if (ret_code != SHA204_SUCCESS) {
      if (Serial) {
        Serial.println(F("Failed to obtain device serial number. Response: "));
      }
      Serial.println(ret_code, HEX);
    } else {
      if (Serial) {
        Serial.print(F("OK (serial : "));
        for (int i = 0; i < 9; i++) {
          if (rx_buffer[i] < 0x10) {
            Serial.print('0'); // Because Serial.print does not 0-pad HEX
          }
          Serial.print(rx_buffer[i], HEX);
        }
        Serial.println(")");
      }
      return true;
    }
  } else {
    if (Serial) {
      Serial.println(F("Failed to wakeup SHA204"));
    }
  }
  return false;
} 

void setup() {
   Serial.begin(115200);

  Serial.println(F(MYSENSORS_LIBRARY_VERSION "\n")); 

  // CPU
  Serial.println("Test CPU: OK");

  // NRF24/RFM69
  if (transportInit()) { // transportSanityCheck
    Serial.println("Radio: OK");
  } else {
    Serial.println("Radio: ERROR");
  }

  // Flash
  if (_flash.initialize()) {
    Serial.println("Flash: OK");
  } else {    
    Serial.println("Flash: ERROR ");
  }

  // ATASHA
  testSha204();

  // LED+button
  Serial.println("Key: Watch tester LED");

  pinMode(BUTTON, INPUT);
  pinMode(CH1_PIN, OUTPUT);
  pinMode(CH2_PIN, OUTPUT);
  pinMode(CH3_PIN, OUTPUT);

  int state = 0;
  int buttonState = 0; 
  while (true) {
    buttonState = digitalRead(BUTTON);
    Serial.print("Connect: ");
    Serial.println(buttonState);
    
    switch (state) {
    case 0:
      digitalWrite(CH1_PIN, HIGH);
      digitalWrite(CH2_PIN, LOW);
      digitalWrite(CH3_PIN, LOW);
      break;
     case 1:
      digitalWrite(CH1_PIN, LOW);
      digitalWrite(CH2_PIN, HIGH);
      digitalWrite(CH3_PIN, LOW);
      break;
     case 2:
      digitalWrite(CH1_PIN, LOW);
      digitalWrite(CH2_PIN, LOW);
      digitalWrite(CH3_PIN, HIGH);
      break;
    }

    state++;
    if (state > 2) {
      state = 0;
    }
    
    delay(1000);    
  } 
}

void loop() {

}

