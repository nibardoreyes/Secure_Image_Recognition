
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
#include "application.h"
#include "hardware.h"
//===Macros====
#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
//===================
int open_physical (int fd);
void close_physical(int fd);
int unmap_physical(void *virtual_base, unsigned int span);
void displayMessageOnLCD(char* line1, char* line2, char* line3, char* line4);
void send_binary_message(char *message);
char* receive_binary_message1(void);
char test_circuit(char *message, int encrypt_decrypt);
//====================
// Define the base address and span for the lightweight bridge
#define LW_BRIDGE_BASE 0xFF200000
#define LW_BRIDGE_SPAN 0x00005000
//======================
//======================
//     Main
//======================
//======================
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

    //Hardware components
    gpioregister = (GpioRegister)(*data_reg);
    PushButton button;
    button.value = 0;
    Switches switch1;
    switch1.value = 0;
    switch1.value = *SW_ptr;

    int flag = 0;

    	//Clear the screen and ouput the intial message
    	displayMessageOnLCD("", "", "", "");
    	displayMessageOnLCD("Flip up", "Switch 0", "to get", "started");
        while (1) {
    	   //read the value of the switches in while loop to choose what to do
    	   switch1.value = *SW_ptr;
    	   button.value = *KEY_ptr;
           // Display initial message if all switches are down and it's the initial state
           if (switch1.bits.sw0 == 1) {
        	   if (flag == 0){
        		   displayMessageOnLCD("Pushbutton 0:", "E->FPGA->D", "Pushbutton: ", "D<-FPGA<-E");
        	   }
               	   	   if(button.bits.key0 == 1){
                   		   printf("Sending Encrypted Data to Pi\n");
                   		   displayMessageOnLCD("DE:Encrypted:a", "->FPGA:h", "->gpio->PI", "Decrypted:a");
                   		   char initial [1] = "a";
                   		   char new_char = test_circuit(initial, 1);//char is a and 1 means we are encrypting
                   		   char send_char[2] = {new_char, '\0'};
                   		   send_binary_message(send_char);
                   		   printf("Make a new choice\n");
                   		   flag = 0;

                   	   }else if(button.bits.key1 == 1){//this condition will keep things form clashing
                   		   printf("Receive Encrypted Data from Pi\n");
                   		   displayMessageOnLCD("DE:Decrypted:b", "<-b=FPGA", "<-gpio<-PI", "<-i-Encrypted:b");
                   		   char* encrypted_char=receive_binary_message1();
                   		   printf("Received message: %s\n", encrypted_char);
                   		   char decrypted_char = test_circuit(encrypted_char, 0);//put in the encrypted character and decrypt
                   		   printf("\nDecrypted char: %c", decrypted_char);
                   		   flag = 0;
                   	   }else {
                   		   flag = 1;
                   	   }

                   	usleep(100000);

        }else {
        		   flag = 0;
        }

}

    // Unmap the memory and close /dev/mem
    unmap_physical(LW_virtual, LW_BRIDGE_SPAN);

    close_physical(fd);

    return EXIT_SUCCESS;
}



