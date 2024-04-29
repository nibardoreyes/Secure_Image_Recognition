#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "hardware.h"

volatile unsigned int *data_reg;
volatile unsigned int *direction_reg;
void *virtual_base;
volatile unsigned int * JP1_ptr; //address pointer to the JP1 expansion ports
volatile signed int * SW_ptr;	///address pointer to the switches
volatile signed int *KEY_ptr;     // virtual address for the KEY port
int value;



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

//=========
//Synchronization is done using 2 GPIO pins,
//One pin sends data across while the other receives data.
//The other device does the same and the Pi adjust itself to
//match the DE-10's signals as a communication protocol.
//=========
int check_sync_signal(void) {
    int i;
    while (1) { // Continuous attempt to synchronize
        for (i = 0; i < 1; ++i) {
            if (gpioregister.bits.gpio2 == 0){
                gpioregister.bits.gpio2 = 1;
            } else {
                gpioregister.bits.gpio2 = 0;
            }
            gpioregister.bits.gpio21 = 1;
            *data_reg = gpioregister.value; // Apply the change

            // Read the GPIO state after setting
            GpioRegister gpioregister = (GpioRegister)(*data_reg);
            printf("GPIO2 Sent: %d, GPIO22 Received: %d\n", gpioregister.bits.gpio2, gpioregister.bits.gpio22);
            usleep(25000);
            if (gpioregister.bits.gpio22 != gpioregister.bits.gpio2) {
                return 0;
            }
        }

        return 1;
    }
}
//============
//Function that sends data across the GPIO pins.
//First function first goes through initial synchronization
//to determine if the other device is synchronized.
//Once initial synchronization is made, we start sending each bit of
//the character across the GPIO one by one while also checking to
//see if the devices on synced. Once all the bits have been sent, a termination signal
//is sent to signal to the other device that the message is complete.
//============
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
    printf("Sending Message now\n"); // Indicate that we are sending a message in console
    int i;
    for(i = 0; message[i] != '\0'; ++i) {
            char current_char = message[i];
            printf("Sending binary value for character '%c': ", current_char); // Print the character being sent
            for (j = 0; j < 8; ++j) {
            	if(check_sync_signal() && check_sync_signal()){
                int value_to_send = (current_char >> (7 - j)) & 0x01;
                //printf("%d", value_to_send); // Print the binary value for that character(for debugging)
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
        usleep(25000); //small delay to match pi
    	}
    }
    printf("Termination Signal sent\n");

}


//================
//Receive binary function is similar to sending binary function.
//I start but ensuring we are synced in the very beginning.
//Once synced we read each bit coming in through the GPIO
//until we received the termination signal. Every 8 bits
//will be converted into it's respective character.
//=================
char* receive_binary_message1(void) {
    char* message= malloc(100); //array to store the received message characters
    int index = 0; //index to keep track of the current position in the message array
    unsigned char byte = 0; //variable to turn bits into a byte
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

//=========================
//This function is used to encrypt or decrypt characters
//through the circuit that was designed. This is done through the
//FPGA and is able to encrypt or decrypt any lowercase character.
//========================
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

//===============

// Close /dev/mem to give access to physical addresses
void close_physical(int fd)
{
    close(fd);
}

//==============
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
//=========
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










#endif
