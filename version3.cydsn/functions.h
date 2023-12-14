/* ========================================
 * Artyom Martirosyan
 * Final project functions
 * Program Synopsis
 * 
 * This file consists of all of the functions and global variables used in the main program.
 * 
 *
 * ========================================
*/
#include "project.h"
#include "GUI.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
void init_uart_printf(void);
void uart_printf(char *print_string);
int char_index = 0; // Indexing for terminal inputs
char result[50]; // Buffer for terminal inputs

#define XSIZE 320 // Width of LCD screen in pixels
#define YSIZE 240 // Height of LCF screen in pixels
#define XDIVISIONS 10 // Number of x-axis divisions for grid
#define YDIVISIONS 8 // Number of y-axis divisions for grid
#define XMARGIN 5 // Margin around screen on x-axis
#define YMARGIN 4 // Margin around screen on y-axis
#define MAXPOINTS XSIZE-2*XMARGIN // Maximum number of points in wave for plotting

int x_scale_1 = 1000; // X scale value in mv(for frequncy)
int y_scale_1 = 1000; // Y scale value for height(voltage)

// Ping pong buffers for dmas
uint16 buffera[1024], bufferb[1024], bufferc[1024], bufferd[1024];

//Creating larger buffers for doing the scaling
uint16 big_one[4096], big_two[4096];
int big_one_index, ping_pong_index, big_two_index, ping_pong_2_index = 0; // indexes for buffers
int run, run_2 = 0; // run flag for handling ps < 1 case in both buffers
// buffers for deleting waves as well as displaying the stopped result
int yplot_1[310];
int old_yplot_1[310] = {0};
int yplot_2[310];
int old_yplot_2[310] = {0};

int current_one, current_two = 0; // Current buffer being filled
int wait_one, wait_two = 0;    // Flag for if a buffer is still being read into
int start = 0; // Flag for starting and stopping the device.
int mode = 0; // 0 = free-running, 1 = trigger-mode
int slope = 0; // 0 = positive slope, 1 = negative slop.
int trigger_channel = 1; // 1 = channel 1, 2 = channel 2
int trigger_level = 2000; // Trigger level in mv
int trigger_index = 0; // index for where to start if we are triggering
int potential_1 = 100; // Global value for the potentiometer readings
int potential_2 = 120; // Global variable for the second potentiometer readings
int frequency_1, frequency_2 = 0; // Global variables for finding the frequency

/*******************************************************************************
// Does the y scaling and triggering for both waves
*******************************************************************************/
void y_scaling(int y_scale) {

    int py = 240 / 8;
    int sy = y_scale;
    trigger_index = 0; // Reset trigger index(will remaine zero if we are in free mode or trigger above reading)
    int found = 0; // Flag for if trigger index is found
    for(int i = 0; i < 4096; i++) {
        if(mode == 1) { // If we are in trigger mode
            
            if( i < 4095 && found == 0) { // If we have yet to find the value
                if(trigger_channel == 1) { // If triggering based on channel 1
                    if(slope == 1) { // If negative slope check if indexes cross trigger index from negative slope
                        if(((big_one[i]*3300)/4096) < trigger_level && ((big_one[i+1]*3300)/4096) >= trigger_level) {
                            trigger_index = i + 1; // set trigger index as the next index
                            found = 1; // flag set showing that we have found the trigger index
                        }    
                    } else if(slope == 0){ // if positive slope find where channel crosses trigger level in positive direction
                        if(((big_one[i]*3300)/4096) > trigger_level && ((big_one[i+1]*3300)/4096) <= trigger_level) {
                            trigger_index = i + 1; // set trigger index
                            found = 1; // set flag indicating we have found the trigger index
                        }
                    }
                } else if(trigger_channel == 2) { // if triggering is based on channel 2
                    if(slope == 1) { // If negative slope look for index where we cross the trigger level in negative slope
                        if(((big_two[i]*3300)/4096) < trigger_level && ((big_two[i+1]*3300)/4096) >= trigger_level) {
                            trigger_index = i + 1; // set trigger index
                            found = 1; // set flag saying we have found index
                        }    
                    } else if(slope == 0){ // if positive slope look for where we cross trigger in positive slope
                        if(((big_two[i]*3300)/4096) > trigger_level && ((big_two[i+1]*3300)/4096) <= trigger_level) {
                            trigger_index = i + 1; // set trigger index
                            found = 1; // set flag saying we have found the index
                        }
                    }
                }
            } // going to have a large else
        } 
        // yscale each individual index
        big_one[i] = (((float) py / (float) sy) * 3300 * ((float)big_one[i] / (float)4096)); 
        big_two[i] = (((float) py / (float) sy) * 3300 * ((float)big_two[i] / (float)4096));
    }
    
    
}

/*******************************************************************************
// Draw the background for the oscilloscope screen
// Note: I want to give credit to Professor varma for providing code to do this
*******************************************************************************/

void drawBackground(){ // Margin around the image
   GUI_SetBkColor(GUI_BLACK);
   GUI_SetColor(GUI_DARKCYAN);
   GUI_FillRect(0, 0, 320, 420);
}

/*******************************************************************************
// Draw the X/Y grid
// Note: I want to give credit to professor varma for providing code for drawing the grid
*******************************************************************************/
void drawGrid(int w, int h, // width and height of screen
		    int xdiv, int ydiv,// Number of x and y divisions
		    int xmargin, int ymargin){ // Margin around the image

 int xstep = (w-xmargin*2)/xdiv;
 int ystep = (h-ymargin*2)/ydiv;
 
  GUI_SetPenSize(1);
   GUI_SetColor(GUI_LIGHTGRAY);
   GUI_SetLineStyle(GUI_LS_DOT);
   for (int i=1; i<ydiv; i++){
      GUI_DrawLine(xmargin, ymargin+i*ystep, w-xmargin, ymargin+i*ystep);
    }
    for (int i=1; i<xdiv; i++){
      GUI_DrawLine(xmargin+i*xstep, ymargin, xmargin+i*xstep, h-ymargin);
    }
   GUI_SetColor(GUI_LIGHTGRAY);
   GUI_DrawRect(xmargin, ymargin, w-xmargin, h-ymargin);
}

/*******************************************************************************
// Draw the line from the given x and y array
*******************************************************************************/            
void draw(){
    //Set color to light gray and redraw old wave
    GUI_SetPenSize(2);
    // Set line thickness
    
    
    
    // Set color
    GUI_SetLineStyle(GUI_LS_SOLID);
    GUI_SetColor(GUI_DARKCYAN);
    for(int i = 0; i < 309; i++) { // Erase the old waves
        GUI_DrawLine(i+5, old_yplot_1[i], i + 6, old_yplot_1[i + 1]); 
        GUI_DrawLine(i+5, old_yplot_2[i], i + 6, old_yplot_2[i + 1]);
        
    }
    //redraw the grid
    drawGrid(XSIZE, YSIZE, XDIVISIONS, YDIVISIONS, XMARGIN, YMARGIN);
    GUI_SetPenSize(2);
    GUI_SetLineStyle(GUI_LS_SOLID);
    GUI_SetColor(GUI_GREEN);
    for(int i = 0; i < 309; i++){
        GUI_SetColor(GUI_GREEN); // Draw channel 1 in green
        GUI_DrawLine(i+5, big_one[i + trigger_index] + potential_1, i+6, big_one[i + 1 + trigger_index] + potential_1); //start at i + index +...
        GUI_SetColor(GUI_RED); // Draw channel 2 in red
        GUI_DrawLine(i+5, big_two[i + trigger_index] + potential_2, i+6, big_two[i + 1 + trigger_index] + potential_2);
        
        // store the old values for future erasing or if we are in stopped state
        yplot_1[i] = big_one[i + trigger_index];
        old_yplot_1[i] = big_one[i + trigger_index] + potential_1;
        
        yplot_2[i] = big_two[i + trigger_index];
        old_yplot_2[i] = big_two[i + trigger_index] + potential_2;
        
        
    }
    //get the final index for erasing/stopping
    yplot_1[309] = big_one[309 + trigger_index];
    old_yplot_1[309] = big_one[309 + trigger_index] + potential_1;
    // get final index for erasing/stopping
    yplot_2[309] = big_two[309 + trigger_index];
    old_yplot_2[309] = big_two[309 + trigger_index] + potential_2; 
}

/*******************************************************************************
// Draw the lines in the stopped state, essentially redraw same wave, but may change location if scrollled
*******************************************************************************/
void drawstopped(){
    //Set color to light gray and redraw old wave
    GUI_SetPenSize(2);
    // Set line thickness
    
    
    
    // Set color
    GUI_SetLineStyle(GUI_LS_SOLID);
    GUI_SetColor(GUI_DARKCYAN);
    for(int i = 0; i < 309; i++) { // erase the old lines by drawing over them
        GUI_DrawLine(i+5, old_yplot_1[i], i + 6, old_yplot_1[i + 1]);
        GUI_DrawLine(i+5, old_yplot_2[i], i + 6, old_yplot_2[i + 1]);
    }
    drawGrid(XSIZE, YSIZE, XDIVISIONS, YDIVISIONS, XMARGIN, YMARGIN);
    GUI_SetPenSize(2);
    GUI_SetLineStyle(GUI_LS_SOLID);
    GUI_SetColor(GUI_GREEN);
    for(int i = 0; i < 309; i++){
        GUI_SetColor(GUI_GREEN); // Draw the first channel
        GUI_DrawLine(i+5, yplot_1[i] + potential_1, i+6, yplot_1[i + 1] + potential_1); //start at i + index +...
        
        GUI_SetColor(GUI_RED); // Draw the second channel
        GUI_DrawLine(i+5, yplot_2[i] + potential_2, i+6, yplot_2[i + 1] + potential_2);
        
        
        // Store these values for erasing later
        old_yplot_1[i] = yplot_1[i] + potential_1;
        old_yplot_2[i] = yplot_2[i] + potential_2;
    }
    // store last values for erasing
    old_yplot_1[309] = yplot_1[309] + potential_1;
    old_yplot_2[309] = yplot_2[309] + potential_2;
}

/*******************************************************************************
// Displays the frequencys of the waves
// Note: I want to give credit to Professor varma for providing the main logic for displaying the frequency.
*******************************************************************************/
void printFrequency() { 
 char str[20];
 
 GUI_SetBkColor(GUI_DARKCYAN); // Set background color
 GUI_SetFont(GUI_FONT_16B_1); // Set font
 GUI_SetColor(GUI_DARKCYAN);
 GUI_FillRect(10, 10, 130, 60); // Creates a rectangle for erasing the old frequencys
 GUI_SetColor(GUI_LIGHTGRAY); // Set foreground color
 sprintf(str, "Ch 1 Freq: %0d Hz", frequency_1); // Displays the freqeuncy for wave 1
 GUI_DispStringAt(str, 10, 10);
 sprintf(str, "Ch 2 Freq: %0d Hz", frequency_2); // Displays the freqeuncy for wave 2
 GUI_DispStringAt(str, 10, 30);
}

/*******************************************************************************
// Displays the xscale and y scale
// Note: I want to give credit to professor varma for providing the main logic for displaying the scaling
*******************************************************************************/
void printScaleSettings() { 
 char str[20];
 GUI_SetColor(GUI_DARKCYAN);
 GUI_FillRect(200, 10, 320, 60); // Using a rectangle for clearing the old displayed values
 GUI_SetBkColor(GUI_DARKCYAN); // Set background color
 GUI_SetFont(GUI_FONT_16B_1); // Set font
 GUI_SetColor(GUI_LIGHTGRAY); // Set foreground color
 if (x_scale_1 >= 1000) // If x scale >= 1000 convert to ms
    sprintf(str, "Xscale: %0d ms/div", x_scale_1/1000);
 else
    sprintf(str, "Xscale: %0d us/div", x_scale_1);
 GUI_DispStringAt(str, 200, 10);
 int yscaleVolts = y_scale_1/1000;
 int yscalemVolts = y_scale_1 % 1000;
 if (y_scale_1 >= 1000)
   sprintf(str, "Yscale: %0d V/div", yscaleVolts);
 else
   sprintf(str, "Yscale: %0d.%0d V/div", yscaleVolts, yscalemVolts/100);
 GUI_DispStringAt(str, 200, 30);
  
}
/*******************************************************************************
// This function is used to calculate the fequency for the two channels
*******************************************************************************/
void frequency_finder(){
    int max = 0;
    int min = 2000;
    for(int i = 0; i < 4096; i++) {  // Iterate through buffer
        if(big_one[i] > max) {                 // If buffer value larger then max set value as new max
            max =  big_one[i];
        }
        if(big_one[i] < min) {                 // If buffer value smaller then min set value as new min
            min = big_one[i];
        }
    }
    float average = ((max - min) / 2);               // Find the middle voltage
    
    // Now calculate frequency:
    int first = 0;
    int second = 0;
    for(int i = 0; i < (4096); i++) { // Iterate through buffer
        if(big_one[i] < average && big_one[i+1] > average) { // If rising past mid point(doing plus 4 to account for potential noise)
            first = i;                       // Set first crossing point 
            break;
        }
    }
    for(int i = first + 1; i < (4096); i++) { // Iterate from previous crossing point
        if(big_one[i] < average && big_one[i+1] > average) {   // If a second rising edge 
            second = i;                        // Set Second rising edge
            break;
        }
    }
    
    int period = (second - first);    // Get period
    frequency_1 = (1000000 / (period * (x_scale_1/32)))/10; // Frequency = 1/period, need to also factor in the scaling
    
    max = 0;
    min = 2000;
    for(int i = 0; i < 4096; i++) {  // Iterate through buffer
        if(big_two[i] > max) {                 // If buffer value larger then max set value as new max
            max =  big_two[i];
        }
        if(big_two[i] < min) {                 // If buffer value smaller then min set value as new min
            min = big_two[i];
        }
    }
    average = ((max - min) / 2);               // Find the middle voltage
    
    // Now calculate frequency:
    first = 0;
    second = 0;
    for(int i = 0; i < (4096); i++) { // Iterate through buffer
        if(big_two[i] < average && big_two[i+1] > average) { // If rising past mid point(doing plus 4 to account for potential noise)
            first = i;                       // Set first crossing point 
            break;
        }
    }
    for(int i = first + 1; i < (4096); i++) { // Iterate from previous crossing point
        if(big_two[i] < average && big_two[i+1] > average) {   // If a second rising edge 
            second = i;                        // Set Second rising edge
            break;
        }
    }
    
    period = (second - first);    // Get period
    frequency_2 = (1000000 / (period * (x_scale_1/32)))/10; // Frequency = 1/period, need to factor in the x scaling 
}



/*******************************************************************************
// This function is used to handle the terminal inputs
*******************************************************************************/
void terminal_handler(char *result, int size){
    
    if (strncmp(result, "set mode free", size) == 0){
        if(start == 0){ // If in stopped state.
            uart_printf("Mode set to free-running \n \r");
            mode = 0;
        } else {
            uart_printf("Mode can only be set when stopped. \n \r");
        }
        //Setting the mode to free-running
    } else if (strncmp(result, "set mode trigger", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Mode set to trigger \n \r");
            mode = 1;
        } else {
            uart_printf("Mode can only be set when stopped. \n \r");
        }
        //Setting the mode to trigger
    } else if (strncmp(result, "set trigger_level 0", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 0 mv\n \r");
            trigger_level = 3000 - 0;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 0 mv
    } else if (strncmp(result, "set trigger_level 100", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 100 mv\n \r");
            trigger_level = 3000 - 100;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 100 mv
    } else if (strncmp(result, "set trigger_level 200", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 200 mv\n \r");
            trigger_level = 3000 - 200;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 200 mv
    } else if (strncmp(result, "set trigger_level 300", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 300 mv\n \r");
            trigger_level = 3000 - 300;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 300 mv
    } else if (strncmp(result, "set trigger_level 400", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 400 mv\n \r");
            trigger_level = 3000 - 400;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 400 mv
    } else if (strncmp(result, "set trigger_level 500", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 500 mv\n \r");
            trigger_level = 3000 - 500;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 500 mv
    } else if (strncmp(result, "set trigger_level 600", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 600 mv\n \r");
            trigger_level = 3000 - 600;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 600 mv
    } else if (strncmp(result, "set trigger_level 700", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 700 mv\n \r");
            trigger_level = 3000 - 700;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 700 mv
    } else if (strncmp(result, "set trigger_level 800", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 800 mv\n \r");
            trigger_level = 3000 - 800;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 800 mv
    } else if (strncmp(result, "set trigger_level 900", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 900 mv\n \r");
            trigger_level = 3000 - 900;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 900 mv
    } else if (strncmp(result, "set trigger_level 1000", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 1000 mv\n \r");
            trigger_level = 3000 - 1000;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 1000 mv
    } else if (strncmp(result, "set trigger_level 1100", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 1100 mv\n \r");
            trigger_level = 3000 - 1100;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 1100 mv
    } else if (strncmp(result, "set trigger_level 1200", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 1200 mv\n \r");
            trigger_level = 3000 - 1200;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 1200 mv
    } else if (strncmp(result, "set trigger_level 1300", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 1300 mv\n \r");
            trigger_level = 3000 - 1300;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 1300 mv
    } else if (strncmp(result, "set trigger_level 1400", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 1400 mv\n \r");
            trigger_level = 3000 - 1400;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 1400 mv
    } else if (strncmp(result, "set trigger_level 1500", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 1500 mv\n \r");
            trigger_level = 3000 - 1500;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 1500 mv
    } else if (strncmp(result, "set trigger_level 1600", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 1600 mv\n \r");
            trigger_level = 3000 - 1600;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 1600 mv
    } else if (strncmp(result, "set trigger_level 1700", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 1700 mv\n \r");
            trigger_level = 3000 - 1700;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 1700 mv
    } else if (strncmp(result, "set trigger_level 1800", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 1800 mv\n \r");
            trigger_level = 3000 - 1800;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 1800 mv
    } else if (strncmp(result, "set trigger_level 1900", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 1900 mv\n \r");
            trigger_level = 3000 - 1900;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 1900 mv
    } else if (strncmp(result, "set trigger_level 2000", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 2000 mv\n \r");
            trigger_level = 3000 - 2000;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 2000 mv
    } else if (strncmp(result, "set trigger_level 2100", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 2100 mv\n \r");
            trigger_level = 3000 - 2100;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        
        //Setting the trigger level to 2100 mv
    } else if (strncmp(result, "set trigger_level 2200", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 2200 mv\n \r");
            trigger_level = 3000 - 2200;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 2200 mv
    } else if (strncmp(result, "set trigger_level 2300", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 2300 mv\n \r");
            trigger_level = 3000 - 2300;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 2300 mv
    } else if (strncmp(result, "set trigger_level 2400", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 2400 mv\n \r");
            trigger_level = 3000 - 2400;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 2400 mv
    } else if (strncmp(result, "set trigger_level 2500", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 2500 mv\n \r");
            trigger_level = 3000 - 2500;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 2500 mv
    } else if (strncmp(result, "set trigger_level 2600", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 2600 mv\n \r");
            trigger_level = 3000 - 2600;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 2600 mv
    } else if (strncmp(result, "set trigger_level 2700", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 2700 mv\n \r");
            trigger_level = 3000 - 2700;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 2700 mv
    } else if (strncmp(result, "set trigger_level 2800", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 2800 mv\n \r");
            trigger_level = 3000 - 2800;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 2800 mv
    } else if (strncmp(result, "set trigger_level 2900", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 2900 mv\n \r");
            trigger_level = 3000 - 2900;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 2900 mv
    } else if (strncmp(result, "set trigger_level 3000", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger level set to 3000 mv\n \r");
            trigger_level = 3000 - 3000;
        } else {
            uart_printf("Trigger level can only be set when stopped. \n \r");
        }
        
        //Setting the trigger level to 3000 mv
    } else if (strncmp(result, "set trigger_slope negative", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger slope set to negative\n \r");
            slope = 1;
        } else {
            uart_printf("Slope can only be set when stopped. \n \r");
        }
        //Setting the trigger slope to negative
    } else if (strncmp(result, "set trigger_slope positive", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger slope set to positive\n \r");
            slope = 0;
        } else {
            uart_printf("Slope can only be set when stopped. \n \r");
        }
        
        //Setting the trigger slope to positive
    } else if (strncmp(result, "set trigger_channel 1", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger channel set to 1\n \r");
            trigger_channel = 1;
        } else {
            uart_printf("Trigger channel can only be set when stopped. \n \r");
        }
        
        //Setting the trigger channel to 1
    } else if (strncmp(result, "set trigger_channel 2", size) == 0) {
        if(start == 0){ // If in stopped state.
            uart_printf("Trigger channel set to 2\n \r");
            trigger_channel = 2;
        } else {
            uart_printf("Trigger channel can only be set when stopped. \n \r");
        }
        
        //Setting the trigger channel to 2
    } else if (strncmp(result, "set xscale 100", size) == 0) {
        uart_printf("Setting the x scale to 100 us\n \r");
        //Setting the x scale to 100 us
        x_scale_1 = 100;
    } else if (strncmp(result, "set xscale 200", size) == 0) {
        uart_printf("Setting the x scale to 200 us\n \r");
        //Setting the x scale to 200 us
        x_scale_1 = 200;
    } else if (strncmp(result, "set xscale 500", size) == 0) {
        uart_printf("Setting the x scale to 500 us\n \r");
        //Setting the x scale to 500 us
        x_scale_1 = 500;
    } else if (strncmp(result, "set xscale 1000", size) == 0) {
        uart_printf("Setting the x scale to 1000 us\n \r");
        //Setting the x scale to 1000 us
        x_scale_1 = 1000;
    } else if (strncmp(result, "set xscale 2000", size) == 0) {
        uart_printf("Setting the x scale to 2000 us\n \r");
        //Setting the x scale to 2000 us
        x_scale_1 = 2000;
    } else if (strncmp(result, "set xscale 5000", size) == 0) {
        uart_printf("Setting the x scale to 5000 us\n \r");
        //Setting the x scale to 5000 us
        x_scale_1 = 5000;
    } else if (strncmp(result, "set xscale 10000", size) == 0) {
        uart_printf("Setting the x scale to 10000 us\n \r");
        //Setting the x scale to 10000 us
        x_scale_1 = 10000;
    } else if (strncmp(result, "set yscale 500", size) == 0) {
        uart_printf("Setting the y scale to 500 mv\n \r");
        //Setting the x scale to 500 mv
        y_scale_1 = 500;
    } else if (strncmp(result, "set yscale 1000", size) == 0) {
        uart_printf("Setting the y scale to 1000 mv\n \r");
        //Setting the x scale to 1000 mv
        y_scale_1 = 1000;
    } else if (strncmp(result, "set yscale 1500", size) == 0) {
        uart_printf("Setting the y scale to 1500 mv\n \r");
        //Setting the x scale to 1500 mv
        y_scale_1 = 1500;
    } else if (strncmp(result, "set yscale 2000", size) == 0) {
        uart_printf("Setting the y scale to 2000 mv\n \r");
        //Setting the x scale to 2000 mv
        y_scale_1 = 2000;
    } else if (strncmp(result, "start", size) == 0) {
        uart_printf("Starting\n \r");
        start = 1;
        //Start displaying
    } else if (strncmp(result, "stop", size) == 0) {
        uart_printf("Stopping\n \r");
        start = 0;
        //Stop the displaying and stop sampling
    } else {
        uart_printf("Not valid input\n \r");
        //Stop the displaying and stop sampling
    }
    
    

}

/*******************************************************************************
// Function used to scan the uart constanly in order to get the terminal inputs
*******************************************************************************/
void recieve_data(){
    
    char value;
    // Checks if the rx fifo is not empty indicating data has been transfered
    uint32_t rxStatus = Cy_SCB_UART_GetRxFifoStatus(UART_printf_HW); // Get the rx fifo status
    if (rxStatus & CY_SCB_UART_RX_NOT_EMPTY) {
        value = Cy_SCB_UART_Get(UART_printf_HW);
        if(value == '\n' || value == '\r') { // If we are at the end of the word
            // Need to service and reset buffer
            terminal_handler(result, char_index + 1);
            
            memset(result, 0, 50);
            char_index = 0;
            
        } else {
            result[char_index] = value;
            char_index++;
        }
        Cy_SCB_UART_ClearRxFifoStatus(UART_printf_HW, CY_SCB_UART_RX_NOT_EMPTY);
    }
    
}

/* [] END OF FILE */
