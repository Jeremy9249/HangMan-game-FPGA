#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include "platform.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xstatus.h"
#include "xuartps.h"
#include "VGA.h"
#include "xil_mmu.h"
#include "xil_assert.h"

//USED wrapper_hw_platform_4
/* Definitions */
#define RBG_background_default 25
#define RGB_Noose 90
#define RGB_Person 50
#define RGB_Letter 100
#define GPIO_DEVICE_ID  XPAR_AXI_GPIO_0_DEVICE_ID    /* GPIO device that buttons are connected to */
#define CLK_COUNT 50000000
#define printf xil_printf                           /* smaller, optimised printf */
#define VGA_CONFIG_BASE_ADDRESS     0x43c00000      /* Control reg's for the VGA circuitry */
#define VGA_MEMORY_ATTRIBUTE 0x00010c06             /* Attribute applied to VGA frame buffer in DRAM. */
#define button_val XGpio_DiscreteRead(&BTNInst, 1)	/* Read from button input*/

XGpio BTNInst;

XUartPs Uart_Ps;        /* The instance of the UART Driver */
XUartPs_Config *Config;

vga vga_obj;
vga_frame vga_frame_obj;
vga_pixel vga_pixel_obj;

int w = 640, start_x= 279, start_y= 375, x, y;
int wrong, correct, seed_num;
char letter;
char str[5]; 		  //string or array that will store the input
char mystery_word[5]; //string or array that will store mystery word
int word_logic[5] = {0,0,0,0,0}; //each element represent if letter has been guessed right (yes=1, no=0)
char guessed_letters[26] = "                          ";

//ALL HEXADECIMAL SERIAL FOR THE MONITOR
int RGB_background = RBG_background_default;
void outputGUI(int x, int y);
void drawNoose(int x_counter,int y_counter);
void drawPerson(int x_counter,int y_counter, int wrong_count);
void drawBlanks(int x_counter,int y_counter);
void read_console();
void set_word(int seed);
void check_guessed();
void check_word();
void showLetter();
void blanks(int x_counter,int y_counter);
void PAUSE();
void RESET();
void showMESSAGE();

int main(){
	int Status;
    /* GPIO driver initialisation */
    Status = XGpio_Initialize(&BTNInst, GPIO_DEVICE_ID);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }
    XGpio_SetDataDirection(&BTNInst, 1, 0xFF);

     /* Configure VGA frame buffer memory to device. */
    Xil_SetTlbAttributes(VGA_FRAME_ADDRESS,VGA_MEMORY_ATTRIBUTE);
    /* Configure the vga object. */
    vga_setup(&vga_obj,(uint32_t*)VGA_CONFIG_BASE_ADDRESS,(vga_frame*)VGA_FRAME_ADDRESS);
    /* Clear the image frame, then draws a simple circle, then flushes the local buffer to VGA memory in DRAM for display. */
    vga_pixel_setup( &vga_pixel_obj, 0x4,0x4,0x4);
    vga_frame_clear( &vga_frame_obj);

    //Initialize UART
    Config = XUartPs_LookupConfig(UART_DEVICE_ID);
    XUartPs_CfgInitialize(&Uart_Ps, Config, Config->BaseAddress);
    XUartPs_SetBaudRate(&Uart_Ps, 115200);
    wrong = 0;
    correct = 0;
    printf("Please enter an integer > ");
	scanf(" %d", &seed_num);				//input used as seed for rand() function to generate a random number ~every time
    set_word(seed_num);
    while (1){					//Start and play game
    	for(y = 0; y < w;y++){	//create frame and display on monitor
    		for(x=0; x < w;x++){
    			outputGUI(x,y);
    		}
    	}
    	if (correct == 5 || wrong >= 6){ //Check win or lose, then instruct to restart if either
    		if (correct == 5)
    			printf("YOU WIN\n");
    		else if (wrong >= 6)
    			printf("GAMEOVER \n");
    		printf("PLEASE START NEW GAME BY PRESSING LEFT BUTTON\n");
    		while(1){
    			if (button_val == 4){
    				RESET();
    				printf("\n***NEW GAME STARTED***\n\n");
    				break;
    			}
    		}
        	for(y = 0; y < w;y++){	//create frame and display on monitor
        		for(x=0; x < w;x++){
        			outputGUI(x,y);
        		}
        	}
    	}
    	read_console(); //read console for input
    	if (strcmp(str,"pause")!=0 && strcmp(str,"PAUSE")!=0){
    		check_guessed();
    	}
    }
    return 0;
}

void read_console(){					//Read input from console: pause or first letter
	printf("Please enter a letter > ");
	scanf(" %s", &str);
	if (strcmp(str,"pause")==0||strcmp(str,"PAUSE")==0){
		PAUSE();
	} else {
		letter = tolower(str[0]);
		printf("\n");
	}
}

void set_word(int seed){		//Sets word for mystery word
	srand((unsigned)seed);
	int RandNum = rand()%12;
	switch(RandNum){
        case 0:
        	strcpy(mystery_word,"ergot"); //ergot for 0
        	break;
        case 1:
        	strcpy(mystery_word,"aglet");
        	break;
        case 2:
        	strcpy(mystery_word,"vapid");
        	break;
        case 3:
        	strcpy(mystery_word,"blood");
        	break;
        case 4:
        	strcpy(mystery_word,"winze");
        	break;
        case 5:
        	strcpy(mystery_word,"gumbo");
        	break;
        case 6:
        	strcpy(mystery_word,"chute");
        	break;
        case 7:
        	strcpy(mystery_word,"sixty");
        	break;
        case 8:
        	strcpy(mystery_word,"joule");
        	break;
        case 9:
        	strcpy(mystery_word,"forks");
        	break;
        case 10:
        	strcpy(mystery_word,"queen");
        	break;
        case 11:
        	strcpy(mystery_word,"xylem");
        	break;
	}
}

void check_guessed(){				//Check if letter has already been guessed
	for(int i = 0; i < 26; i=i+1){
		if (strcmp(guessed_letters[i],' ')==0){	//Has not been guessed yet, thus check if
			guessed_letters[i]=letter;			//in mystery word.
			check_word();
			break;
		} else if (strcmp(letter,guessed_letters[i])==0){		   //Has been guessed
			printf("Letter '%c' was already guessed. \n", letter); //No penalty
			break;
		}
	}
}

void check_word(){			//Check if mystery word has letter
	int in_word = 0;
	for(int i = 0; i < 5; i=i+1){
		if (strcmp(letter,mystery_word[i])==0){
			word_logic[i]=1;
			in_word = 1;
			correct = correct + 1;
		}
	}
	if (in_word == 0)
		wrong = wrong + 1;
}

void outputGUI(int x, int y){	//Creates and outputs frame/image
    drawNoose(x,y);
    drawPerson(x,y,wrong);
    blanks(x,y);
    showLetter();
    if (wrong >= 6){			//Display message if lose or win
    	showMESSAGE("gameover");
    } else if (correct == 5){
    	showMESSAGE("you  win");
    }
}

void drawNoose(int x_counter,int y_counter){				   // First function to be executed in outputGUI
	if(x_counter >= 0 && x_counter <= 39 && y_counter >= 119){ // Left stand of noose
	        vga_set_pixel( x_counter, y_counter, RGB_Noose);
	} else if(x_counter >= 0 && x_counter <= 159 && y_counter <= 159 && y_counter >=119){ // Top bar of noose
        vga_set_pixel( x_counter, y_counter, RGB_Noose);
	} else if(x_counter >= 119 && x_counter <= 143 && y_counter <= 199 && y_counter >=159){ // Rope of noose
        vga_set_pixel( x_counter, y_counter, RGB_Noose);
	} else {
        vga_set_pixel( x_counter, y_counter, 0); //Black background
    }
}

void drawPerson(int x_counter,int y_counter, int wrong_count){
	if(wrong_count >= 1 && x_counter >= 108 && x_counter <= 154 && y_counter <= 259 && y_counter >= 199){ // Head
			vga_set_pixel( x_counter, y_counter, RGB_Person);
	}
	if(wrong_count >= 2 && x_counter >= 108 && x_counter <= 154 && y_counter <= 345 && y_counter >= 265){ // Body
			vga_set_pixel( x_counter, y_counter, RGB_Person);
	}
	if(wrong_count >= 3 && x_counter >= 80 && x_counter <= 104 && y_counter <= 349 && y_counter >= 269){ // Left Arm
			vga_set_pixel( x_counter, y_counter, RGB_Person);
	}
	if(wrong_count >= 4 && x_counter >= 158 && x_counter <= 182 && y_counter <= 349 && y_counter >= 269){ // Right Arm
			vga_set_pixel( x_counter, y_counter, RGB_Person);
	}
	if(wrong_count >= 5 && x_counter >= 108 && x_counter <= 128 && y_counter <= 411 && y_counter >= 350){ // Left Leg
			vga_set_pixel( x_counter, y_counter, RGB_Person);
	}
	if(wrong_count >= 6 && x_counter >= 134 && x_counter <= 154 && y_counter <= 411 && y_counter >= 350){ // Right Leg
			vga_set_pixel( x_counter, y_counter, RGB_Person);
	}
}

//Generates character
void sendChar( int x_counter,int y_counter,char ChangedChar,int Colors,int y_max,int y_min,int x_max, int x_min){
    switch(ChangedChar){
        case 'a': // A
        if(x_counter > x_min && x_counter <x_min+8 && y_counter > y_min && y_counter<y_max){ //Left vertical
          		vga_set_pixel( x_counter, y_counter, Colors);
       		 }
        else if (x_counter > x_min+8 && x_counter<x_min+24 && y_counter> y_min && y_counter<y_min+8) // Top Horizontal S=2 W=4
        				vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+24 && y_counter> y_min+16 && y_counter<y_min+24) // Middle Horizontal S=3 W=4
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if(x_counter > x_min+24 && x_counter <x_min+32 && y_counter > y_min && y_counter<y_max) //Right vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing ? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);
                    }
        break;

        case 'b': // B
        if(x_counter > x_min && x_counter <x_min+8 && y_counter > y_min && y_counter<y_max){ //Left vertical
          			  vga_set_pixel( x_counter, y_counter, Colors);
       				 }
        else if (x_counter > x_min+8 && x_counter<x_min+24 && y_counter> y_min && y_counter<y_min+8) // Top Horizontal S=2 W=4
        				vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_min+8 && x_counter<x_min+24 && y_counter> y_max-8 && y_counter<y_max) // U bottom
        		vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+24 && y_counter> y_min+16 && y_counter<y_min+24) // Middle Horizontal S=3 W=4
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_min+8 && y_counter<y_min+16) // [4,2]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_max-16 && y_counter<y_max-8) // [4,4]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing ? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);
                    }
        break;

        case 'c': // C
                if (x_counter > x_min && x_counter<x_min+8 && y_counter> y_min+8 && y_counter<y_max-8) // Left vertical S=3
                	vga_set_pixel( x_counter, y_counter, Colors);
                else if (x_counter > x_min+8 && x_counter<x_max-8 && y_counter> y_min && y_counter<y_min+8) // S top
        			vga_set_pixel( x_counter, y_counter, Colors);
                else if (x_counter > x_min+8 && x_counter<x_max-8 && y_counter> y_max-8 && y_counter<y_max) // I bottom
        			vga_set_pixel( x_counter, y_counter, Colors);
                else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                                vga_set_pixel( x_counter, y_counter, 0);
                            }
				break;

        case 'd': // D
                if(x_counter > x_min && x_counter <x_min+8 && y_counter > y_min && y_counter<y_max){ //Left vertical
          			  vga_set_pixel( x_counter, y_counter, Colors);
       				 }
                else if (x_counter > x_min+8 && x_counter<x_min+24 && y_counter> y_min && y_counter<y_min+8) // Top Horizontal S=2 W=4
        				vga_set_pixel( x_counter, y_counter, Colors);
        		else if (x_counter >x_min+24 && x_counter<x_min+32 && y_counter> y_min+8 && y_counter<y_max-8) // J middle
        				vga_set_pixel( x_counter, y_counter, Colors);
      		    else if (x_counter >x_min+8 && x_counter<x_min+24 && y_counter> y_max-8 && y_counter<y_max) // U bottom
        		vga_set_pixel( x_counter, y_counter, Colors);
                else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing ? Make it black
                                vga_set_pixel( x_counter, y_counter, 0);
                            }
        break;

        case 'e': // E
                if(x_counter > x_min && x_counter <x_min+8 && y_counter > y_min && y_counter<y_max){ //Left vertical
          			  vga_set_pixel( x_counter, y_counter, Colors);
       				 }
                else if (x_counter > x_min+8 && x_counter<x_min+32 && y_counter> y_min+16 && y_counter<y_min+24) // Middle Horizontal S=3 W=4
        			vga_set_pixel( x_counter, y_counter, Colors);
                else if (x_counter > x_min && x_counter<x_max-8 && y_counter> y_min && y_counter<y_min+8) // Z top
        			vga_set_pixel( x_counter, y_counter, Colors);
    			else if(x_counter > x_min && x_counter <x_max-8 && y_counter > y_min+32 && y_counter< y_max) //Bottom horizontal
           			 vga_set_pixel( x_counter, y_counter, Colors);
                else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                                vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 'f': // F
                if(x_counter > x_min && x_counter <x_min+8 && y_counter > y_min && y_counter<y_max){ //Left vertical
          			  vga_set_pixel( x_counter, y_counter, Colors);
       				 }
                else if (x_counter > x_min+8 && x_counter<x_min+32 && y_counter> y_min+16 && y_counter<y_min+24) // Middle Horizontal S=3 W=4
        			vga_set_pixel( x_counter, y_counter, Colors);
                else if (x_counter > x_min && x_counter<x_max-8 && y_counter> y_min && y_counter<y_min+8) // Z top
        			vga_set_pixel( x_counter, y_counter, Colors);
                else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                                vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 'g': // G
        if (x_counter > x_min && x_counter<x_min+8 && y_counter> y_min+8 && y_counter<y_max-8) // Left vertical S=3
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+24 && y_counter> y_min && y_counter<y_min+8) // Top Horizontal S=2 W=4
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_min+16 && y_counter<y_min+32) // Right Vertical S=2
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if(x_counter > x_min+16 && x_counter < x_min+24 && y_counter > y_min+16 && y_counter<y_min+24) // [3,3]
        		vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_min+8 && x_counter<x_min+24 && y_counter> y_max-8 && y_counter<y_max) // U bottom
        		vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
                break;

        case 'h': // H
        if(x_counter > x_min && x_counter <x_min+8 && y_counter > y_min && y_counter<y_max){ //Left vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min+24 && x_counter <x_min+32 && y_counter > y_min && y_counter<y_max) //Right vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+24 && y_counter> y_min+16 && y_counter<y_min+24) // Middle Horizontal S=3 W=4
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);
        }
				break;

        case 'i': // I
        if (x_counter > x_min+8 && x_counter<x_max-8 && y_counter> y_min && y_counter<y_min+8) // S top
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_min+16 && x_counter<x_min+24 && y_counter> y_min && y_counter<y_max) // T Middle
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_max-8 && y_counter> y_max-8 && y_counter<y_max) // I bottom
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);
        }
				break;

        case 'j': // J
        if(x_counter > x_min+16 && x_counter <x_max && y_counter > y_min && y_counter<y_min+8){//Top J
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if (x_counter >x_min && x_counter<x_min+8 && y_counter> y_min+24 && y_counter<y_min+32) // [1,4]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_min+8 && x_counter<x_min+24 && y_counter> y_max-8 && y_counter<y_max) // U bottom
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_min+24 && x_counter<x_min+32 && y_counter> y_min+8 && y_counter<y_max-8) // J middle
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);
        }
        break;

        case 'k': //K
        if(x_counter >x_max-8 && x_counter<x_max && y_counter> y_max-8 && y_counter<y_max) // [5,5]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if(x_counter > x_min+8 && x_counter <x_min+16 && y_counter > y_min && y_counter<y_max){ //Left vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min+16 && x_counter < x_min+24 && y_counter > y_min+16 && y_counter<y_min+24) // [3,3]
        		vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_min+8 && y_counter<y_min+16) // [4,2]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_max-16 && y_counter<y_max-8) // [4,4]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_max-8 && x_counter<x_max && y_counter> y_min && y_counter<y_min+8) // [5,1]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);
        }
        break;

        case 'l': // L
        if(x_counter > x_min && x_counter <x_min+8 && y_counter > y_min && y_counter<y_max){ //Left vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min && x_counter <x_max-8 && y_counter > y_min+32 && y_counter< y_max){ //Bottom horizontal
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if (x_counter > x_min && x_counter< x_max && y_counter > y_min && y_counter<y_max){ //Nothing? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);
        }
				break;

        case 'm': //M
        if(x_counter > x_min && x_counter <x_min+8 && y_counter > y_min && y_counter<y_max){ //Left vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_max-8 && x_counter<x_max && y_counter> y_min && y_counter<y_max){ //Right vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min+8 && x_counter < x_min+16 && y_counter > y_min+8 && y_counter<y_min+16) // [2,2]
        		vga_set_pixel( x_counter, y_counter, Colors);
        else if(x_counter > x_min+16 && x_counter < x_min+24 && y_counter > y_min+16 && y_counter<y_min+24) // [3,3]
        		vga_set_pixel( x_counter, y_counter, Colors);
        else if(x_counter > x_min+24 && x_counter < x_min+32 && y_counter > y_min+8 && y_counter<y_min+16) // [4,2]
        		vga_set_pixel( x_counter, y_counter, Colors);
        else if(x_counter > x_max-8 && x_counter<x_max && y_counter> y_min && y_counter<y_max){ //Right vertical W=5
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if (x_counter > x_min && x_counter< x_max && y_counter > y_min && y_counter<y_max){ //Nothing? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);}
        break;

      	case 'n': //N
        if(x_counter > x_min && x_counter <x_min+8 && y_counter > y_min && y_counter<y_max){ //Left vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_max-16 && x_counter<x_max-8 && y_counter> y_min && y_counter<y_max) //Right vertical W=4
            vga_set_pixel( x_counter, y_counter, Colors);
        else if(x_counter > x_min+8 && x_counter < x_min+16 && y_counter > y_min+8 && y_counter<y_min+16) // [2,2]
        		vga_set_pixel( x_counter, y_counter, Colors);
        else if(x_counter > x_min+16 && x_counter < x_min+24 && y_counter > y_min+16 && y_counter<y_min+24) // [3,3]
        		vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min && x_counter< x_max && y_counter > y_min && y_counter<y_max){ //Nothing? Make it black
                        vga_set_pixel( x_counter, y_counter, 0); }
        break;

        case 'o': // O
        if (x_counter > x_min && x_counter<x_min+8 && y_counter> y_min+8 && y_counter<y_max-8) // Left vertical S=3
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_max-8 && x_counter<x_max && y_counter> y_min+8 && y_counter<y_max-8) // Right vertical S=3 W=5
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_max-8 && y_counter> y_min && y_counter<y_min+8) // Top Horizontal S=3 W=5
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_max-8 && y_counter> y_max-8 && y_counter<y_max) // Bottom Horizontal S=3 W=5
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 'p': // P
        if(x_counter > x_min && x_counter <x_min+8 && y_counter > y_min && y_counter<y_max){ //Left vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if (x_counter > x_min+8 && x_counter<x_max-16 && y_counter> y_min && y_counter<y_min+8) // Top Horizontal S=3 W=4
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+24 && y_counter> y_min+16 && y_counter<y_min+24) // Middle Horizontal S=3 W=4
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_min+8 && y_counter<y_min+16) // [4,2]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 'q': // Q
        if (x_counter > x_min && x_counter<x_min+8 && y_counter> y_min+8 && y_counter<y_max-8) // Left vertical S=3
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+24 && y_counter> y_min && y_counter<y_min+8) // Top Horizontal S=2 W=4
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_min+8 && y_counter<y_min+24) // Right Vertical S=2
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+16 && y_counter> y_min+32 && y_counter<y_max) // [2,5]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_min+32 && y_counter> y_min+32 && y_counter<y_max) // [4,5]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+16 && x_counter<x_min+24 && y_counter> y_min+24 && y_counter<y_max-8) // [3,4]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 'r': // R
        if(x_counter > x_min && x_counter <x_min+8 && y_counter > y_min && y_counter<y_max){ //Left vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if (x_counter > x_min+8 && x_counter<x_max-16 && y_counter> y_min && y_counter<y_min+8) // Top Horizontal S=3 W=4
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_max-16 && y_counter> y_min+16 && y_counter<y_min+24) // Middle Horizontal S=3 W=4
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_min+8 && y_counter<y_min+16) // [4,2]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_min+24 && y_counter<y_max) // [4,4]+[4,5]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 's': // S
        if (x_counter > x_min && x_counter<x_min+8 && y_counter> y_min+8 && y_counter<y_min+16) // S left
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_min+24 && y_counter<y_max-8) // S right
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_max-8 && y_counter> y_min && y_counter<y_min+8) // S top
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min && x_counter<x_max-16 && y_counter> y_max-8 && y_counter<y_max) // S bottom
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_max-16 && y_counter> y_min+16 && y_counter<y_min+24) // S Middle
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 't': // T
        if (x_counter >x_min+16 && x_counter<x_min+24 && y_counter> y_min && y_counter<y_max) // T Middle
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min && x_counter<x_max && y_counter> y_min && y_counter<y_min+8) // T top
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 'u': // U
        if (x_counter >x_min+8 && x_counter<x_min+24 && y_counter> y_max-8 && y_counter<y_max) // U bottom
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min && x_counter<x_min+8 && y_counter> y_min && y_counter<y_max-8) // U left
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_min+32 && y_counter> y_min && y_counter<y_max-8) // U right
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 'v': // V
        if (x_counter >x_min+16 && x_counter<x_min+24 && y_counter> y_max-8 && y_counter<y_max) // V bottom [3,5]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+16 && y_counter> y_min+16 && y_counter<y_max-8) // V left
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_min+32 && y_counter> y_min+16 && y_counter<y_max-8) // V right
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min && x_counter<x_min+8 && y_counter> y_min && y_counter<y_max-24) // V left 2
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+32 && x_counter<x_max && y_counter> y_min && y_counter<y_max-24) // V right 2
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 'w': // W
        if (x_counter >x_min+16 && x_counter<x_min+24 && y_counter> y_min+8 && y_counter<y_max-16) // W Middle [3,2] +[3,3]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+16 && y_counter> y_min+24 && y_counter<y_max) // W left
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_min+32 && y_counter> y_min+24 && y_counter<y_max) // W right
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min && x_counter<x_min+8 && y_counter> y_min && y_counter<y_max-16) // W left 2
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+32 && x_counter<x_max && y_counter> y_min && y_counter<y_max-16) // W right 2
        	vga_set_pixel( x_counter, y_counter, Colors);
        else  if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

      case 'x': // X
        if (x_counter >x_min && x_counter<x_min+8 && y_counter> y_min && y_counter<y_min+8) // [1,1]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_min && x_counter<x_min+8 && y_counter> y_max-8 && y_counter<y_max) // [1,5]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if(x_counter > x_min+16 && x_counter < x_min+24 && y_counter > y_min+16 && y_counter<y_min+24) // [3,3]
        		vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_min+8 && y_counter<y_min+16) // [4,2]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_max-16 && y_counter<y_max-8) // [4,4]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+16 && y_counter> y_max-16 && y_counter<y_max-8) // [2,4]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+16 && y_counter> y_min+8 && y_counter<y_min+16) // [2,2]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_max-8 && x_counter<x_max && y_counter> y_min && y_counter<y_min+8) // [5,1]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_max-8 && x_counter<x_max && y_counter> y_max-8 && y_counter<y_max) // [5,5]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 'y': // Y
        if (x_counter >x_min+16 && x_counter<x_min+24 && y_counter> y_min+16 && y_counter<y_max) // Y Middle
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_min && x_counter<x_min+8 && y_counter> y_min && y_counter<y_min+8) // [1,1]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_min+16 && y_counter> y_min+8 && y_counter<y_min+16) // [2,2]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_min+8 && y_counter<y_min+16) // [4,2]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_max-8 && x_counter<x_max && y_counter> y_min && y_counter<y_min+8) // [5,1]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

        case 'z': // Z
        if (x_counter > x_min && x_counter<x_max-8 && y_counter> y_min && y_counter<y_min+8) // Z top
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+8 && x_counter<x_max-16 && y_counter> y_min+16 && y_counter<y_min+24) // Middle Horizontal S=3 W=4
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min && x_counter<x_max-8 && y_counter> y_max-8 && y_counter<y_max) // Z bottom
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter > x_min+24 && x_counter<x_max-8 && y_counter> y_min+8 && y_counter<y_min+16) // [4,2]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter >x_min && x_counter<x_min+8 && y_counter> y_min+24 && y_counter<y_min+32) // [1,4]
        	vga_set_pixel( x_counter, y_counter, Colors);
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
        break;

    	}
    }

void showLetter(){ //display letter at location if guessed correctly
	for (int i=0;i<250;i=i+50){
		if (word_logic[i/50]==1)
      		sendChar( x, y, mystery_word[i/50], RGB_Letter, start_y, start_y-40, start_x+40+i, start_x+i);
	}
}

void showMESSAGE(char message[8]){ //Displays 8 character message at top of screen
	sendChar( x, y, message[0], RGB_Person, 110, 70, 169, 129);
	sendChar( x, y, message[1], RGB_Person, 110, 70, 219, 179);
	sendChar( x, y, message[2], RGB_Person, 110, 70, 269, 229);
	sendChar( x, y, message[3], RGB_Person, 110, 70, 319, 279);
	sendChar( x, y, message[4], RGB_Person, 110, 70, 369, 329);
	sendChar( x, y, message[5], RGB_Person, 110, 70, 419, 379);
	sendChar( x, y, message[6], RGB_Person, 110, 70, 469, 429);
	sendChar( x, y, message[7], RGB_Person, 110, 70, 519, 479);
}

 void blanks(int x_counter,int y_counter){ 		// draws blanks under letters
	 for (int index=0;index<250;index=index+50){
		 if(x_counter > start_x+index && x_counter <start_x+40+index && y_counter > start_y-5 && y_counter<start_y ){ // 5 lines
			 vga_set_pixel( x_counter, y_counter, RGB_Letter);
		 }
	 }
}

 void RESET(){
 	wrong = 0;
 	correct = 0;
 	strcpy(guessed_letters,"                          ");
 	word_logic[0]=0;
 	word_logic[1]=0;
 	word_logic[2]=0;
 	word_logic[3]=0;
 	word_logic[4]=0;
 	seed_num = seed_num+1;
 	set_word(seed_num);
 }

 void PAUSE(){
     for(y = 0; y < w;y++){
            for(x=0; x < w;x++){
                outputGUI(x,y);
                showMESSAGE(" paused ");
            }
     }
 	printf("\n********************************PAUSED********************************\n");
 	printf("PRESS LEFT BUTTON FOR RESET, CENTER FOR RESUME, AND RIGHT FOR GIVE UP.\n");
 	while(1){
 		if (button_val == 4){ //RESET
 			printf("\nRESET GAME \n\n");
 			RESET();
 			break;
 		} else if (button_val == 1){ //RESUME
 			printf("\nRESUME GAME \n\n");
 			break;
 		} else if (button_val == 8){ //GIVE UP = game over and display mystery word
 			printf("\nGIVE UP GAME \n\n");
 			wrong = 6;
 			word_logic[0]=1;
 			word_logic[1]=1;
 			word_logic[2]=1;
 			word_logic[3]=1;
 			word_logic[4]=1;
 			break;
 		}
 	}
     for(y = 0; y < w;y++){
            for(x=0; x < w;x++){
                outputGUI(x,y);
            }
     }
 }

