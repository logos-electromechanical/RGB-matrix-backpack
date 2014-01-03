
// For use with 4 backpacks in a quad configuration

#include <WSWire.h>

// I2C addresses for the four backpacks
#define ADD_UR (0x41)
#define ADD_UL (0x42)
#define ADD_LL (0x43)
#define ADD_LR (0x44)

// define cosntants to describe the display
#define SIZE       (16)        // the number of pixels on each side
#define BUFSIZE    (24)        // number of bytes in each display buffer

// define constants for the game of life
#define DENSITY  (50)    // density of initial fill
#define FRAMES   (1)    // number of frames between generations
#define MAXGENS  (75)

// function prototypes
uint16_t live(uint8_t y, uint16_t world[][2]);
void redWorld2Buf (uint16_t world[SIZE][2]);
void grnWorld2Buf (uint16_t world[SIZE][2]);
void bluWorld2Buf (uint16_t world[SIZE][2]);
void initWorlds (void);
void writeOutBuffer(void);
void printWorld(uint16_t world[SIZE][2]);
void printBuffer(uint8_t buf[BUFSIZE]);

// define the world
uint16_t redWorld[SIZE][2];
uint16_t grnWorld[SIZE][2];
uint16_t bluWorld[SIZE][2];

// define the output buffers
uint8_t bufUL[BUFSIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t bufUR[BUFSIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t bufLL[BUFSIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t bufLR[BUFSIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void setup (void)
{
  uint8_t i, j;
  /* start up the 4x4 shield */
  Serial.begin(57600);
  Serial.println("I live!"); 
  
  /* start up the first PWM driver */
  Wire.begin();
  
  initWorlds();
    
  Serial.println("All set up!"); 
}

void loop (void) {
  static uint8_t column = 0;
  static uint8_t frameCnt = 0;
  static long genCnt = 0;
  uint8_t i,j,k;
  
  // update framebuffer
  frameCnt++;
  writeOutBuffer();
  delay(500);
  
  // update world
  if ((frameCnt > FRAMES)/* && Serial.available()*/) {
    //while (Serial.available()) Serial.read();
    frameCnt = 0;
    genCnt++;
    if (genCnt > MAXGENS) {
      initWorlds();
      genCnt = 0;
    }
    Serial.print("Starting generation ");
    Serial.println(genCnt);
    // step through all three worlds
    for (i = 0; i < SIZE; i++) {
      redWorld[i][1] = live(i,redWorld);
      grnWorld[i][1] = live(i,grnWorld);
      bluWorld[i][1] = live(i,bluWorld);
    }
    // copy new world over old
    for (i = 0; i < SIZE; i++) {
      redWorld[i][0] = redWorld[i][1];
      grnWorld[i][0] = grnWorld[i][1];
      bluWorld[i][0] = bluWorld[i][1];
    }
    // check that we did it right
    printWorld(redWorld);
    printWorld(grnWorld);
    printWorld(bluWorld);
    // send each world to its corresponding buffer
    redWorld2Buf(redWorld);
    grnWorld2Buf(grnWorld);
    bluWorld2Buf(bluWorld);
    //printBuffer(bufUL);
    //printBuffer(bufUR);
    //printBuffer(bufLL);
    //printBuffer(bufLR);
  }
}


void initWorlds (void) {
  randomSeed(analogRead(0));
  for (int i = 0; i < SIZE; i++) {
    redWorld[i][0] = 0;
    grnWorld[i][0] = 0;
    bluWorld[i][0] = 0;
    redWorld[i][1] = 0;
    grnWorld[i][1] = 0;
    bluWorld[i][1] = 0;
    for (int j = 0; j < SIZE; j++) {
      if (random(100) < DENSITY) {
        redWorld[i][0] |= _BV(j);
        //Serial.print(".");
      }
      else {
        //Serial.print(" ");
      }
      if (random(100) < DENSITY) {
        grnWorld[i][0] |= _BV(j);
        //Serial.print(",");
      }
      else {
        Serial.print(" ");
      }
      if (random(100) < DENSITY) {
        bluWorld[i][0] |= _BV(j);
        //Serial.print(";");
      }
      else {
        //Serial.print(" ");
      }
    }
    //Serial.println("");
  }
  // send each world to its corresponding buffer
  redWorld2Buf(redWorld);
  grnWorld2Buf(grnWorld);
  bluWorld2Buf(bluWorld);
  printWorld(redWorld);
  printWorld(grnWorld);
  printWorld(bluWorld);
  //printBuffer(bufUL);
  //printBuffer(bufUR);
  //printBuffer(bufLL);
  //printBuffer(bufLR);
}

void redWorld2Buf (uint16_t world[SIZE][2]) {
  int i;
  Serial.println("Writing red world to buffer");
  for (i = 0; i < SIZE; i++) {
    if (i < 8) {
      bufLL[7-i] = (uint8_t)((world[i][0] & 0xff00) >> 8);
      bufLR[7-i] = (uint8_t)(world[i][0] & 0xff);
    } else {
      bufUL[15-i] = (uint8_t)((world[i][0] & 0xff00) >> 8);
      bufUR[15-i] = (uint8_t)(world[i][0] & 0xff);
    }
  }
}

void grnWorld2Buf (uint16_t world[SIZE][2]) {
  int i, j;
  Serial.println("Writing green world to buffer");
  for (i = 0; i < SIZE; i++) {
    if (i < 8) {
      bufLL[(i)+8] = (uint8_t)(world[i][0] >> 8);
      bufLR[(i)+8] = (uint8_t)(world[i][0]);
    } else {
      bufUL[(i-8)+8] = (uint8_t)(world[i][0] >> 8);
      bufUR[(i-8)+8] = (uint8_t)(world[i][0]);
    }
  }
}

void bluWorld2Buf (uint16_t world[SIZE][2]) {
  int i, j;
  Serial.println("Writing blue world to buffer");
  for (i = 0; i < SIZE; i++) {
    if (i < 8) {
      bufLL[(7-i)+16] = (uint8_t)(world[i][0] >> 8);
      bufLR[(7-i)+16] = (uint8_t)(world[i][0]);
    } else {
      bufUL[(15-i)+16] = (uint8_t)(world[i][0] >> 8);
      bufUR[(15-i)+16] = (uint8_t)(world[i][0]);
    }
  }
}

uint16_t live(uint8_t y, uint16_t world[][2]) {
  uint8_t x, count; 
  uint16_t up, down, next, last, mark, ret;
  mark = 1;
  ret = 0;
  if (y == 0) {
    up = 1;
    down = 15;
  } else if (y==15) {
    up = 0;
    down = 14;
  } else {
    up = y+1;
    down = y-1;
  }
  for (x = 0; x < 16; x++) {
    count = 0;
    if (x == 0) {
      next = 1;
      last = 15;
    } else if (x==15) {
      next = 0;
      last = 14;
    } else {
      next = x+1;
      last = x-1;
    }
    if (world[y][0] & (uint16_t)(mark << next)) count++;
    if (world[y][0] & (uint16_t)(mark << last)) count++;
    if (world[up][0] & (uint16_t)(mark << next)) count++;
    if (world[up][0] & (uint16_t)(mark << x)) count++;
    if (world[up][0] & (uint16_t)(mark << last)) count++;
    if (world[down][0] & (uint16_t)(mark << next)) count++;
    if (world[down][0] & (uint16_t)(mark << x)) count++;
    if (world[down][0] & (uint16_t)(mark << last)) count++;
    if ((count == 3) || ((count == 2) && (world[y][0] & (mark << x)))) ret |= (mark << x);
  }
  return ret;
}


void printWorld(uint16_t world[SIZE][2]) {
 uint8_t i;
 Serial.println("======== =======");
 for (i = 0; i < SIZE; i++) {
   Serial.println(world[i][0], BIN);
   //Serial.print("\t");
   //Serial.println(world[i][1], BIN);
 }
}


void printBuffer(uint8_t buf[BUFSIZE]) {
  uint8_t i;
 Serial.println("========");
  for (i = 0; i < BUFSIZE; i++) {
    Serial.println(buf[i], BIN);
  }
}

void writeOutBuffer(void) {
  Wire.beginTransmission(ADD_UL);
  Wire.write(bufUL, 24);
  Wire.endTransmission();
  //Serial.print(Wire.endTransmission());
  delay(50);
  Wire.beginTransmission(ADD_UR);
  Wire.write(bufUR, 24);
  Wire.endTransmission();
  //Serial.print(Wire.endTransmission());
  delay(50);
  Wire.beginTransmission(ADD_LL);
  Wire.write(bufLL, 24); 
  Wire.endTransmission();
  //Serial.print(Wire.endTransmission());
  delay(50);
  Wire.beginTransmission(ADD_LR);
  Wire.write(bufLR, 24);
  Wire.endTransmission();
  //Serial.println(Wire.endTransmission());
  delay(50);
}

