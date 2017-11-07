/*
ECE 354: Group 14C
Group Members: J Lagasse, A Sjogren, O Onyenokwe, L Wu
Notes:
1. Please leave swtiches SW1 and SW2 high during programming and during use.
2. Please toggle switches SW3-SW9 accordingly to perform the functions described in the case switches
*/

#include <time.h>								// import to use timestamp
#include <stdio.h>

#define KEY_BASE              0xFF200050
#define SW_BASE				  0xFF200040		// location of first toggle switch
#define VIDEO_IN_BASE         0xFF203060		// control/flag register
#define FPGA_ONCHIP_BASE      0xC8000000		// pixel buffer
#define CHARACTER_BASE        0xC9000000		// character buffer

int picture_counter = 0;

/* This program demonstrates the use of the D5M camera with the DE1-SoC Board
 * 	1. Capture one frame of video when any key is pressed.
 * 	2. Display the captured frame when any key is pressed.		  
*/
/* Note: Set the switches SW1 and SW2 to high and rest of the switches to low for correct exposure timing while compiling and the loading the program in the Altera Monitor program.
 * Note: KEY_ptr means address of KEY_ptr, while *KEY_ptr means value stored in KEY_ptr
*/


int main(void){										// main method
	
	// initialize variables
	volatile int * KEY_ptr = (int *) KEY_BASE;
	volatile int * SW_ptr = (int *) SW_BASE;
	volatile int * Video_In_DMA_ptr	= (int *) VIDEO_IN_BASE;
	volatile short * Video_Mem_ptr	= (short *) FPGA_ONCHIP_BASE;
	volatile short * Char_ptr	= (short *) CHARACTER_BASE;

	*(Video_In_DMA_ptr + 3)	= 0x4;					// Enable the video

	while (1){										// while loop
		
		switch(*SW_ptr){
			
			// TAKE PICTURE
			case 14:								// Switch 3,2,1 high (Toggle 3)
				picture_counter++;					// increment picture counter by 1
				clear_char(Char_ptr);				// clear text
				take_picture(Video_In_DMA_ptr);		// take photo
				while(*SW_ptr == 14){}				// While toggle 3 is high
				resume_video(Video_In_DMA_ptr);
				break;
				
			// MIRROR
			case 22:								// Switch 4,2,1 high (Toggle 4)
				picture_counter++;					// increment picture counter by 1
				clear_char(Char_ptr);				// clear text
				take_picture(Video_In_DMA_ptr);		// take photo
				mirror_horizontal(Video_Mem_ptr);
				while(*SW_ptr == 22){}				// While toggle 4 is high					
				resume_video(Video_In_DMA_ptr);
				break;
				
			// ROTATE
			case 38:								// Switch 5,2,1 high (Toggle 5)
				picture_counter++;					// increment picture counter by 1
				clear_char(Char_ptr);				// clear text
				take_picture(Video_In_DMA_ptr);		// take photo
				mirror_vertical(Video_Mem_ptr);
				while(*SW_ptr == 38){}				// While toggle 5 is high					
				resume_video(Video_In_DMA_ptr);
				break;
				
			// TIMESTAMP
			case 70:								// Switch 6,2,1 high (Toggle 6)
				picture_counter++;					// increment picture counter by 1
				clear_char(Char_ptr);				// clear text
				take_picture(Video_In_DMA_ptr);		// take photo
				timestamp(Char_ptr);
				while(*SW_ptr == 70){}				// While toggle 6 is high
				clear_char(Char_ptr);				// clear text
				resume_video(Video_In_DMA_ptr);
				break;
				
			// COUNTER
			case 134:								// Switch 7,2,1 high (Toggle 7)
				picture_counter++;					// increment picture counter by 1
				clear_char(Char_ptr);				// clear text
				take_picture(Video_In_DMA_ptr);		// take photo
				display_counter(Char_ptr, picture_counter);
				while(*SW_ptr == 134){}				// While toggle 7 is high, hold
				clear_char(Char_ptr);
				resume_video(Video_In_DMA_ptr);
				break;
				
			// GRAYSCALE
			case 262:								// Switch 8,2,1 high (Toggle 8)
				picture_counter++;					// increment picture counter by 1
				clear_char(Char_ptr);				// clear text
				take_picture(Video_In_DMA_ptr);		// take photo
				grayscale(Video_Mem_ptr);
				while(*SW_ptr == 262){}				// While toggle 8 is high
				resume_video(Video_In_DMA_ptr);
				break;
				
			// INVERT
			case 518:								// Switch 9,2,1 high (Toggle 9)
				picture_counter++;					// increment picture counter by 1
				clear_char(Char_ptr);				// clear text
				take_picture(Video_In_DMA_ptr);		// take photo
				//bw(Video_Mem_ptr);
				invert(Video_Mem_ptr);
				while(*SW_ptr == 518){}				// While toggle 9 is high
				resume_video(Video_In_DMA_ptr);
				break;
				
			// BONUS
			case 7:									// Switch 0,1,2 high (Toggle 0)
				while(*SW_ptr == 7){
					
					if (*KEY_ptr == 1){				// Key 0 Edge Detector
						picture_counter++;					// increment picture counter by 1
						take_picture(Video_In_DMA_ptr);		// take photo
						clear_char(Char_ptr);				// clear text
						edge_det(Video_Mem_ptr);
						while(*KEY_ptr == 1){}				// While toggle key pressed
						resume_video(Video_In_DMA_ptr);
					}
					
					if (*KEY_ptr == 2){				// Key 2 Pencil Sketch
						picture_counter++;					// increment picture counter by 1
						take_picture(Video_In_DMA_ptr);		// take photo
						clear_char(Char_ptr);				// clear text
						grayscale(Video_Mem_ptr);
						blur_filter(Video_Mem_ptr);
						invert(Video_Mem_ptr);
						while(*KEY_ptr == 4){}				// While key pressed
						resume_video(Video_In_DMA_ptr);
					}
				break;
		
			default:								// for any other switch combo, print menu
				print_menu(Char_ptr);
				break;
			
		}	// end case switch

	}	// end while loop

}	// end main method


//////////////////// Submethods


int print_menu(volatile short * Char_ptr){			// prints menu options on video feed
	int x1 = 8;										// set coordinates
	int x2 = 8;
	int x3 = 9;
	int x4 = 9;
	int x5 = 9;
	int x6 = 9;
	int y1 = 10;
	int y2 = 11;
	int y3 = 13;
	int y4 = 14;
	int y5 = 15;
	int y6 = 16;
	int offset1, offset2, offset3, offset4, offset5, offset6; // set offset vars
	char *line1_ptr = "Welcome to Group 14C Demo!";	// set menu text
	char *line2_ptr = "Flip switches to operate. ";
	char *line3_ptr = "3 Take picture, 4 Mirror  ";
	char *line4_ptr = "5 Rotate, 6 Timestamp     ";
	char *line5_ptr = "7 Counter, 8 Grayscale    ";
	char *line6_ptr = "9 Invert, 0 Bonus         ";
	/* Display a null-terminated text string at coordinates x, y. Assume that the text fits on one line */
	offset1 = (y1 << 7) + x1;
	offset2 = (y2 << 7) + x2;
	offset3 = (y3 << 7) + x3;
	offset4 = (y4 << 7) + x4;
	offset5 = (y5 << 7) + x5;
	offset6 = (y6 << 7) + x6;
	while ( *(line1_ptr) ){							// print menu text
		*(Char_ptr + offset1) = *(line1_ptr); 		// write to the character buffer line 1
		++line1_ptr;
		++offset1;
		*(Char_ptr + offset2) = *(line2_ptr); 		// write to the character buffer line 2
		++line2_ptr;
		++offset2;
		*(Char_ptr + offset3) = *(line3_ptr); 		// write to the character buffer line 3
		++line3_ptr;
		++offset3;
		*(Char_ptr + offset4) = *(line4_ptr); 		// write to the character buffer line 4
		++line4_ptr;
		++offset4;
		*(Char_ptr + offset5) = *(line5_ptr); 		// write to the character buffer line 5
		++line5_ptr;
		++offset5;
		*(Char_ptr + offset6) = *(line6_ptr); 		// write to the character buffer line 6
		++line6_ptr;
		++offset6;
	}
	return 0;
}

int clear_char(volatile short * Char_ptr){			// clears all characters from screen
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

int take_picture(volatile int * Video_In_DMA_ptr){	// disables the video to capture one frame
	*(Video_In_DMA_ptr + 3) = 0x0;					// freeze frame
	return 0;
}

int resume_video(volatile int * Video_In_DMA_ptr){	// enables the video, disbales frame capture
	*(Video_In_DMA_ptr + 3)	= 0x4;					// unfreezes frame
	return 0;
}

int mirror_horizontal(volatile int * Video_Mem_ptr){// "mirror" in demo, mirror horizontally
	int x,y;
	short* base_address = Video_Mem_ptr;
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 159; x++) {
			long temp1 = *(base_address + (y << 9) + x );
			long temp2 = *(base_address + (y << 9) + (319-x));
			*(base_address + (y << 9) + x ) = temp2;
			*(base_address + (y << 9) + (319-x)) = temp1;
		}
	}
	return 0;
}

int mirror_vertical(volatile int * Video_Mem_ptr){	// "rotate" in demo, mirror vertically or flip 180 deg
	int x,y;
	short* base_address = Video_Mem_ptr;
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 159; x++) {
			long temp1 = *(base_address + (y << 9) + x );
			long temp2 = *(base_address + (y << 9) + (319-x));
			*(base_address + (y << 9) + x ) = temp2;
			*(base_address + (y << 9) + (319-x)) = temp1;
		}
	}
	for (y = 0; y < 119; y++) {
		for (x = 0; x < 320; x++) {
			long temp3 = *(base_address + (y << 9) + x );
			long temp4 = *(base_address + ((239 - y) << 9) + x ); 
			*(base_address + (y << 9) + x ) = temp4;
			*(base_address + ((239 - y) << 9) + x ) = temp3;	
		}
	}						
	return 0;
}

int timestamp(volatile short * Char_ptr){			// print timestamp
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	tm.tm_hour = tm.tm_hour - 5;
	int x = 8;
	int y = 13;
	int offset;
	char *timestamp_ptr = asctime(&tm);
	offset = (y << 7) + x;
	while ( *(timestamp_ptr) ){
		*(Char_ptr + offset) = *(timestamp_ptr); 
		++timestamp_ptr;
		++offset;
	}
	return 0;
}

int display_counter(volatile short * Char_ptr, int picture_counter){	// display the value stored in the counter
	// try spritf to get rid of null pointer extra character
	int x = 15;
	int y = 13;
	int num_offset;
	char array[16];
	snprintf(array, sizeof(array), "Count: %i\n", picture_counter);
	char *num_ptr = &array;
	num_offset = (y << 7) + x;
	while ( *(num_ptr) ){
		*(Char_ptr + num_offset) = *(num_ptr); 
		++num_ptr;
		++num_offset;
	}
	return 0;
}

int grayscale(volatile int * Video_Mem_ptr){		// grayscale
	int x,y;
	short* base_address = Video_Mem_ptr;
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 320; x++) {
			long pixel_ptr = *(base_address + (y << 9) + x );
			unsigned int blue = ( pixel_ptr & 0x1F );
			unsigned int green = ( ( pixel_ptr >> 6 ) & 0x1F );
			unsigned int red = ( ( pixel_ptr >> 11 ) & 0x1F );
			unsigned int average = (blue+red+green)/3;
			*(base_address + (y << 9) + x ) = average + (average<<6) + (average<<11);
		}
	}
	return 0;
}

int invert(volatile int * Video_Mem_ptr){			// inverts colors from grayscale
	int x,y;
	short* base_address = Video_Mem_ptr;
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 320; x++) {
			long pixel_ptr = *(base_address + (y << 9) + x );
			unsigned int blue = ( pixel_ptr & 0x1F );
			unsigned int green = ( ( pixel_ptr >> 6 ) & 0x1F );
			unsigned int red = ( ( pixel_ptr >> 11 ) & 0x1F );
			unsigned int average = (blue+red+green)/3;
			average = ~average;
			*(base_address + (y << 9) + x ) = average + (average<<6) + (average<<11);
		}
	}
	return 0;
}

int blur_filter(volatile int * Video_Mem_ptr){			// gaussian filter to blur
	int x,y;
	double blur_corner = 0.01; 	// gaussian 0.01
	double blur_edge = 0.08; 	// gaussian 0.08
	double blue_center = 0.64; 	// gaussian 0.64
	short* base_address = Video_Mem_ptr;
	for (y = 1; y < 239; y++) {
		for (x = 1; x < 319; x++) {
			
			// row 1
			long r1c1 = *(base_address + ((y-1) << 9) + (x-1) );
			long r1c2 = *(base_address + ((y-1) << 9) + x );
			long r1c3 = *(base_address + ((y-1) << 9) + (x+1) );
			// row 2
			long r2c1 = *(base_address + (y << 9) + (x-1) );
			long r2c2 = *(base_address + (y << 9) + x );
			long r2c3 = *(base_address + (y << 9) + (x+1) );
			// row 3
			long r3c1 = *(base_address + ((y+1) << 9) + (x-1) );
			long r3c2 = *(base_address + ((y+1) << 9) + x );
			long r3c3 = *(base_address + ((y+1) << 9) + (x+1) );
			
			long blur = (int)(r1c1*blur_corner+r1c2*blur_edge+r1c3*blur_corner+r2c1*blur_edge+r2c2*blue_center+r2c3*blur_edge+r3c1*blur_corner+r3c2*blur_edge+r3c3*blur_corner);
			
			*(base_address + (y << 9) + x ) = blur;
		}
	}
	return 0;
}

int edge_det(volatile int * Video_Mem_ptr){			// gaussian filter to blur
	int x,y;
	short* base_address = Video_Mem_ptr;
	short array[319][239];
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 320; x++) {
			array[x][y] = *(base_address + (y << 9) + x );
		}
	}

	bw(base_address);
	short hit_color = 0x6666; 
	unsigned int cell[8];
	
	for (y = 1; y < 239; y++) {
		for (x = 1; x < 319; x++) {
			
			// row 1
			cell[0] = *(base_address + ((y-1) << 9) + (x-1) );
			cell[1] = *(base_address + ((y-1) << 9) + x );
			cell[2] = *(base_address + ((y-1) << 9) + (x+1) );
			// row 2
			cell[3] = *(base_address + (y << 9) + (x-1) );
			cell[4] = *(base_address + (y << 9) + x );
			cell[5] = *(base_address + (y << 9) + (x+1) );
			// row 3
			cell[6] = *(base_address + ((y+1) << 9) + (x-1) );
			cell[7] = *(base_address + ((y+1) << 9) + x );
			cell[8] = *(base_address + ((y+1) << 9) + (x+1) );
			
			if ( cell[0]!=cell[4] || cell[1]!=cell[4] || cell[2]!=cell[4] || cell[3]!=cell[4] || cell[5]!=cell[4] || cell[6]!=cell[4] || cell[7]!=cell[4] || cell[8]!=cell[4] ){	
			}
			else{
				array[x][y] = hit_color;
			}			
		}
	}
	*(base_address + (y << 9) + x ) = array[1][1];
/*	
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 320; x++) {
			*(base_address + (y << 9) + x ) = array[x][y];
		}
	}
*/
	return 0;

}

int bw(volatile int * Video_Mem_ptr){			// gaussian filter to blur
	int x,y;
	short sum = 0;
	short black = 0xFFFF; 	// 
	short white = 0x0000; 	// 
	short* base_address = Video_Mem_ptr;
	
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 320; x++) {
			sum = (sum + *(base_address + (y << 9) + x )) / 2;
		}
	}
	
	for (y = 0; y < 240; y++) {
		for (x = 0; x < 320; x++) {
			short value =  *(base_address + (y << 9) + x );
			if (value<sum){
				*(base_address + (y << 9) + x ) = black;
			}
			else{
				*(base_address + (y << 9) + x ) = white;
			}
		}
	}
	
	return 0;
}