// ============================================================
// CCD Sensor array data retrival loop  
// with subframe and binning capability.
// Implemented with emulated sensor.
// 
// by Hristo I. Gueorguiev 
// for CSC214 MCC Rochester, NY 2018
// ============================================================

#include <stdio.h>
#define VERTICALSHIFT printf("\n");row--; col=COLS;
#define HORIZONTALSHIFT col--;
#define RESTFDNODE
#define CDS1
#define CDS2
#define READFDVAL printf(" %d",TestImage[row][col]);

#define ROWS 20
#define COLS 20

int TestImage[ROWS][COLS];

int fillImage(void){
    for(int i=0; i<20; i++)
        for(int j=0; j<20; j++){
            TestImage[i][j] = i*100+j;
        }
}

int main(void) {
    //TestImage array cursor variables (Remeove after TESTING)
    int row = ROWS;
    int col = COLS;
   
    //Image paramaters 
    int height = ROWS;
    int width = COLS;
    int xbin = 1;
    int ybin = 1;
    int xstart = 0;
    int ystart = 0;
    
	// Initilize test image array (Remeove after TESTING)
	fillImage();
	
	//Align vertical positon for subframe 
	for(int i=0; i<ROWS-(ystart + height); i++) { VERTICALSHIFT }
	for(int i=0; i<COLS;i++) { HORIZONTALSHIFT } // Clear data from sensor shift register
	
	//Iterate frame to collect data 
	for(int i=0; i<(height/ybin)+(height%ybin)/(height%ybin); i++){
	     
	    // VERTICAL SHIFT & Binning
	    for(int k=0; k<ybin -((ybin - height%ybin)* !((i-(height/ybin)) >> 7)); k++) { VERTICALSHIFT }
	   
        //Align horizontal positon for subframe 
    	for(int i=0; i<COLS-(xstart + width); i++) { HORIZONTALSHIFT }
	    
	    for(int j=0; j<(width/xbin)+(width%xbin)/(width%xbin); j++){
	        RESTFDNODE
	        CDS1
	        // HORIZONTAL SHIFT & Binning
	        for(int k=0; k<xbin -((xbin - width%xbin)* !((j-(width/xbin)) >> 7)); k++) { HORIZONTALSHIFT }     
	        CDS2
            READFDVAL	        
	    }
	}
	
	
	
	return 0;
}

