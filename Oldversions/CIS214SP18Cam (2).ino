/*
=============================================
|											                      |
|				        CCD Camera code			        |
|				          CIS214 SP18        				|
|											                      |
|					         Authors:				          |
|			
=============================================
*/

#include <limits.h>

//TEMP (GPIO)         Physical
#define TEMP 14       //36
#define TEC 32        //on board dac controll

//SPI (GPIO)          Physical
#define DAC_CS 15     // 37
#define VID_CS 9     // 12
#define DAC_MOSI 11   // 13
#define DAC_MISO 12   // 14
#define DAC_SCK 13    // 35

//voltages
//  amp = x3
//    DAC0 = 6V {6/3 = 2V} [2/5*256 = 102.4]
//    DAC1 = 4V {4/3 = 1.3333V} [1.3333/5*256 = 68.26496]
//    DAC2 = 0V {0/3 = 0V} [0/5*256 = 0]
//    DAC3 = -10V (10V) {10/3 = 3.3333V} [3.3333/5*256 = 170.66496]
//    DAC4 = -4V (4V) {4/3 = 1.3333V} [1.3333/5*256 = 68.26496]
//    DAC5 = -2V (2V) {2/3 = .6667V} [.6667/5*256 = 34.13504]
//    DAC6 = 9V {9/3 = 3V} [3/5*256 = 153.6]
//    DAC7 = 11V {11/3 = 3.6667V} [3.6667/5*256 = 187.73504]
//    DAC8 = 4V {4/3 = 1.3333V} [1.3333/5*256 = 68.26496]
//    DAC9 = 2V {2/3 = .6667V} [.6667/5*256 = 34.13504]
//  Bit order
//    Least --- Most		Most ----------------- Least
//  Read order
//   |D11|D10|D9|D8|       |D7|D6|D5|D4|   |D3|D2|D1|D0|
//0b[|  0|  0| 0| 0| pin] [| 0| 0| 0| 0|   | 0| 0| 0| 0| k value] where k = Vout/Vref * 256 (Vref = 5V) 
#define DAC0 0b100001100110   // 102
#define DAC1 0b010001000100   // 68
#define DAC2 0b110000000000   // 0
#define DAC3 0b001010101011   // 170 | 1010 1010 // 171
#define DAC4 0b101001000100   // 68
#define DAC5 0b011000100010   // 34
#define DAC6 0b111010011001   // 153 | 1001 1010 // 154
#define DAC7 0b000110111011   // 187 | 1011 1100 // 188
#define DAC8 0b100101000100   // 68
#define DAC9 0b010100100010   // 34

//nop delay for 192MHZ (oc) in microseconds (5 microseconds per nop instruction)
#define NOP "nop\n\t"
#define DELAY5NS __asm__(NOP);
#define DELAY10NS __asm__(NOP NOP);
#define DELAY15NS __asm__(NOP NOP NOP);
#define DELAY20NS __asm__(NOP NOP NOP NOP);
#define DELAY25NS __asm__(NOP NOP NOP NOP NOP);
#define DELAY30NS __asm__(NOP NOP NOP NOP NOP NOP);
#define DELAY35NS __asm__(NOP NOP NOP NOP NOP NOP NOP);
#define DELAY40NS __asm__(NOP NOP NOP NOP NOP NOP NOP NOP);
#define DELAY45NS __asm__(NOP NOP NOP NOP NOP NOP NOP NOP NOP);
#define DELAY50NS __asm__(NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP);
#define DELAY55NS __asm__(NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP);
#define DELAY60NS __asm__(NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP);
#define DELAY65NS __asm__(NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP);
#define DELAY70NS __asm__(NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP NOP);

//pins (GPIO)                           Physical
#define V1 5                            // 7
#define V2 8                            // 10
#define H 6                             // 8
#define RST 2                           // 4
#define OA 7                            // 9
#define CDS1 26                         // 18
#define CDS2 27                         // 19
#define CDS3 28                         // 20
#define SHTR1 21                        // 42
#define SHTR2 20                        // 43

//bits                                  //PORT
#define V1BIT 0b10000000                //D (1 << 7)
#define V2BIT 0b1000                	  //D (1 << 3)
#define HBIT 0b10000                 	  //D (1 << 4)
#define RSTBIT 0b1              		    //D (1 << 0)
#define OABIT 0b100                		  //D (1 << 2)
#define CDS1BIT 0b100000000000000       //A (1 << 14)
#define CDS2BIT 0b1000000000000000      //A (1 << 15)
#define CDS3BIT 0b10000000000000000     //A (1 << 16)
#define SHTR1BIT 0b01000000             //D (1 << 6)
#define SHTR2BIT 0b00100000             //D (1 << 5)
//bits SPI
#define VID_CSBIT 0b1000				        //C (1 << 4)
#define DAC_SCKBIT 0b100000 			      //C (1 << 5)
#define DAC_MISOBIT 0b10000000          //C (1 << 7)

// bit on
#define V1ON (GPIOD_PSOR |= V1BIT)
#define V2ON (GPIOD_PSOR |= V2BIT)
#define HON (GPIOD_PSOR |= HBIT)
#define RSTON (GPIOD_PSOR |= RSTBIT)
#define OAON (GPIOD_PSOR |= OABIT)
#define CDS1ON (GPIOA_PSOR |= CDS1BIT)
#define CDS2ON (GPIOA_PSOR |= CDS2BIT)
#define CDS3ON (GPIOA_PSOR |= CDS3BIT)
#define SHTR1ON (GPIOD_PSOR |= SHTR1BIT)
#define SHTR2ON (GPIOD_PSOR |= SHTR2BIT)
// bit off
#define V1OFF (GPIOD_PCOR |= V1BIT)
#define V2OFF (GPIOD_PCOR |= V2BIT)
#define HOFF (GPIOD_PCOR |= HBIT)
#define RSTOFF (GPIOD_PCOR |= RSTBIT)
#define OAOFF (GPIOD_PCOR |= OABIT)
#define CDS1OFF (GPIOA_PCOR |= CDS1BIT)
#define CDS2OFF (GPIOA_PCOR |= CDS2BIT)
#define CDS3OFF (GPIOA_PCOR |= CDS3BIT)
#define SHTR1OFF (GPIOD_PCOR |= SHTR1BIT)
#define SHTR2OFF (GPIOD_PCOR |= SHTR2BIT)

//SPI bit high
#define VID_CSHIGH (GPIOC_PSOR |= VID_CSBIT)
#define DAC_SCKHIGH (GPIOC_PSOR |= DAC_SCKBIT)

//SPI bit low
#define VID_CSLOW (GPIOC_PCOR |= VID_CSBIT)
#define DAC_SCKLOW (GPIOC_PCOR |= DAC_SCKBIT)

//SPI Read
#define DAC_MISOREAD (GPIOC_PDIR & DAC_MISOBIT) == 0 ? 0 : 1;

//milti-bit macros
#define V1HV2L GPIOD_PDOR = (GPIOD_PDOR & ~V2BIT) | V1BIT   //V1 high V2 low
#define V1LV2H GPIOD_PDOR = (GPIOD_PDOR & ~V1BIT) | V2BIT  //V2 high V1 low

//Reset Pulse
#define RESET RSTON; CDS3ON; HOFF; CDS1OFF; delayNanoseconds(400); CDS3OFF; RSTOFF; delayMicroseconds(1); CDS2ON; delayNanoseconds(2000); CDS2OFF; 

//Vertical Shift
#define VERTICAL V1ON; delayMicroseconds(2); V1LV2H; delayMicroseconds(2); V1HV2L; delayMicroseconds(2); V1OFF; delayMicroseconds(1);

//Horizontal Shift
#define HORIZONTAL HON; CDS1ON; delayNanoseconds(1000); CDS2ON; delayNanoseconds(2000); CDS2OFF;

//Nanosecond timing code
volatile unsigned long delayCycles_end = 0L;
unsigned long ns2cycles(unsigned long ns){
  return (ns*(F_CPU / 8000000UL)) / 125UL;
}
unsigned long cycles2ns(unsigned long cycles){
  return (cycles * 125UL) / (F_CPU / 8000000UL);
}
#define delayNanoseconds(ns) delayCycles_end = ARM_DWT_CYCCNT+ns2cycles(ns-80); while (ARM_DWT_CYCCNT < delayCycles_end)
//

//Constants
// const int frameHeight = 520;
// const int frameWidth = 796;
//test Constants
const int frameHeight = 50;
const int frameWidth = 10;

// put your setup code here, to run once
void setup() {
  analogReadResolution(12);
  Serial.begin(115000);
  //DAC/ADC
  pinMode(TEMP, INPUT);
  pinMode(TEC, OUTPUT);
  pinMode(DAC_CS, OUTPUT);
  pinMode(VID_CS, OUTPUT);
  pinMode(DAC_MOSI, OUTPUT);
  pinMode(DAC_MISO, INPUT);
  pinMode(DAC_SCK, OUTPUT);
  //CCD pins
  pinMode(V1, OUTPUT);
  pinMode(V2, OUTPUT);
  pinMode(H, OUTPUT);
  pinMode(RST, OUTPUT);
  pinMode(OA, OUTPUT);
  pinMode(CDS1, OUTPUT);
  pinMode(CDS2, OUTPUT);
  pinMode(CDS3, OUTPUT);
  pinMode(SHTR1, OUTPUT);
  pinMode(SHTR2, OUTPUT);
  //set voltages
  DACW(DAC0);
  DACW(DAC1);
  DACW(DAC2);
  DACW(DAC3);
  DACW(DAC4);
  DACW(DAC5);
  DACW(DAC6);
  DACW(DAC7);
  DACW(DAC8);
  DACW(DAC9);
  //set default clock state
  //  Each line (row) of charge is first transported from
  //  the vertical CCD to the horizontal CCD register using the
  //  V1 and V2 register clocks. The horizontal CCD is
  //  presented a new line on the falling edge of V2 while H1
  //  is held high. The horizontal CCD then transports each line,
  //  pixel by pixel, to the output structure by alternately clocking
  //  the H1 and H2 pins in a complementary fashion. On each
  //  falling edge of H2 a new charge packet is transferred onto
  //  a floating diffusion and sensed by the output amplifier
  HON;

  //Nanosecond timing code
  ARM_DEMCR |= ARM_DEMCR_TRCENA;
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
}
//

//DAC bit write
void DACW(int bitVal) {
  digitalWrite(DAC_CS, LOW);
  for (int i=11; i>=0; i--){
    int b = bitRead(bitVal, i);
    digitalWrite(DAC_MOSI, b);
    digitalWrite(DAC_SCK, HIGH);
    digitalWrite(DAC_SCK, LOW);
  }
  digitalWrite(DAC_CS, HIGH);
}
//

//ADC bit read
int ADCR() {
  DAC_SCKLOW;
  int pixBit = 0;
  VID_CSHIGH;
  delayMicroseconds(2);
  // delayNanoseconds(300);
  VID_CSLOW;
  DELAY15NS
  for (int i=13; i>=0; i--){
    // pixBit = (pixBit << 1) | DAC_MISOREAD;
    DAC_SCKHIGH;
    DELAY10NS
    pixBit = (pixBit << 1) | DAC_MISOREAD;
    
    DAC_SCKLOW;
    DELAY20NS
  }
  DELAY20NS
  return pixBit;
}

//ADC bit read
int ADCRold() {
  digitalWrite(DAC_SCK, LOW);
  int pixBit = 0;
  int bit = 0;
  digitalWrite(VID_CS, HIGH);
  delayMicroseconds(2);
  delayNanoseconds(200);
  digitalWrite(VID_CS, LOW);
  DELAY70NS
  for (int i=13; i>=0; i--){
    digitalWrite(DAC_SCK, HIGH);
    bit = digitalRead(DAC_MISO);
    pixBit = (pixBit << 1) | bit;
    Serial.print(bit);
    digitalWrite(DAC_SCK, LOW);
    DELAY70NS
  }
  Serial.println("--------");
  return pixBit;
}

//code for frame transfer
void full_frame(){
  int row[frameWidth];
  int pxR = 0;
  for (int j=0; j<frameHeight; j++){
    VERTICAL
    for (int k=0; k<frameWidth; k++){
      RESET
      HORIZONTAL
      // row[frameWidth-k] = ADCR();  // read value and add value to row
    }
    //send row to host
  }
}

//clear the sensor
void flush(){
  int row[frameWidth];
  int pxR = 0;
  for (int j=0; j<frameHeight; j++){
    VERTICAL
    for (int k=0; k<frameWidth; k++){
      RESET
      HORIZONTAL
    }
  }
}

// test loop
void loop() {
  // put your main code here, to run repeatedly
  int row[0];
  Serial.println("HI");
  // for (int k=0; k<frameWidth; k++){
    // RESET
    // HORIZONTAL
    // row[frameWidth-k] =
    ADCR();  // read value and add value to row
  // }
  // for (int i = 0; i< frameWidth; i++) {
  //   Serial.println(row[i]);
  // }
  delayMicroseconds(500);
}

// TEC read and controll
//void readTEC(){
//  int TEMP_PIN = analogRead(TEMP);
//  double Vout = (double) TEMP_PIN/ 4095 * 3.3;
//  double temperature = (Vout - 0.5)/.01;
//  double wrong_temp = temperature * 9/5 + 32;
//
//  Serial.println(temperature);
//  Serial.println(wrong_temp);
//
//  delay(500);
//}

// binnging and subframe code
// void get_bin() {  //params specified by host need to do it in overload potentially 
//   //Image parameters from host
//   int ysize = frameHeight;  // set by host or default to FrameHeight
//   int xsize = frameWidth;  // set by host or default to FrameWidth
//   int xbin = 1;
//   int ybin = 1;
//   int xstart = 0;
//   int ystart = 0;
//   int pxR = 0;
//   int row[xsize];

//   //Align vertical position for sub frame 
//   for(int i=0; i<frameHeight-(ystart + ysize); i++) { VERTICAL }
//   for(int i=0; i<frameWidth;i++) { HORIZONTAL } // Clear data from sensor shift register

//   //Iterate frame to collect data 
//   for(int i=0; i<(ysize/ybin)+(ysize%ybin)/(ysize%ybin); i++){
       
//     // VERTICAL SHIFT & Binning
//     for(int k=0; k<ybin -((ybin - ysize%ybin)* !((i-(ysize/ybin)) >> sizeof(int)*CHAR_BIT-1 )); k++) { VERTICAL }

//     //Align horizontal position for sub frame 
//     for(int i=0; i<frameWidth-(xstart + xsize); i++) { HORIZONTAL }
    
//     for(int j=0; j<(xsize/xbin)+(xsize%xbin)/(xsize%xbin); j++){
//       RESET
//       // HORIZONTAL SHIFT & Binning
//       for(int k=0; k<xbin -((xbin - xsize%xbin)* !((j-(xsize/xbin)) >> sizeof(int)*CHAR_BIT-1)); k++) { HORIZONTAL }
//     }
//   }
//   //send to host
// }












