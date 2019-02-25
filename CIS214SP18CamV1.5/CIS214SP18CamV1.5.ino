/*
=============================================
|	                                          |
|	             CCD Camera code              |
|	              CIS214 SP18                 |
|	                                          |
|	                Authors:                  |
|	            Hristo Gueorguiev             |
|                Tony Sanzo                 |
|              Sem T. Vladyka               |
=============================================
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;

// Pins------------------------------------------

//TEMP (GPIO)           Physical
#define TEMP 14         //36
#define TEC A21         //on board dac controll ????????????????
#define TECDAC A21      //32 ????????
//Clocking (GPIO)       Physical
#define V1 5            // 7
#define V2 8            // 10
#define H 6             // 8
#define RST 2           // 4
#define OA 7            // 9
#define CDS1 26         // 18
#define CDS2 27         // 19
#define CDS3 28         // 20
#define SHTR1 20        // 42
#define SHTR2 21        // 43 

//SPI (GPIO)            Physical
#define DAC_CS 15       // 37
#define VID_CS 10       // 12
#define MOSI 11         // 13
#define MISO 12         // 14
#define SCK 13          // 35

// Voltages KAF xxxx----------------------------------------------------------------------------------------------
//  amp = x3
//    DAC0 = 6V {6/3 = 2V} [2/5*256 = 102.4]
//    DAC1 = 3V {3/3 = 1V} [1/5*256 = 51.2]
//    DAC2 = 0V {0/3 = 0V} [0.17/5*256 = 8.7]
//    DAC3 = -10V (10V) {10/3 = 3.3333V} [2.67/5*256 = 136.7]
//    DAC4 = -4V (4V) {4/3 = 1.3333V} [1.33/5*256 = 68.096]
//    DAC5 = -2V (2V) {2/3 = .6667V} [0.67/5*256 = 34.3]
//    DAC6 = 9V {9/3 = 3V} [2.33/5*256 = 119.296]
//    DAC7 = 11V {11/3 = 3.6667V} [3.33/5*256 = 170.5]
//    DAC8 = 4V {4/3 = 1.3333V} [1/5*256 = 51.2]
//    DAC9 = 2V {2/3 = .6667V} [.23/5*256 = 11.8]
//  Bit order
//    Least --- Most		Most ----------------- Least
//  Read order
//   |D11|D10|D9|D8|       |D7|D6|D5|D4|   |D3|D2|D1|D0|
//0b[|  0|  0| 0| 0| pin] [| 0| 0| 0| 0|   | 0| 0| 0| 0| k value] where k = Vout/Vref * 256 (Vref = 5V) 
#define DAC0 0b100001100110   // 102
#define DAC1 0b010000110011   // 51
#define DAC2 0b110000001001   // 9
#define DAC3 0b001010001001   // 137 | 1000 1000 // 136
#define DAC4 0b101001000100   // 68
#define DAC5 0b011000100010   // 34
#define DAC6 0b111001110111   // 119
#define DAC7 0b000110101010   // 170
#define DAC8 0b100100110011   // 51
#define DAC9 0b010100001100   // 12
//--------------------------------------------------------------------------------------------------------

//voltages KAF-0204
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
//    Least --- Most    Most ----------------- Least
//  Read order
//   |D11|D10|D9|D8|       |D7|D6|D5|D4|   |D3|D2|D1|D0|
//0b[|  0|  0| 0| 0| pin] [| 0| 0| 0| 0|   | 0| 0| 0| 0| k value] where k = Vout/Vref * 256 (Vref = 5V) 
//#define DAC0 0b100001100110   // 102
//#define DAC1 0b010001000100   // 68
//#define DAC2 0b110000000000   // 0
//#define DAC3 0b001010101011   // 170 | 1010 1010 // 171
//#define DAC4 0b101001000100   // 68
//#define DAC5 0b011000100010   // 34
//#define DAC6 0b111010011001   // 153 | 1001 1010 // 154
//#define DAC7 0b000110111011   // 187 | 1011 1100 // 188
//#define DAC8 0b100101000100   // 68
//#define DAC9 0b010100100010   // 34
//----------------------------------------------------------------------------------------------------------

// Assembly NOP instruction based delay for 192MHZ (oc) in microseconds (5 microseconds per nop instrudtion)
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

// Bits----------------------------------------------------------------------------------------------

// Clock                                //PORT        | GPIO | Physical
#define V1BIT 0b10000000                //D (1 << 7)  | 5    | 7
#define V2BIT 0b1000                	  //D (1 << 3)  | 8    | 10
#define HBIT 0b10000                 	  //D (1 << 4)  | 6    | 8
#define RSTBIT 0b1              		    //D (1 << 0)  | 2    | 4
#define OABIT 0b100                		  //D (1 << 2)  | 7    | 9
#define CDS1BIT 0b100000000000000       //A (1 << 14) | 26   | 18
#define CDS2BIT 0b1000000000000000      //A (1 << 15) | 27   | 19
#define CDS3BIT 0b10000000000000000     //A (1 << 16) | 28   | 20
// Shutter--------------------------------------------------------
#define SHTR1BIT 0b1000000              //D (1 << 6)  | 20   | 42
#define SHTR2BIT 0b100000               //D (1 << 5)  | 21   | 43
// SPI------------------------------------------------------------
#define VID_CSBIT 0b10000				        //C (1 << 4)  | 10   | 12
#define SCKBIT 0b100000 			          //C (1 << 5)  | 13   | 35
#define MISOBIT 0b10000000              //C (1 << 7)  | 12   | 14

// Clock bit ON--------------------------------------------------
#define V1ON (GPIOD_PSOR |= V1BIT)
#define V2ON (GPIOD_PSOR |= V2BIT)
#define HON (GPIOD_PSOR |= HBIT)
#define RSTON (GPIOD_PSOR |= RSTBIT)
#define OAON (GPIOD_PSOR |= OABIT)
#define CDS1ON (GPIOA_PSOR |= CDS1BIT)
#define CDS2ON (GPIOA_PSOR |= CDS2BIT)
#define CDS3ON (GPIOA_PSOR |= CDS3BIT)
// #define SHTR1ON (GPIOD_PSOR |= SHTR1BIT)
// #define SHTR2ON (GPIOD_PSOR |= SHTR2BIT)
// Clock bit OFF--------------------------------------------------
#define V1OFF (GPIOD_PCOR |= V1BIT)
#define V2OFF (GPIOD_PCOR |= V2BIT)
#define HOFF (GPIOD_PCOR |= HBIT)
#define RSTOFF (GPIOD_PCOR |= RSTBIT)
#define OAOFF (GPIOD_PCOR |= OABIT)
#define CDS1OFF (GPIOA_PCOR |= CDS1BIT)
#define CDS2OFF (GPIOA_PCOR |= CDS2BIT)
#define CDS3OFF (GPIOA_PCOR |= CDS3BIT)
// #define SHTR1OFF (GPIOD_PCOR |= SHTR1BIT)
// #define SHTR2OFF (GPIOD_PCOR |= SHTR2BIT)

// SPI bit ON----------------------------------------------------
#define VID_CSON (GPIOC_PSOR |= VID_CSBIT)
#define SCKON (GPIOC_PSOR |= SCKBIT)

// SPI bit OFF---------------------------------------------------
#define VID_CSOFF (GPIOC_PCOR |= VID_CSBIT)
#define SCKOFF (GPIOC_PCOR |= SCKBIT)

// SPI Read------------------------------------------------------
#define MISOREAD ((GPIOC_PDIR & MISOBIT) == 0 ? 0 : 1)
//#define MISOREAD(value) (GPIOC_PDIR & MISOBIT) == 0 ? (value << 1) | 0 : (value << 1) | 1

// milti-bit macros----------------------------------------------
#define V1HV2L GPIOD_PDOR = (GPIOD_PDOR & ~V2BIT) | V1BIT   //V1 High | V2 Low
#define V1LV2H GPIOD_PDOR = (GPIOD_PDOR & ~V1BIT) | V2BIT  //V2 High | V1 Low
#define SHTROPEN GPIOD_PDOR = (GPIOD_PDOR & ~SHTR2BIT) | SHTR1BIT   //SHTR1 High | SHTR2 Low
#define SHTRCLOSE GPIOD_PDOR = (GPIOD_PDOR & ~SHTR1BIT) | SHTR2BIT  //SHTR2 High | SHTR1 Low

// ADC read------------------------------------------------------
#define READ row[xframe-k] = ADCR()
//#define READ pxR = ADCR(); Serial.print(pxR); row[frameWidth-k] = pxR debug

// CDS-----------------------------------------------------------
#define INTEGRATE CDS2ON; delayNanoseconds(1000); CDS2OFF
#define DRAINCAP CDS3ON; delayNanoseconds(500); CDS3OFF

// Reset Pulse---------------------------------------------------
#define RESET RSTON; delayNanoseconds(500); RSTOFF; DRAINCAP;
#define RESETwREAD RSTON; READ; RSTOFF; DRAINCAP; INTEGRATE;

// Vertical Shift------------------------------------------------
#define VERTICAL V1ON; delayMicroseconds(5); V1LV2H; delayMicroseconds(5); V1HV2L; delayMicroseconds(5); V1OFF; delayMicroseconds(1);

// Horizontal Shift----------------------------------------------
#define HORIZONTAL HON; delayNanoseconds(1000); HOFF;
#define HORIZONTALwCDS HON; CDS1OFF; delayNanoseconds(500); INTEGRATE; delayNanoseconds(100); HOFF; CDS1ON;

// Nanosecond timing code---------------------------------------
volatile unsigned long delayCycles_end = 0L;
unsigned long ns2cycles(unsigned long ns){
  return (ns*(F_CPU / 8000000UL)) / 125UL;
}
unsigned long cycles2ns(unsigned long cycles){
  return (cycles * 125UL) / (F_CPU / 8000000UL);
}
#define delayNanoseconds(ns) delayCycles_end = ARM_DWT_CYCCNT+ns2cycles(ns-80); while (ARM_DWT_CYCCNT < delayCycles_end)
//

// Global Variables-----------------------------------------------------------------------------------------------------------------
int xframe = 784, yframe = 520, xsize =0, ysize = 0, xoffset = 0, yoffset = 0, xbin = 0, ybin =0, tectgt = 0 , temptr;
// Above: Integer values for the widtch of the CCD sensor the number of values being binned, and the frame of data that will be read
//        from the CCD sensor.
char cString[128]; // Character array that will be used to contain or hold the commands the Host software will send to the Teensy 3.6.
char *token;       // Character pointer that will be used for the strtok, creating a stream of words command.
char *expStr;      // cString that will be used as a parameter in the fixCstring command.
int exposureTimeS = 0;  // This is the amount of time the shutter will be opened for (in milliseconds.)
int exposureTime = 0; // MS exposure time


char* commandArray [] = { "xframe?", "yframe?", "ver?" , "xsize?" , "ysize?" , "xoffset?" 
                           ,"yoffset?", "xbin?", "ybin?", "xsec?", "xmsec?" , "temp?" , "tecttgt"
                           ,"test?" , "xsize" , "ysize" , "xoffset" , "yoffset" , "xbin"
                           , "ybin" , "xsec" , "xmsec" , "tectgt" , "pon" , "poff"
                           , "tecon" , "tecoff" , "open" , "close" , "flush" , "expose" 
                           , "grimg" , "grimg_demo", "capture"};

// const int frameHeight = 520;
// const int frameWidth = 784;
// //test Constants
// const int frameHeight = 5;
// const int frameWidth = 10;


// put your setup code here, to run once
void setup() {
  analogReadResolution(12);
  Serial.begin(115000);
  //DAC/ADC
  pinMode(TEMP, INPUT);
  pinMode(TEC, OUTPUT);
  pinMode(DAC_CS, OUTPUT);
  pinMode(VID_CS, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT_PULLUP);
  pinMode(SCK, OUTPUT);
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
  //OAON;
  /* On default clock state...
   *  Each line (row) of charge is first transported from
   *  the vertical CCD to the horizontal CCD register using the
   *  V1 and V2 register clocks. The horizontal CCD is
   *  presented a new line on the falling edge of V2 while H1
   *  is held high. The horizontal CCD then transports each line,
   *  pixel by pixel, to the output structure by alternately clocking
   *  the H1 and H2 pins in a complementary fashion. On each
   *  falling edge of H2 a new charge packet is transferred onto
   *  a floating diffusion and sensed by the output amplifier
   */

  //Nanosecond timing code
  ARM_DEMCR |= ARM_DEMCR_TRCENA;
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
}






/************************************
 *             Methods              *
 ************************************/

/////////////////////////////////////////
/* void set_ExposureTime(int time_ms) *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: The argument must be sent in as a integer representing the exposure time in milliseconds.
 * postcondition: The exposure time will be updated based on the argument.
 */
void set_ExposureTime(int time_ms){
  // sets the exposure time equal to the arguement.
  exposureTime = time_ms;
}

void set_ExposureTimeS(int time_s){
  // sets the exposure time equal to the arguement.
  exposureTimeS = time_s;
}

/////////////////////////////
/* int get_ExposureTime() *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: None, no variable is passed in.   
 * postcondition: The method returns the exposuretime, and outputs it to the Serial Monitor.
 */
int get_ExposureTime(){
  Serial.print(exposureTime);     
  return exposureTime;
}

int get_ExposureTimeS(){
  exposureTimeS = exposureTimeS/1000;
  Serial.print(exposureTimeS);     
  return exposureTimeS;
}

// Frame-----------------------------------------------------------------------------

///////////////////////
/* int get_xframe() *|
\\\\\\\\\\\\\\\\\\\\\\
 * precondition: None, no variable is passed in.   
 * postcondition: The method returns the xframe, and outputs it to the Serial Monitor. 
 */
int get_xframe(){     
  Serial.print(xframe);      
  return xframe;    
}

///////////////////////
/* int get_yframe() *|
\\\\\\\\\\\\\\\\\\\\\\
// precondition: None, no variable is passed in.   
// postcondition: The method returns the yframe, and outputs it to the Serial Monitor. 
 */
int get_yframe(){     
  Serial.print(yframe);     
  return yframe;   
}       

// Size-------------------------------------------------------------------------------

//////////////////////
/* int get_xsize() *|
\\\\\\\\\\\\\\\\\\\\\
 * precondition: None, no variable is passed in.   
 * postcondition: The method returns the xSize, and outputs it to the Serial Monitor. 
 */
int get_xsize(){
  Serial.print(xframe);   
  return xsize;  
}

////////////////////////////
/* void set_xsize(int x) *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * preconditon: the argument must be an integer datatype.
 * postcondition: the xsize variable will be updated to the arguement.
 */
void set_xsize (int x){     
  xsize = x;   
}    

//////////////////////
/* int get_ysize() *|
\\\\\\\\\\\\\\\\\\\\\
 * precondition: None, no variable is passed in.
 * postcondition: The method returns the ySize, and outputs it to the Serial Monitor. 
 */
int get_ysize(){     
  Serial.print(yframe);     
  return ysize;
}   

////////////////////////////
/* void set_ysize(int y) *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: Variable passed in must be type int.  This sets the ySize. 
 * postcondition: The method sets the xSize, and outputs it to the Serial Monitor. 
 */
void set_ysize (int y){  
  ysize = y;   
}    

// Offset----------------------------------------------------------------------------

////////////////////////
/* int get_xoffset() *|
\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: None, no variable is passed in.   
 * postcondition: The method returns the xoffset, and outputs it to the Serial Monitor.  
 */
int get_xoffset(){     
  Serial.print(xoffset);      
  return xoffset;   
}

/////////////////////////////////
/* void set_xoffset(int xoff) *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: Variable passed in must be type int.  This sets the xoffset. 
 * postcondition: The method sets the xoffset, and outputs it to the Serial Monitor.
 */
void set_xoffset(int xoff){    
  xoffset = xoff;  
}       

////////////////////////
/* int get_yoffset() *|
\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: None, no variable is passed in.   
 * postcondition: The method returns the yoffset, and outputs it to the Serial Monitor.  
 */
int get_yoffset(){
  Serial.print(yoffset);     
  return yoffset;   
} 
 
/////////////////////////////////
/* void set_yoffset(int yoff) *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: Variable passed in must be type int.  This sets the yoffset. 
 * postcondition: The method sets the yoffset, and outputs it to the Serial Monitor.
 */
void set_yoffset(int yoff){     
  yoffset=yoff;   
}    

// Binning---------------------------------------------------------------------------   

/////////////////////
/* int get_xbin() *|
\\\\\\\\\\\\\\\\\\\\
 * precondition: None, no variable is passed in.   
 * postcondition: The method returns the xbin, and outputs it to the Serial Monitor.  
 */
int get_xbin(){     
  Serial.print(xbin);     
  return xbin;   
}

////////////////////////////////
/* void set_xbin(int xvalue) *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: Variable passed in must be type int.  This sets the xbin. 
 * postcondition: The method sets the xbin, and outputs it to the Serial Monitor.
 */
void set_xbin(int xvalue){   
  xbin = xvalue;  
}

////////////////////
/* int get_ybin() *|
\\\\\\\\\\\\\\\\\\\
 * precondition: None, no variable is passed in.   
 * postcondition: The method returns the ybin, and outputs it to the Serial Monitor.  
 */
int get_ybin(){     
  Serial.print(ybin);    
  return ybin;  
}

///////////////////////////////
/* void set_ybin(int yvalue) *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: Variable passed in must be type int.  This sets the ybin. 
 * postcondition: The method sets the ybin, and outputs it to the Serial Monitor.
 */
void set_ybin(int yvalue){
  ybin = yvalue;   
}    

// TEC Controll (needs attention)!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

////////////////////
/* int get_temp() *|
\\\\\\\\\\\\\\\\\\\
 * Pre-condition: None, no variable is passed in.
 * Post-condition: The method returns the current temparture, and outputs this value to the Serial Monitor.
 */
int get_temp(){
  temptr = analogRead(TEMP);
  double Vout = (temptr / 4095.0 * 3.3 - 0.5) / .01;
  double conversion = (Vout * 1.8) + 119;
  return conversion;
}

//////////////////////
/* int get_tectgt() *|
\\\\\\\\\\\\\\\\\\\\\
 * precondition: None, no variable is passed in.   
 * postcondition: The method returns the tec-temperature, and outputs it to the Serial Monitor.  
 */
int get_tectgt(){
  Serial.print(tectgt);      
  return tectgt;  
}

///////////////////////////////
/* void set_tectgt(int temp) *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: None, no variable is passed in.   
 * postcondition: The method returns the tec-temperature, and outputs it to the Serial Monitor.  
 */
void set_tectgt(int temp){     
  tectgt=temp;  
}  



// Potential defines below?

/////////////////
/* void pon() *|
\\\\\\\\\\\\\\\\
 * Pre-condition: None, no variable is passed in.
 * Post-condition: The method turns the onboard teensy DAC ON.
 */
void pon(){
  analogWrite(TECDAC,255); //Set the Pin high/Low in order to turn on the DAC
}

//////////////////
/* void poff() *|
\\\\\\\\\\\\\\\\\
 * Pre-condition: None, no variable is passed in.
 * Post-condition: The method turns the onboard teensy DAC OFF.
 */
void poff(){
  analogWrite(TECDAC,0); //Set the Pin high/Low in order to turn off the DAC
}

///////////////////
/* void tecon() *|
\\\\\\\\\\\\\\\\\\
 * Pre-condition: None, no variable is passed in.
 * Post-condition: the method returns the current temparture, and outputs this value to the Serial Monitor.
 */
void tecon(){
  digitalWrite(TEC, HIGH); //Set the Pin high/Low in order to turn off the TEC
}

///////////////////
/* void tecoff() *|
\\\\\\\\\\\\\\\\\\
 * Pre-condition: None, no variable is passed in.
 * Post-condition: the method returns the current temparture, and outputs this value to the Serial Monitor.
 */
void tecoff(){
  digitalWrite(TEC,LOW); //Set the Pin high/Low in order to turn off the TEC
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

////////////////////
/* void expose() *|
\\\\\\\\\\\\\\\\\\\
 * precondition: open() and close() must be working and defined funtions, and exposureTime must be a globally 
 *               defined variable.
 * postcondition: The shutter will be opened for the exposureTime than it will be closed. 
 */
void expose(){
  //OAOFF;                // turn off the OA to reduce noise
  SHTROPEN;             // opens the shutter
  delay(exposureTime + exposureTimeS);  // waits the amount of time that the lens needs to be exposed
  SHTRCLOSE;            // closes the shutter
  //OAON;                 // turn the OA back on once the exposure function is done
}            

/////////////////////
/* void capture() *|
\\\\\\\\\\\\\\\\\\\\
 * precondition: The functions contained within this function must be defined and working, these functions 
 *               include expose(), cameraFlush(), sendArrayValueToHost(), and the clocking functions.
 * postcondition: Captures and sends a image to the HOST software.
 */
void capture(){
   // call the flush method to clear the photon wells
   cameraFlush();
   cameraFlush(); 

   // exposes for the amount of time necessary
   expose();

   // grab image method, should send the  values captured in th CCD to the HOST software !!!!!!!!!!!!!!!!!!!!!!!!
   full_frame();
   // get_frame();
}

/////////////////////
/* int fixCString(char *seeString) *|
\\\\\\\\\\\\\\\\\\\\
 * precondition: this cString must contain only integer
 * postcondition: the newline terminited array will be replaced with a null char.
 */
int fixCString(char *seeString){
  int j = 0 ;
  
  while(seeString[j]!= '\n'){
    j++;
  }
  seeString[j] = '\0'; // converts the '\n' char to the '\0' char

  int i = atoi(seeString); // converts the c-string into a integer using the atoi function
  return i; 
}

/////////////////////
/* void cameraFlush() *|
\\\\\\\\\\\\\\\\\\\\
 * precondition: The clocking cycles must be set and in working callable functions.
 * postcondition: the photon wells of the CCD will be emptied.
 */
void cameraFlush(){
  for (int j=0; j<yframe; j++){
    VERTICAL
    for (int k=0; k<xframe; k++){
      RESET
      HORIZONTAL
    }
  }
}

////////////////////////////
/* void DACW(int bitVal) *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: None, done in Set up :205
 * postcondition: Sets up the DACs to generate the required Voltages :45
 */
void DACW(int bitVal) {
  digitalWrite(DAC_CS, LOW);
  for (int i=11; i>=0; i--){
    int b = bitRead(bitVal, i);
    digitalWrite(MOSI, b);
    digitalWrite(SCK, HIGH);
    digitalWrite(SCK, LOW);
  }
  digitalWrite(DAC_CS, HIGH);
  digitalWrite(DAC_CS, LOW);
}

////////////////////////////
/* unsigned short ADCR() *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: A pixel has been pushed into the ADC
 * postcondition: Returns a pixel value.
 */
unsigned short ADCR() {
  SCKOFF;
  unsigned short pixBit = 0;
  VID_CSON;
  delayNanoseconds(3200);
  VID_CSOFF;
  DELAY70NS
  for (int i=13; i>=0; i--){
    SCKON;
    DELAY70NS
    //pixBit = MISOREAD(pixBit);
    pixBit = (pixBit << 1) | MISOREAD;
    SCKOFF;
    DELAY70NS
  }
  DELAY70NS
  return pixBit;
}

////////////////////////
/* void full_frame() *|
\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: All voltages are set, sensor is exposed
 * postcondition: Sends yframe number of arrays to host, each containing xframe ammount of pixels
 */
void full_frame(){
  unsigned short row[xframe+1];
  unsigned short* dataPointer = row + 1;//
  for (int j=0; j<yframe; j++){
    VERTICAL
    for (int k=0; k<=xframe; k++){
      RESETwREAD
      HORIZONTALwCDS
    }
    //send row to host
    Serial.write((byte *)dataPointer, xframe*sizeof(unsigned short));
  }
}

///////////////////////
/* void get_frame() *|
\\\\\\\\\\\\\\\\\\\\\\
 * precondition: All voltages are set, sensor is exposed
 * postcondition: Sends yframe number of arrays to host, each containing xframe ammount of pixels or a binned frame or sub frame
 */
void get_frame(){
  unsigned short row[xframe+1];
  unsigned short pxR = 0;
  unsigned short* dataPointer = row + 1;

  //Align vertical position for sub frame 
  for(int i=0; i<yframe-(yoffset + ysize); i++) { VERTICAL }
  for(int i=0; i<xframe;i++) { HORIZONTAL } // Clear data from sensor shift register

  //Iterate frame to collect data 
  for(int i=0; i<(ysize/ybin)+(ysize%ybin)/(ysize%ybin); i++){
       
    // VERTICAL SHIFT & Binning
    for(int k=0; k<ybin -((ybin - ysize%ybin)* !((i-(ysize/ybin)) >> sizeof(int)*CHAR_BIT-1 )); k++) { VERTICAL }

    //Align horizontal position for sub frame 
    for(int i=0; i<xframe-(xoffset + xsize); i++) { HORIZONTAL }
    
    for(int j=0; j<(xsize/xbin)+(xsize%xbin)/(xsize%xbin); j++){
      RESET
      // HORIZONTAL SHIFT & Binning
      for(int k=0; k<xbin -((xbin - xsize%xbin)* !((j-(xsize/xbin)) >> sizeof(int)*CHAR_BIT-1)); k++) { HORIZONTAL }
      // read code
    }
  }
  //send row to host
  Serial.write((byte *)dataPointer, xframe*sizeof(unsigned short));
}

//ADC bit read with digital comands DEBUG
// int ADCRold() {
//   digitalWrite(SCK, LOW);
//   int pixBit = 0;
//   int bit = 0;
//   digitalWrite(VID_CS, HIGH);
//   delayNanoseconds(2200);
//   digitalWrite(VID_CS, LOW);
//   for (int i=13; i>=0; i--){
//     digitalWrite(SCK, HIGH);
//     bit = digitalRead(MISO);
//     pixBit = (pixBit << 1) | bit;
//     Serial.println(bit);
//     digitalWrite(SCK, LOW);
//   }
//   return pixBit;
// }

////////////////////////////////////////////////////////////////
/* sendArrayValueToHost(int photonArray [], int arraySize) *  | //scrapped
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: photonArray has to be a array pointer, and arraySize has to be the number of values passed in 
 *               to the array
 * postcondition: writes the values of the photonArray to the HOST software.
 */

// void sendArrayValueToHost(int photonArray [], int arraySize){
//  int i;

//  for(i = 0; i < arraySize; i++){
//    Serial.print(photonArray[i]);
//    Serial.write(" ");
//  }

///////////////////////////////
/* void readSerialMonitor() *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition:  cString must be a globally defined cString ( an array of chars, that the computer
 *                can interpret as a string object.)               
 * postcondition: The serial monitor will be read, and converted into a strtok broken apart with 
 *                the delimeters " ", "/n" , ",".  Each string will be in the strtok stream.
 */
void readSerialMonitor(){
  
  int moreCharacters = 1, cStringIndex = 0; // moreCharacters is used as a flag whether their are more characters in the serial buffer
                                            // and cStringIndex is used to index the cString from index 0 til the serial monitor doesn't 
                                            // have any more commands.  

  while(Serial.available() == 0); // keeps the program paused until a command is sent in from the serial monitor

  while(moreCharacters){
    
    cString[cStringIndex]= tolower(Serial.read());
    cStringIndex++;
    
    if(cString[cStringIndex - 1] == '\n' || cString[cStringIndex - 1] == '\r') {
      cString[cStringIndex++] = '\0'; // ends the character array to make it a c-string
      
      cStringIndex = 0;
      moreCharacters = 0;
      // resets the varaibles to 0
    }
  } // ends the while "reading" loop

  token = strtok(cString, " ,\n"); // uses " ," as a delimeter and breaks cString apart into strings separated by commas and spaces.
}

///////////////////////////////
/* int checkCommandOption() *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition:  commandArray must be a globally defined array, and that the command is
 *                the first word the HOST software sent to the serial buffer.
 * postcondition: Returns an integer that is the equivalent to the option index within the array, if the option
 *                is not found returns -1.
 */
int checkCommandOption(){
  // increments through the command Array from index 0 - 37, until token is matched to the commandArray string. 
  //Serial.write("ChkCmd ");
  for(int i = 0; i < 37 ; i++){ if((strcmp(token,commandArray[i]) == 0)) return i; }
  // returns the index of the command array if the command is found
    
  return -1; // returns -1 if the command is not found
}

/////////////////////////////////////
/* commandCall(int chooseCommand) *|
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 * precondition: chooseCommand parameter should be integer from 0 - 37 otherwise it will have result in the 
 *               default statement.  Also the functions for all the cases must be defined and working.
 * postcondition: The function that corresponds to the index of the chooseCommand parameter should be called.
 */
void commandCall(int chooseCommand){

  int temp, temp1;  
  //Serial.write("CmdCall ");

  switch(chooseCommand) {
    case 0: // xframe?
            temp1 = get_xframe();
            Serial.write(" OK\n"); 
            break;
    case 1: // yframe?
            temp1 = get_yframe();
            Serial.write(" OK\n"); 
            break;
    case 2: // ver?
            Serial.write("Firmware Version: champ 2.6 OK\n");
            break;
    case 3: // xsize?
            temp1 = get_xsize();
            Serial.write(" OK\n"); 
            break;
    case 4: // ysize?
            temp1 = get_ysize();
            Serial.write(" OK\n"); 
            break;
    case 5: // xoffset?
            temp1 = get_xoffset();;
            Serial.write(" OK\n"); 
            break;
    case 6: // yoffset?
            temp1 = get_yoffset();
            Serial.write(" OK\n"); 
            break;
    case 7: // xbin?
            temp1 = get_xbin();
            Serial.write(" OK\n"); 
            break;
    case 8: // ybin?
            temp1 = get_ybin();
            Serial.write(" OK\n"); 
            break;
    case 9: // xsec?
            temp1 = get_ExposureTimeS();
            Serial.write(" OK\n"); 
            break;
    case 10: // xmsec?
            temp1 = get_ExposureTime();
            Serial.write(" OK\n"); 
            break;
    case 11: // temp?
            temp1 = get_temp();
            Serial.write(" OK\n"); 
            break;
    case 12: // tectgt
            temp1 = get_tectgt();
            Serial.write(" OK\n"); 
            break;
    case 13: // test?
            Serial.write("42 OK\n");
            break;
    case 14: // xsize
           expStr = strtok(NULL, " ,\n");
           temp = fixCString(expStr);
           set_xsize(temp);
           Serial.write(" OK\n");
           break;
    case 15: // ysize
            expStr = strtok(NULL, " ,\n");
            temp = fixCString(expStr);
            set_ysize(temp);
            Serial.write(" OK\n");
            break;
    case 16: // xoffset
            expStr = strtok(NULL, " ,\n");
            temp = fixCString(expStr);
            set_xoffset(temp);  
            Serial.write(" OK\n");
            break;
    case 17: // yoffset
            expStr = strtok(NULL, " ,\n");
            temp = fixCString(expStr);
            set_yoffset(temp);
            Serial.write(" OK\n");
            break;
    case 18: // xbin
            expStr = strtok(NULL, " ,\n");
            temp = fixCString(expStr);
            set_xbin(temp); 
            Serial.write(" OK\n");
            break;
    case 19: // ybin
            expStr = strtok(NULL, " ,\n");
            temp = fixCString(expStr);
            set_ybin(temp);
            Serial.write(" OK\n");
            break;
    case 20: // xsec
             expStr = strtok(NULL, " ,\n");
             temp = fixCString(expStr);
             temp *= 1000;
             //Serial.println(temp);
             set_ExposureTimeS(temp);
             Serial.write(" OK\n");
             break;
    case 21: // xmsec
             expStr = strtok(NULL, " ,\n");
             temp = fixCString(expStr);
             //Serial.println(temp);
             set_ExposureTime(temp);
             Serial.write(" OK\n");
             break;
    case 22: // tectgt
            expStr = strtok(NULL, " ,\n");
            temp = fixCString(expStr);
            set_tectgt(temp);
            Serial.write(" OK\n");
            break;
    case 23: // pon
            pon();
            Serial.write(" OK\n");
            break;
    case 24: // poff 
            poff();
            Serial.write(" OK\n");
            break;
    case 25: // tecon
            tecon();
            Serial.write(" OK\n");
            break;
    case 26: // tecoff
            tecoff();
            Serial.write(" OK\n");
            break;
    case 27: // open
            SHTROPEN;
            Serial.write(" OK\n");
            break;
    case 28: // close
            SHTRCLOSE;
            Serial.write(" OK\n");
            break;
    case 29: // flush
            cameraFlush();
            Serial.write(" OK\n");
            break;
    case 30: // expose 
            expose();
            Serial.write(" OK\n");
            break;
    case 31: // grimg - no function and prototype //--------------------------------------------------------------------------------------

          Serial.write(" OK\n");
          break;
    case 32: // grimg_demo - no function and prototype

            Serial.write(" OK\n");
            break; 
    case 33: // capture
            capture();
            Serial.write(" OK\n");
            break;
    default: // This is the default case therefore nothing happens
            break;
  }
}

//Main loop
void loop() {

  //Serial.write("fred");
  readSerialMonitor();         // Reads the serial buffer and converts the command into the strtok.
  int j = checkCommandOption();// Using what is in the strtok buffer returns the appropriate index of the
                               // command.
  if(j > -1)commandCall(j);    // Calls the commandCall function as long as the command from the HOST software is correct.
}

// // test communication loop sfor host software
// void loop() {
//   // put your main code here, to run repeatedly
//   int xsize = frameWidth;
//   int pxR = 0;
//   int row[xsize];

//   //Serial.println("HI");
//   while(!Serial.available());
//   while(Serial.available()){
//   Serial.read();
//   }
//   Serial.write("42 OK\n");

//   while(!Serial.available());
//   while(Serial.available()){
//   Serial.read();
//   }
//   Serial.write("796 \n");

//   while(!Serial.available());
//   while(Serial.available()){
//   Serial.read();
//   }
//   Serial.write("520 \n");
//   while(!Serial.available());
//   while(Serial.available()){
//   Serial.read();
//   }
//   Serial.write("796 \n");
 
  
//   while(!Serial.available());
//   while(Serial.available()){
//   Serial.read();
//   }
//   Serial.write("520 \n");

//   while(!Serial.available());
//   while(Serial.available()){
//   Serial.read();
//   }

//   full_frame();
//   Serial.write("\n");
//   while(!Serial.available());
//   while(Serial.available()){
//   Serial.read();
//   }
// }
