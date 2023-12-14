/* ========================================
 * Artyom Martirosyan
 * Final project
 * Program Synopsis
 * 
 * This program will have a 4 channel adc where the first and 3rd channel will be
 * used with a dma to constantly read in data each channel will have a dma which will
 * use ping pong buffers to store the x scaled result. The remaining two channels and 
 * will be used for taking a potentiometer reading for scrolling. The uart will be used
 * for terminal interface and the display will be used to actually display the wave form
 * and additional details.
 * 
 *
 * ========================================
*/



#include "project.h" // Holds all of the functions used in this program
#include "GUI.h"
#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "functions.h"

void init_uart_printf(void);
void uart_printf(char *print_string);

// ISR for the first channel
void DMA_1_ISR(void)
{
     
    
    if(wait_one == 0 && start == 1) { // If we are in start mode and the data is not being processed from the big buffer
        // Calculate x scaling
        int px = 310 / 10; // Calculate px(numbfer of pixels divided by number of divisions) 
        
        int sx = x_scale_1;
        float ps = 4 *( (float) px / (float) sx); // ps = 4*number of pixels/division divided by x scale
        
        if(current_one == 0) { // If buffer a was being filled set buffer b as buffer now being filled
            current_one = 1; 
                if(ps > 1) { // If ps > 1 then we need to get every nth element
                    while(big_one_index < 4096 && ping_pong_index < 1024) { // store as many x scaled values from the ping pong buffer into the larger buffer
                        //yplot_1[i] = data[ i * (int)ps];
                        big_one[big_one_index] = buffera[ping_pong_index];
                        big_one_index++;
                        ping_pong_index = ping_pong_index +(int)ps; // essentially gets every nth value from where we left off
                    }
                    ping_pong_index = ping_pong_index - 1024; // Gets future index
                } else if (ps == 1) {
                    while(big_one_index < 4096 && ping_pong_index < 1024) { // Get every individual element from the buffer.
                        big_one[big_one_index] = buffera[ping_pong_index];
                        big_one_index++;
                        ping_pong_index++;
                    }
                    ping_pong_index = ping_pong_index - 1024; // Gets future index
                } else if (ps < 1) {
                    //yplot_1[i] = data[(i * (int)ceil(1/ps)) + ((int)ceil(1/ps))];
                    if(ping_pong_index == 0 && run == 0) {
                        ping_pong_index = ((int)ceil(1/ps));
                    }
                    while(big_one_index < 4096 && ping_pong_index < 1024) { // Gets every 1/ps element from the ping pong buffer
                        big_one[big_one_index] = buffera[ping_pong_index];
                        big_one_index++;
                        ping_pong_index = ping_pong_index + ((int)ceil(1/ps));
                        
                    }
                    ping_pong_index = ping_pong_index - 1024;
                    run = 1; // Flag used to indicate if this was the first transfer(if so need to start off with 1/ps)
                }
            
           
        } else if(current_one == 1) { // If buffer b was being filled set buffer a as buffer now being filled
            current_one = 0;
            
            if(ps > 1) {
                    while(big_one_index < 4096 && ping_pong_index < 1024) { // Get every nth element from the ping pong buffer
                        //yplot_1[i] = data[ i * (int)ps];
                        big_one[big_one_index] = bufferb[ping_pong_index];
                        big_one_index++;
                        ping_pong_index = ping_pong_index +(int)ps; // essentially gets every nth value from where we left off
                    }
                    ping_pong_index = ping_pong_index - 1024; // Gets future index
                } else if (ps == 1) {
                    while(big_one_index < 4096 && ping_pong_index < 1024) { // Does a one to one transfer from the ping pong buffer
                        big_one[big_one_index] = bufferb[ping_pong_index];
                        big_one_index++;
                        ping_pong_index++;
                    }
                    ping_pong_index = ping_pong_index - 1024; // Gets future index
                } else if (ps < 1) {
                    //yplot_1[i] = data[(i * (int)ceil(1/ps)) + ((int)ceil(1/ps))];
                    if(ping_pong_index == 0 && run == 0) {
                        ping_pong_index = ((int)ceil(1/ps));
                    }
                    while(big_one_index < 4096 && ping_pong_index < 1024) { // Get every 1/ps element
                        big_one[big_one_index] = bufferb[ping_pong_index];
                        big_one_index++;
                        ping_pong_index = ping_pong_index + ((int)ceil(1/ps));
                        
                    }
                    ping_pong_index = ping_pong_index - 1024;
                    run = 1; // Flag for if this is the first transfer    
                }
            }
    } else { // even if we arent copying data we need to keep alternating the buffers
        if(current_one == 0) { // If buffer a was being filled set buffer b as buffer now being filled
            current_one = 1;
        } else {
            current_one = 0;
        }
    }
    if(big_one_index > 4095) { // Our large buffer is now full of x scaled values
        big_one_index = 0;
        wait_one = 1; // Indicate we have data to be processed
        run = 0;      // After this we are restarting the x scaling
        ping_pong_index = 0;
    }
    Cy_DMA_Channel_ClearInterrupt(DMA_1_HW, DMA_1_DW_CHANNEL); // Clear interrupt
}

// ISR for second channel
void DMA_2_ISR(void)
{
     
    if(wait_two == 0 && start == 1) { // If the data is not being processed and we are in start mode
        // Calculate x scaling
        int px = 310 / 10; 
        
        int sx = x_scale_1;
        //May have an off by 10 issue somewhere
        float ps = 4 *( (float) px / (float) sx); // ps = number of pixels per division / xscale
        
        if(current_two == 0) { // If buffer c was being filled set buffer d as buffer now being filled
            current_two = 1;
                if(ps > 1) { // if ps > 1 get every nth value where n is ps
                    while(big_two_index < 4096 && ping_pong_2_index < 1024) {
                        //yplot_1[i] = data[ i * (int)ps];
                        big_two[big_two_index] = bufferc[ping_pong_2_index];
                        big_two_index++;
                        ping_pong_2_index = ping_pong_2_index +(int)ps; // essentially gets every 5th value from where we left off
                    }
                    ping_pong_2_index = ping_pong_2_index - 1024; // Gets future index
                } else if (ps == 1) { // if ps = 1 get each individual element
                    while(big_two_index < 4096 && ping_pong_2_index < 1024) {
                        big_two[big_two_index] = bufferc[ping_pong_2_index];
                        big_two_index++;
                        ping_pong_2_index++;
                    }
                    ping_pong_2_index = ping_pong_2_index - 1024; // Gets future index
                } else if (ps < 1) { // if ps < 1 get every 1/ps element
                    //yplot_1[i] = data[(i * (int)ceil(1/ps)) + ((int)ceil(1/ps))];
                    if(ping_pong_2_index == 0 && run_2 == 0) {
                        ping_pong_2_index = ((int)ceil(1/ps));
                    }
                    while(big_two_index < 4096 && ping_pong_2_index < 1024) {
                        big_two[big_two_index] = bufferc[ping_pong_2_index];
                        big_two_index++;
                        ping_pong_2_index = ping_pong_2_index + ((int)ceil(1/ps));
                        
                    }
                    ping_pong_2_index = ping_pong_2_index - 1024;
                    run_2 = 1;    
                }
            
           
        } else if(current_two == 1) { // If buffer d was being filled set buffer c as buffer now being filled
            current_two = 0;
            
            if(ps > 1) { // if ps > 1 get every ps index from the ping pong buffer
                    while(big_two_index < 4096 && ping_pong_2_index < 1024) {
                        //yplot_1[i] = data[ i * (int)ps];
                        big_two[big_two_index] = bufferd[ping_pong_2_index];
                        big_two_index++;
                        ping_pong_2_index = ping_pong_2_index +(int)ps; // essentially gets every 5th value from where we left off
                    }
                    ping_pong_2_index = ping_pong_2_index - 1024; // Gets future index
                } else if (ps == 1) { // if ps = 1 get each individual element
                    while(big_two_index < 4096 && ping_pong_2_index < 1024) {
                        big_two[big_two_index] = bufferd[ping_pong_2_index];
                        big_two_index++;
                        ping_pong_2_index++;
                    }
                    ping_pong_2_index = ping_pong_2_index - 1024; // Gets future index
                } else if (ps < 1) { // if ps < 1 get every 1/ps element
                    //yplot_1[i] = data[(i * (int)ceil(1/ps)) + ((int)ceil(1/ps))];
                    if(ping_pong_2_index == 0 && run_2 == 0) {
                        ping_pong_2_index = ((int)ceil(1/ps));
                    }
                    while(big_two_index < 4096 && ping_pong_2_index < 1024) {
                        big_two[big_two_index] = bufferd[ping_pong_2_index];
                        big_two_index++;
                        ping_pong_2_index = ping_pong_2_index + ((int)ceil(1/ps));
                        
                    }
                    ping_pong_2_index = ping_pong_2_index - 1024;
                    run_2 = 1;    
                }
            }
    } else { // even if we arent reading data we still need to flip the buffers to prevent data loss/corruption
        if(current_two == 0) { // If buffer a was being filled set buffer b as buffer now being filled
            current_two = 1;
        } else {
            current_two = 0;
        }
    }
    if(big_two_index > 4095) { // if the big buffer is full
        big_two_index = 0;
        wait_two = 1; // Indicate we have data to be processed
        run_2 = 0;      // After this we are restarting the x scaling
        ping_pong_2_index = 0;
    }
    Cy_DMA_Channel_ClearInterrupt(DMA_2_HW, DMA_2_DW_CHANNEL); // Clear interrupt

    }



// This functions initializes the first dma for the first channel
void start_first_wave(){
    // Configure the dma channel
    cy_stc_dma_channel_config_t channelConfig;
    channelConfig.descriptor  = &DMA_1_Descriptor_1;
    channelConfig.preemptable = DMA_1_PREEMPTABLE;
    channelConfig.priority    = DMA_1_PRIORITY;
    channelConfig.enable      = false;
    channelConfig.bufferable  = DMA_1_BUFFERABLE;
    
    //Initializing descriptor 1(for buffer a)
    Cy_DMA_Descriptor_Init(&DMA_1_Descriptor_1,&DMA_1_Descriptor_1_config);
    Cy_DMA_Descriptor_SetSrcAddress(&DMA_1_Descriptor_1, (uint32_t *) &(SAR->CHAN_RESULT[0])); //Initializing the source from adc address
    Cy_DMA_Descriptor_SetDstAddress(&DMA_1_Descriptor_1, buffera); //and destination for descriptor 1
    
    //Initializing descriptor 2(for buffer b)
    Cy_DMA_Descriptor_Init(&DMA_1_Descriptor_2,&DMA_1_Descriptor_2_config);
    Cy_DMA_Descriptor_SetSrcAddress(&DMA_1_Descriptor_2, (uint32_t *) &(SAR->CHAN_RESULT[0])); //Initializing the source
    Cy_DMA_Descriptor_SetDstAddress(&DMA_1_Descriptor_2, bufferb); //and destination for descriptor 2
    
    
    //Initializing the dma
    Cy_DMA_Channel_Init(DMA_1_HW, DMA_1_DW_CHANNEL, &channelConfig);
    Cy_DMA_Channel_Enable(DMA_1_HW, DMA_1_DW_CHANNEL);
    Cy_DMA_Enable(DMA_1_HW);
    
    //Initializing the interrupt
    Cy_SysInt_Init(&DMA_1_INT_cfg, &DMA_1_ISR);
    NVIC_EnableIRQ(DMA_1_INT_cfg.intrSrc);
    Cy_DMA_Channel_SetInterruptMask(DMA_1_HW, DMA_1_DW_CHANNEL, CY_DMA_INTR_MASK);
    
    
    

}

// This functions initializes the second dma for the second channel
void start_second_wave(){
    // Configure the dma channel
    cy_stc_dma_channel_config_t channelConfig;
    channelConfig.descriptor  = &DMA_2_Descriptor_1;
    channelConfig.preemptable = DMA_2_PREEMPTABLE;
    channelConfig.priority    = DMA_2_PRIORITY;
    channelConfig.enable      = false;
    channelConfig.bufferable  = DMA_2_BUFFERABLE;
    
    //Initializing descriptor 1(for buffer c)
    Cy_DMA_Descriptor_Init(&DMA_2_Descriptor_1,&DMA_2_Descriptor_1_config);
    Cy_DMA_Descriptor_SetSrcAddress(&DMA_2_Descriptor_1, (uint32_t *) &(SAR->CHAN_RESULT[2])); //Initializing the source from adc address
    Cy_DMA_Descriptor_SetDstAddress(&DMA_2_Descriptor_1, bufferc); //and destination for descriptor 1
    
    //Initializing descriptor 2(for buffer d)
    Cy_DMA_Descriptor_Init(&DMA_2_Descriptor_2,&DMA_2_Descriptor_2_config);
    Cy_DMA_Descriptor_SetSrcAddress(&DMA_2_Descriptor_2, (uint32_t *) &(SAR->CHAN_RESULT[2])); //Initializing the source
    Cy_DMA_Descriptor_SetDstAddress(&DMA_2_Descriptor_2, bufferd); //and destination for descriptor 2
    
    
    //Initializing the dma
    Cy_DMA_Channel_Init(DMA_2_HW, DMA_2_DW_CHANNEL, &channelConfig);
    Cy_DMA_Channel_Enable(DMA_2_HW, DMA_2_DW_CHANNEL);
    Cy_DMA_Enable(DMA_2_HW);
    
    //Initializing the interrupt
    Cy_SysInt_Init(&DMA_2_INT_cfg, &DMA_2_ISR);
    NVIC_EnableIRQ(DMA_2_INT_cfg.intrSrc);
    Cy_DMA_Channel_SetInterruptMask(DMA_2_HW, DMA_2_DW_CHANNEL, CY_DMA_INTR_MASK);
    
    
    

}




int main(void)
{
    __enable_irq(); /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    /* Initialize EmWin Graphics */
    GUI_Init();
    /* Display background */
    drawBackground();
    /* Display grid*/
    drawGrid(XSIZE, YSIZE, XDIVISIONS, YDIVISIONS, XMARGIN, YMARGIN);
    
    /* Set font size */
    GUI_SetFont(GUI_FONT_32B_1);
    /* Set text color */
    GUI_SetColor(GUI_LIGHTGREEN);
    
    // Buffer for starting message on terminal indicating that they can type into the terminal
    char print_string[] = "Input command: \n\r";
    
    init_uart_printf(); // Initializes the uart
    uart_printf(print_string); // Prints the first welcome command
    start_first_wave(); // Initializes the first dma
    start_second_wave(); // Initializes the second dma
    
    // Starting the ADC
    ADC_1_Start();
    ADC_1_StartConvert();
    int first = 0; // Flag indicating the used has yet to type start so that nothing is displayed on the screen
    for(;;)
    {   
        //stop displaying
        if(start == 1) { // If we are in start mode
            first = 1; // indicate that the stopped state is ready to run
            while(wait_one == 0 || wait_two == 0) {} // Wait for both buffers to be filled 
            uint16_t result = ADC_1_GetResult16(1); //get the 16 bit result from the adc from 0 to 0xfff for the first channel
            potential_1 = (result / 26) + 5 ;// Allows for wave to move up and down within box(scaled to fit screen)
            uint16_t result_2 = ADC_1_GetResult16(3); //get the 16 bit result from the adc from 0 to 0xfff for the second channel
            potential_2 = (result_2 / 26) + 5 ;// Allows for wave to move up and down within box(scalled to fit screen)
            
            y_scaling(y_scale_1); // Yscales the values in the buffers as well as handles triggering
            
            // Calculate frequency
            frequency_finder();
            printFrequency();
            printScaleSettings();
            draw(); // Draw the waves
            wait_one = 0; // Tell Isrs that they can start storing data again
            wait_two = 0; // Tell isrs that they can start storing data again
        } else { // If we are in stop mode
            if (first == 1) {
                uint16_t result = ADC_1_GetResult16(1); //get the 16 bit result from the adc from 0 to 0xfff
                potential_1 = (result / 26) + 5 ;// Allows for wave to move up and down within box
                uint16_t result_2 = ADC_1_GetResult16(3); //get the 16 bit result from the adc from 0 to 0xfff
                potential_2 = (result_2 / 26) + 5 ;// Allows for wave to move up and down within box
            
                drawstopped(); // Function to draw stopped screen.
            }
        }
        for(int i = 0; i < 1000; i++){ // Do a 1000 us delay so that we can handle any terminal inputs
            // Do uart stuff
            recieve_data();
            Cy_SysLib_Delay(1);
        }
        
    }
}

/* [] END OF FILE */
