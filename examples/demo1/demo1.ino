
#include <Wire.h>

//#define TARGET1  0x41
#define TARGET  0x41
#define DEL      250

void setup (void) {
  Serial.begin(57600);
  Wire.begin();
  Serial.println("starting!!");
}

void loop (void) {
  static uint8_t bytes[24] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  static uint8_t test0[24] = {0,0,0x3c,0x3c,0x3c,0x3c,0,0,
                              0,0x7e,0x42,0x5a,0x5a,0x42,0x7e,0,
                              0xff,0x81,0x81,0x99,0x99,0x81,0x81,0xff};
  
  Wire.beginTransmission(TARGET);
  Wire.write(test0, 24);
  Serial.println(Wire.endTransmission());
  delay(DEL);

}
