
#include "lcd/terasic_os_includes.h"
#include "lcd/LCD_Lib.h"
#include "lcd/lcd_graphic.h"
#include "lcd/font.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"
#include "./address_map_arm.h"

/*
Macros and global variables are defined for memory mapping and interfacing
with hardware components like HEX displays, switches, keys, and GPIOs.
*/
//===Macros====
#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define JP1_CONTROL_REG_OFFSET 0x04

//============

void *virtual_base;
volatile unsigned int * JP1_ptr; //address pointer to the JP1 expansion ports
volatile signed int * SW_ptr;	///address pointer to the switches
volatile signed int *KEY_ptr;     // virtual address for the KEY port
int value;
volatile unsigned int *data_reg;
volatile unsigned int *direction_reg;
void displayMessageOnLCD(char* line1, char* line2, char* line3, char* line4);
LCD_CANVAS LcdCanvas;

// Define the base address and span for the lightweight bridge
#define LW_BRIDGE_BASE 0xFF200000
#define LW_BRIDGE_SPAN 0x00005000

typedef union {
    unsigned int value;
    struct
    {
        unsigned int gpio0 : 1;
        unsigned int gpio1 : 1;
        unsigned int gpio2 : 1;
        unsigned int gpio3 : 1;
        unsigned int gpio4 : 1;
        unsigned int gpio5 : 1;
        unsigned int gpio6 : 1;
        unsigned int gpio7 : 1;
        unsigned int gpio8 : 1;
        unsigned int gpio9 : 1;
        unsigned int gpio10 : 1;
        unsigned int gpio11 : 1;
        unsigned int gpio12 : 1;
        unsigned int gpio13 : 1;
        unsigned int gpio14 : 1;
        unsigned int gpio15 : 1;
        unsigned int gpio16 : 1;
        unsigned int gpio17 : 1;
        unsigned int gpio18 : 1;
        unsigned int gpio19 : 1;
        unsigned int gpio20 : 1;
        unsigned int gpio21 : 1;
        unsigned int gpio22 : 1;
        unsigned int gpio23 : 1;
        unsigned int gpio24 : 1;
        unsigned int gpio25 : 1;
        unsigned int gpiou : 5;
    } bits;
} GpioRegister;
GpioRegister gpioregister;

//Union representing the switches
typedef union{
	unsigned int value;
	struct{
		unsigned int sw0:1;	//lsb-switch on the left
		unsigned int sw1:1;
		unsigned int sw2:1;
		unsigned int sw3:1;
		unsigned int sw4:1;
		unsigned int sw5:1;
		unsigned int sw6:1;
		unsigned int sw7:1;
		unsigned int sw8:1;
		unsigned int sw9:1;
	}bits;
}Switches;

//Union for representing the pushbuttons
typedef union{
	unsigned int value;
	struct{
		unsigned int key0:1;
		unsigned int key1:1;
		unsigned int key2:1;
		unsigned int key3:1;

	}bits;
}PushButton;

// Open /dev/mem, if not already done, to give access to physical addresses
int open_physical (int fd)
{
   if (fd == -1)
      if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
      {
         printf ("ERROR: could not open \"/dev/mem\"...\n");
         return (-1);
      }
   return fd;
}

// Close /dev/mem to give access to physical addresses
void close_physical(int fd)
{
    close(fd);
}

// Establish a virtual address mapping for the physical addresses starting at base,
// and extending by span bytes.
void *map_physical(int fd, unsigned int base, unsigned int span)
{
    void *virtual_base = mmap(NULL, span, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, base);
    if (virtual_base == MAP_FAILED)
    {
        perror("ERROR: mmap() failed...");
        close(fd);
        return NULL;
    }
    return virtual_base;
}

// Close the previously-opened virtual address mapping
int unmap_physical(void *virtual_base, unsigned int span)
{
    if (munmap(virtual_base, span) != 0)
    {
        perror("ERROR: munmap() failed...");
        return -1;
    }
    return 0;
}

int check_sync_signal(void) {
    int i;
    while (1) { // Continuous attempt to synchronize
        for (i = 0; i < 1; ++i) { // Send the pattern 010101 three times
            // Send the pattern 010101
            if (gpioregister.bits.gpio2 == 0){
                gpioregister.bits.gpio2 = 1;
            } else {
                gpioregister.bits.gpio2 = 0;
            }
            gpioregister.bits.gpio21 = 1;
            *data_reg = gpioregister.value; // Apply the change to gpio2

            // Read the GPIO state after setting
            GpioRegister gpioregister = (GpioRegister)(*data_reg);
            printf("GPIO2 Sent: %d, GPIO22 Received: %d\n", gpioregister.bits.gpio2, gpioregister.bits.gpio22);
            usleep(25000);
            if (gpioregister.bits.gpio22 != gpioregister.bits.gpio2) {
                // Synchronization failed, return 0
            	//usleep(25000);
                return 0;
            }
        }
        // Synchronization succeeded after all iterations, return 1
        return 1;
    }
}



//Function that sends the words across to the pi
void send_binary_message(char *message)
{
    printf("Waiting to Sync....\n");
    int successful_checks = 0; // Initialize a counter for successful checks
    while(successful_checks < 6) { // Continue until we have 6 successful sync checks
        if(check_sync_signal()) {
            successful_checks++; // Increment counter on success
        } else {
            successful_checks = 0;
        }

    }
    int j;

    //PI is now synchronized and will now read the bits being sent
    printf("Sending Message now\n"); // Indicate that we are sending a message
    int i;
    for(i = 0; message[i] != '\0'; ++i) {
            char current_char = message[i];
            printf("Sending binary value for character '%c': ", current_char); // Print the character being sent
            for (j = 0; j < 8; ++j) {
            	if(check_sync_signal() && check_sync_signal()){
                int value_to_send = (current_char >> (7 - j)) & 0x01;
                //printf("%d", value_to_send); // Print the binary value for that character
                gpioregister.bits.gpio0 = value_to_send;
                *data_reg = gpioregister.value;
                usleep(25000); // 100 milliseconds delay to match the PI

        }else{
            	usleep(25000);
        }


    }
            printf("\n"); // Move to the next line after sending each character


}

    // Send termination signal
    for (i = 0; i <=7 ; ++i) {
    	if(check_sync_signal() && check_sync_signal()){
        gpioregister.bits.gpio0 = 1;
        *data_reg = gpioregister.value;
        usleep(25000); // 100 milliseconds delay to match the PI
    	}
    }
    printf("Termination Signal sent\n");

}



//Works well(can be a hit or miss)
char* receive_binary_message1(void) {
    char* message= malloc(100); //array to store the received message characters
    int index = 0; //index to keep track of the current position in the message array

    unsigned char byte = 0; //variable to accumulate bits into a byte
    int bit_count = 0; //counter to keep track of the number of bits accumulated
    printf("Waiting to Sync....\n");
    int successful_checks = 0; // Initialize a counter for successful checks
     while(successful_checks < 6) { // Continue until we have 6 successful sync checks
         if(check_sync_signal()) {
             successful_checks++; // Increment counter on success
         } else {
             successful_checks = 0;
         }

     }
    printf("Starting message reception...\n");

    while (1) {
    	if(check_sync_signal() && check_sync_signal()){

        GpioRegister gpioregister = (GpioRegister)(*data_reg);
        int value_received = gpioregister.bits.gpio25; //read input from gpio1
        usleep(25000);

        // Accumulate bits to form a byte
        byte = (byte << 1) | value_received;
        printf("received value: %d\n", value_received);

        bit_count++; // Increment bit counter

        if (bit_count == 8) {
            //check termination condition before adding the byte to message
        	//termination signal is (11111111)
            if (byte == 0xFF) {
                printf("Received termination sequence...\n");
                break; //exit loop if termination condition met
            }

            //store the byte in the message array and print it for debugging
            message[index++] = byte; //add the byte to the message array
            printf("Byte: ");
            int i;
            for(i = 7; i >= 0; i--) {
                printf("%d", (byte >> i) & 1);
            }
            printf(" - Character: %c\n", byte);

            //reset for next byte
            byte = 0;
            bit_count = 0;

        }

    	}else{

    	}
    	usleep(25000);
    }

    message[index] = '\0';

    //print the complete message
    printf("Received message: %s\n", message);
    return message;
}

//this function is strctly for testing the circuit
char test_circuit(char *message, int encrypt_decrypt){
	   gpioregister = (GpioRegister)(*data_reg);
	   *data_reg &= ~(0xFFFFFFFF);
	    *direction_reg |= (0xFF << 4);   // 0x3FF is for 10 bits, covering GPIO 4-11
	    *direction_reg |= ( 1 << 12);
	    //=====================
	    //END=================
	    //====================
    printf("Encrypting message\n");
    printf("Initial word: %s\n", message);
    unsigned char new_char;
    int zeroBit, oneBit, twoBit, threeBit, fourBit, fiveBit, sixBit, sevenBit;

    int i, j;
    for (i = 0; message[i] != '\0'; ++i) {
            char current_char = message[i];
            for (j = 0; j < 8; ++j) {
                sevenBit = (current_char >> (7 - 7)) & 0x01;
                sixBit = (current_char >> (7 -6)) & 0x01;
                fiveBit = (current_char >> (7 -5)) & 0x01;
                fourBit = (current_char >> (7 -4)) & 0x01;
                threeBit = (current_char >> (7 -3)) & 0x01;
                twoBit = (current_char >> (7 -2)) & 0x01;
                oneBit = (current_char >> (7 -1)) & 0x01;
                zeroBit = (current_char >> (7 -0)) & 0x01;

                gpioregister.bits.gpio4 = sevenBit;
                gpioregister.bits.gpio5 = sixBit;
                gpioregister.bits.gpio6 = fiveBit;
                gpioregister.bits.gpio7 = fourBit;
                gpioregister.bits.gpio8 = threeBit;
                gpioregister.bits.gpio9 = twoBit;
                gpioregister.bits.gpio10 = oneBit;
                gpioregister.bits.gpio11 = zeroBit;
                gpioregister.bits.gpio12 = encrypt_decrypt;
                gpioregister.bits.gpio21 = 1;

    }
            //keep this
            *data_reg = gpioregister.value;
            gpioregister = (GpioRegister)(*data_reg);

            printf("%c - 8 bits:%d%d%d%d%d%d%d%d\n", current_char, gpioregister.bits.gpio11, gpioregister.bits.gpio10, gpioregister.bits.gpio9, gpioregister.bits.gpio8, gpioregister.bits.gpio7, gpioregister.bits.gpio6, gpioregister.bits.gpio5, gpioregister.bits.gpio4);
            if(gpioregister.bits.gpio12 == 1){
            	printf("---Encrypting---\n");
            }else{
            	printf("---Decrypting---\n");
            }
			printf("%c - 8 bits:%d%d%d%d%d%d%d%d\n",current_char, gpioregister.bits.gpio20, gpioregister.bits.gpio19, gpioregister.bits.gpio18, gpioregister.bits.gpio17, gpioregister.bits.gpio16, gpioregister.bits.gpio15, gpioregister.bits.gpio14, gpioregister.bits.gpio13);
			printf("====================================\n");
		    // Combine the bits into a single character
		    new_char = (gpioregister.bits.gpio20 << 7) |
		                             (gpioregister.bits.gpio19 << 6) |
		                             (gpioregister.bits.gpio18 << 5) |
		                             (gpioregister.bits.gpio17 << 4) |
		                             (gpioregister.bits.gpio16 << 3) |
		                             (gpioregister.bits.gpio15 << 2) |
		                             (gpioregister.bits.gpio14 << 1) |
		                             (gpioregister.bits.gpio13);

		    // Print the new character
		    printf("New Character - %c\n", new_char);


}
    return new_char;

}


//===============

int main(int argc, char **argv)
{
    int fd = -1;         // used to open /dev/mem for access to physical addresses
    void *LW_virtual;    // used to map physical addresses for the light-weight bridge

    // Open /dev/mem to give access to physical addresses
    fd = open_physical(fd);
    if (fd == -1)
        return (-1);

    // Create virtual memory access to the FPGA light-weight bridge
    LW_virtual = mmap(NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (LW_virtual == MAP_FAILED) {
        perror("ERROR: mmap() failed...");
        close(fd);
        return -1;
    }
    // Pointers to GPIO data and direction registers
    SW_ptr = (signed int *)(LW_virtual + SW_BASE);//switch
    KEY_ptr =(signed int *)( LW_virtual + KEY_BASE);    // in it virtual address for KEY port
    data_reg = (volatile unsigned int *)(LW_virtual + JP1_BASE);
    direction_reg = (volatile unsigned int *)(LW_virtual + JP1_BASE + 4);
    *direction_reg |= (1 << 2);
    *direction_reg |= (1 << 0);

    ///==============================
    //Need this do not erase for LCD
    virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );
    //========================

    //GPIO register to control all data going into the gpio pins
    gpioregister = (GpioRegister)(*data_reg);

    PushButton button;
    button.value = 0;
    //switch control
    Switches switch1;
    switch1.value = 0;
    switch1.value = *SW_ptr;
    int flag = 1;
    //I want to output an intial message when all the switches are down to display a message

    	displayMessageOnLCD("", "", "", "");
    	displayMessageOnLCD("Switch 1:", "E->FPGA->D", "Switch 2: ", "D<-FPGA<-E");
        while (1) {
    	   //read the value of the switches in while loop to choose what to do
    	   switch1.value = *SW_ptr;
    	   button.value = *KEY_ptr;
           // Display initial message if all switches are down and it's the initial state
           if (switch1.bits.sw0 == 1) {
        	   if (flag == 0){
               displayMessageOnLCD("Switch 1:", "E->FPGA->D", "Switch 2: ", "D<-FPGA<-E");
               printf("\nNow flip a new switch\n");
        	   }else{
               flag = 0;  // Prevent the message from repeating without break
        	   }
           }
    	   if(button.bits.key0 == 1){
    		   printf("Sending Encrypted Data to Pi\n");
    		   displayMessageOnLCD("DE:Encrypted:a", "->FPGA=h", "->gpio->PI", "Decrypted:a");
    		   char new_char = test_circuit("a", 1);//char is a and 1 means we are encrypting
    		   char send_char[2] = {new_char, '\0'};
    		   send_binary_message(send_char);
    		   printf("Make a new choice\n");

    	   }else if(button.bits.key1 == 1){//this condition will keep things form clashing
    		   printf("Receive Encrypted Data from Pi\n");
    		   displayMessageOnLCD("DE:Decrypted:b", "<-b=FPGA", "<-gpio<-PI", "Encrypted:i");
    		   char* encrypted_char=receive_binary_message1();
    		   printf("Received message: %s\n", encrypted_char);
    		   char decrypted_char = test_circuit(encrypted_char, 0);//put in the encrypted character and decrypt
    		   printf("\nDecrypted char: %c", decrypted_char);
    	   }
    	flag = 1;
    	usleep(100000);
       }

    // Unmap the memory and close /dev/mem
    unmap_physical(LW_virtual, LW_BRIDGE_SPAN);

    close_physical(fd);

    return EXIT_SUCCESS;
}

//==========================================

//Function that initializes LCD and displays a message consisting of 4 lines
void displayMessageOnLCD(char* line1, char* line2, char* line3, char* line4) {
    // Create and initialize the LCD canvas
    LCD_CANVAS LcdCanvas;
    LcdCanvas.Width = LCD_WIDTH;
    LcdCanvas.Height = LCD_HEIGHT;
    LcdCanvas.BitPerPixel = 1;
    LcdCanvas.FrameSize = LcdCanvas.Width * LcdCanvas.Height / 8;
    LcdCanvas.pFrame = (void *)malloc(LcdCanvas.FrameSize);

    // Initialize LCD hardware
    LCDHW_Init(virtual_base);
    LCDHW_BackLight(true);

    LCD_Init();
    DRAW_Clear(&LcdCanvas, LCD_WHITE);
    DRAW_Rect(&LcdCanvas, 0, 0, LcdCanvas.Width - 1, LcdCanvas.Height - 1, LCD_BLACK);

        // Print the message on the LCD canvas
        DRAW_PrintString(&LcdCanvas, 5, 2, line1, LCD_BLACK, &font_16x16);
        DRAW_PrintString(&LcdCanvas, 5, 2+12, line2, LCD_BLACK, &font_16x16);
        DRAW_PrintString(&LcdCanvas, 5, 2+24, line3, LCD_BLACK, &font_16x16);
        DRAW_PrintString(&LcdCanvas, 5, 2+36, line4, LCD_BLACK, &font_16x16);
        DRAW_Refresh(&LcdCanvas);

        free(LcdCanvas.pFrame);
return;

}

