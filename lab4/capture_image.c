/*
ECE 354: Group 14C
Group Members: J Lagasse, A Sjogren, O Onyenokwe, L Wu
Notes:
1. Please leave swtiches SW1 and SW2 high during programming and during use.
2. Please toggle switches SW3-SW9 accordingly to perform the functions described in the case switches
*/

#include <time.h>								// import to use timestamp
#include <stdio.h>

#include "hps_soc_system.h"						// local imports
#include "hps.h"
#include "socal.h"

#define KEY_BASE              0xFF200050
#define SW_BASE				  0xFF200040		// location of first toggle switch
#define VIDEO_IN_BASE         0xFF203060		// control/flag register
#define FPGA_ONCHIP_BASE      0xC8000000		// pixel buffer
#define CHARACTER_BASE        0xC9000000		// character buffer
#define SDRAM_START		      0xC0000000


/* This program demonstrates the use of the D5M camera with the DE1-SoC Board
 * 	1. Capture one frame of video when any key is pressed.
 * 	2. Display the captured frame when any key is pressed.		  
*/
/* Note: Set the switches SW1 and SW2 to high and rest of the switches to low for correct exposure timing while compiling and the loading the program in the Altera Monitor program.
 * Note: KEY_ptr means address of KEY_ptr, while *KEY_ptr means value stored in KEY_ptr
*/


// initialize variables
volatile int * KEY_ptr = (int *) KEY_BASE;
volatile int * SW_ptr = (int *) SW_BASE;
volatile int * Video_In_DMA_ptr	= (int *) VIDEO_IN_BASE;
volatile short * Video_Mem_ptr	= (short *) FPGA_ONCHIP_BASE;
volatile short * Char_ptr = (short *) CHARACTER_BASE;
volatile int * SDRAM_start_ptr = (int *) SDRAM_START;

int global_x = 0;
int global_y = 0;
int print_x = 0;
int print_y = 0;
int fifof = 0;
int rready = 0;
int idata = 0;	// data
int SDRAM_count = 0;


char picture_array[40][240];	// holds 1 bit pixel color representations


int main(void){										// main method

	*(Video_In_DMA_ptr + 3)	= 0x4;					// Enable the video

	while (1){										// while loop
		
		switch(*SW_ptr){
			
			// PRINT MENU
			case 6:
				print_menu();
				break;
			
			// TAKE PICTURE
			case 14:								// Switch 3,2,1 high (Toggle 3)
				clear_char();						// clear text
				take_picture();						// take photo
				bw();								// convert to bw, store in array
//				print_array();
				while(*SW_ptr == 14){}				// While toggle 3 is high
//				resume_video();						// when no longer high, resume video
				black_screen();						// when no longer high, BLACK SCREEN so they know that code works
				break;
			
			// DISPLAY COMPRESSED PICTURE
			case 22:								// Switch 3,2,1 high (Toggle 3)
				clear_char();						// clear text
				take_picture();						// take photo
				compress_RLE();						// obtain data from image
				decompress_RLE();					// get decompressed data from SDRAM
				while(*SW_ptr == 22){}				// While toggle 3 is high
				resume_video();						// when no longer high, resume video
				break;

			default:								// for any other switch combo, print menu
//				resume_video();	
				break;
			
		}	// end case switch

	}	// end while loop

}	// end main method


//////////////////// Submethods


int print_menu(){			// prints menu options on video feed
	int x1 = 8;										// set coordinates
	int x2 = 8;
	int y1 = 10;
	int y2 = 11;
	int offset1, offset2, offset3, offset4, offset5, offset6; // set offset vars
	char *line1_ptr = "Welcome to Group 14C Demo!";	// set menu text
	char *line2_ptr = "Flip switch 3 to operate. ";
	/* Display a null-terminated text string at coordinates x, y. Assume that the text fits on one line */
	offset1 = (y1 << 7) + x1;
	offset2 = (y2 << 7) + x2;
	while ( *(line1_ptr) ){							// print menu text
		*(Char_ptr + offset1) = *(line1_ptr); 		// write to the character buffer line 1
		++line1_ptr;
		++offset1;
		*(Char_ptr + offset2) = *(line2_ptr); 		// write to the character buffer line 2
		++line2_ptr;
		++offset2;
	}
	return 0;
}

int clear_char(){			// clears all characters from screen
	int x=0;
	int y=0;
	int offset;
	char *clear_char_ptr = " ";
	for (y = 0; y < 59; y++) {
		for (x = 0; x < 79; x++) {
			offset = (y << 7) + x;
			*(Char_ptr+ offset) = *(clear_char_ptr); // clear 
		}
	}
	return 0;
}

int take_picture(){	// disables the video to capture one frame
	*(Video_In_DMA_ptr + 3) = 0x0;					// freeze frame
	return 0;
}

int resume_video(){	// enables the video, disbales frame capture
	*(Video_In_DMA_ptr + 3)	= 0x4;					// unfreezes frame
	return 0;
}

int black_screen(){
	int x,y;
	short sum = 0;
	short black = 0x0000;
	short* base_address = Video_Mem_ptr;
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 320; x++) {
			*(base_address + (y << 9) + x ) = black;
		}
	}
}

int bw(){
	int x,y;
	short sum = 0;
	short black = 0x0000; // BLACK IS 1
	short white = 0xFFFF;
	short* base_address = Video_Mem_ptr;
	int shift_count = 0;		// when shift count is 8, clear and send byte for compression
	
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 320; x++) {
			sum = (sum + *(base_address + (y << 9) + x )) / 2;
		}
	}
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 320; x++) {
			short value =  *(base_address + (y << 9) + x );
			
			if (value<sum){
				*(base_address + (y << 9) + x ) = white;
				picture_array[x/8][y] &= ~(1 << x%8);	// store 0

			}
			else{
				*(base_address + (y << 9) + x ) = black;
				picture_array[x/8][y] |= (1 << x%8);	// store 1
			}
		}
	}
	
	return 0;
}	// end of bw


///////////////////// NEW METHODS


int compress_RLE(){

	int bits = 0;
	global_x = 0;
	global_y = 0;
	print_x = 0;
	print_y = 0;
	fifof = 0;
	rready = 0;
	idata = 0;	// data
	SDRAM_count = 0;

	// set reset signal, then leave alone. Timing is fine bc ARM is pipeline, pipeline is longer than clock
	// RLE_RESET_PIO		Signal for initializing RLE encoder.  Assert and de-assert at the beginning of the program.
	alt_write_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + RLE_RESET_BASE, 1);	// set high
	alt_write_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + RLE_FLUSH_PIO_BASE, 1);		// mark end of file

	// set flush to low, not reached end of file yet
	// RLE_FLUSH_PIO		Used at the end of the bit-stream. RLE produces the final encoded data segment immediately with the last counting bit value.
	alt_write_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + RLE_FLUSH_PIO_BASE, 0);		// mark end of file
	alt_write_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + RLE_RESET_BASE, 0);	// set low

	while(global_x < 40 && global_y < 240 && bits < 76800){
	
		// READ INPUTS (3) SENT INTO ARM

		// FIFO_IN_FULL_PIO 		Indicates FIFO is full. Sending picture data stream should wait until this signal is de-asserted.
		fifof = alt_read_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + FIFO_IN_FULL_PIO_BASE);

		// RESULT_READY_PIO		Indicates that there is an encoded data segment in the FIFO. Note that this signal is active low since it is tied to the FIFO empty output.
		rready = alt_read_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + RESULT_READY_PIO_BASE);

//		printf("fifof:%d, rready:%d\n",fifof,rready);
		
		//////////////////////////////

		// DO STUFF HERE

		// if fifof is 1, do not send more data
		if(fifof == 0){		// if it is not full, send data
		
			// FIFO_IN_WRITE_REQ_PIO fifow	Asserted to write bitstream segment to FIFO in buffer. FIFO stores input data when this signal is asserted.
			alt_write_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + FIFO_IN_WRITE_REQ_PIO_BASE, 1);	
			
//			printf("Black=%d\n",picture_array[global_x][global_y]);
			
			alt_write_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + ODATA_PIO_BASE, picture_array[global_x][global_y]);
			increment_global_xy(); // update xy values for next address
			
			// FIFO_IN_WRITE_REQ_PIO fifow	Asserted to write bitstream segment to FIFO in buffer. FIFO stores input data when this signal is asserted.
			alt_write_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + FIFO_IN_WRITE_REQ_PIO_BASE, 0);	
			
//			printf("gx:%d, gy:%d\n",global_x,global_y);
		}
		if(rready == 0)	{// if data is ready to output

			// FIFO_OUT_READ_REQ_PIO fifor	Asserted when ARM wishes to read from the FIFO out. FIFO produces next data from the buffer when this signal is asserted.
			alt_write_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + FIFO_OUT_READ_REQ_PIO_BASE, 1);
	
//			idata = 0x0001FF;
			idata = alt_read_word(ALT_FPGA_BRIDGE_LWH2F_OFST + IDATA_PIO_BASE);
			
			// store to SDRAM
			*(SDRAM_start_ptr + SDRAM_count) = idata; // store SDRAM data
			SDRAM_count++; // increment SDRAM address
			bits = bits + (idata & 0x007FFFFF); // increment bits to make sure all output is read before exiting
//			printf("Bits:%d\n",bits);
//			printf("IDATA:%d\n",idata);
			
			// FIFO_OUT_READ_REQ_PIO fifor	Asserted when ARM wishes to read from the FIFO out. FIFO produces next data from the buffer when this signal is asserted.
			alt_write_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + FIFO_OUT_READ_REQ_PIO_BASE, 0);
			
//			printf("px:%d, py:%d\n",print_x,print_y);
		}

	} // run while loop ends

	// RLE_FLUSH_PIO flush		Used at the end of the bit-stream. RLE produces the final encoded data segment immediately with the last counting bit value.
	alt_write_byte(ALT_FPGA_BRIDGE_LWH2F_OFST + RLE_FLUSH_PIO_BASE, 1);		// mark end of file
	
	return 0;
} // end of compress

int increment_global_xy(){
	global_x = global_x + 1;
	if(global_x == 40 && global_y < 239){
		global_x = 0;
		global_y = global_y + 1;
	}
	else if(global_x == 40 && global_y == 239){
		global_y = global_y + 1;
	}
	return 0;
} 

int increment_print_xy(){
	print_x = print_x + 1;
	if(print_x == 320 && print_y < 239){
		print_x = 0;
		print_y = print_y + 1;
	}
	else if(print_x == 320 && print_y == 239){
		print_y = print_y + 1;
	}
	return 0;
} 

int decompress_RLE(){
	int count = 0;	// will count through all of SDRAM
	int bit_count = 0;
	short* base_address = Video_Mem_ptr;
	
	// COMPRESSION RATIO
	printf("COMPRESSED IMAGE SIZE: %d bytes\n", (3*SDRAM_count));
	printf("DECOMPRESSED IMAGE SIZE: %d bytes\n", 9600);
	printf("COMPRESSION RATIO: %f\n", 9600/(3*(float)SDRAM_count));
	
	for (count = 0; count < SDRAM_count; count++){ // Count until the end of SDRAM
		
		int color = (( *(SDRAM_start_ptr + count) >> 23) & 0x00000001); // 1 if black 0x0000, 0 if white 0xFFFF
		
		for(bit_count = 0; bit_count < ( *(SDRAM_start_ptr + count) & 0x007FFFFF); bit_count++){ // print total number of bits
			
			if(color == 1){
				*(base_address + (print_y << 9) + print_x -4) = 0x0000;
			}
			else{
				*(base_address + (print_y << 9) + print_x -4) = 0xFFFF;
			}	
			increment_print_xy();
		}
	}
	return 0;
}

int print_array(){	// use this to check if array is printing properly
					// White=1 means pixel is white
	int x,y;
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 40; x++) {
			printf("X=%i, Y=%i, Black=%d\n",x,y,picture_array[x][y]);
		}
	}
	return 0;
}
