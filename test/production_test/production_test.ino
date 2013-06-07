
#include <Wire.h>

//#define TARGET1  0x41
#define TARGET  0x7f
#define DEL      250

void setup (void) {
  Serial.begin(57600);
  Wire.begin();
  Serial.println("starting!!");
}

void loop (void) {
  static uint8_t bytes[24] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  static uint8_t test0[24] = {0xff,0,0xff,0,0xff,0,0xff,0,0,0xff,0,0xff,0,0xff,0,0xff,0xff,0,0xff,0,0xff,0,0xff,0};
  static uint8_t test1[24] = {0,0xff,0,0xff,0,0xff,0,0xff,0xff,0,0xff,0,0xff,0,0xff,0,0,0xff,0,0xff,0,0xff,0,0xff};
  static uint8_t test2[24] = {0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  static uint8_t test3[24] = {0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  
  Wire.beginTransmission(TARGET);
  Wire.write(test0, 24);
  Serial.println(Wire.endTransmission());
  delay(DEL);
  Wire.beginTransmission(TARGET);
  Wire.write(test1, 24);
  Serial.println(Wire.endTransmission());
  delay(DEL);
  Wire.beginTransmission(TARGET);
  Wire.write(test2, 24);
  Serial.println(Wire.endTransmission());
  delay(DEL);
  Wire.beginTransmission(TARGET);
  Wire.write(test3, 24);
  Serial.println(Wire.endTransmission());
  delay(DEL);

}
