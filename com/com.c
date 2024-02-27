#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "./address_map_arm.h"
#include <sys/mman.h>

volatile unsigned int *gpio_ptr;
int value;

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
        unsigned int gpio4 : 4;
        unsigned int gpio5 : 4;
        unsigned int gpiou : 11;
    } bits;
} GpioRegister;

GpioRegister gpioregister;

// Open /dev/mem, if not already done, to give access to physical addresses
int open_physical(void)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1)
    {
        perror("Error opening /dev/mem");
        return -1;
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

//Function that sends the words across to the pi
//(Can be a hit or miss)
void send_binary_message(const char *message)
{
    //Send synchronization signal (11111)
    for (int i = 0; i < 5; ++i)
    {
        gpioregister.bits.gpio0 = 1;
        *gpio_ptr = gpioregister.value;
        usleep(100000); //100 milliseconds delay to match the PI (DO NOT TOUCH)
    }


    //PI is now synchronized and will now read the bits being sent
    printf("Sending Message now\n"); // Indicate that we are sending a message
    for (int i = 0; message[i] != '\0'; ++i)
    {
        //Send each character in the message
        char current_char = message[i];
        printf("Sending binary value for character '%c': ", current_char); //Print the character being sent
        for (int j = 0; j < 8; ++j)
        {
            int value_to_send = (current_char >> (7 - j)) & 0x01;
            printf("%d", value_to_send); //Print the binary value for that character
            gpioregister.bits.gpio0 = value_to_send;
            *gpio_ptr = gpioregister.value;
            usleep(100000); //100 milliseconds delay to match the PI (DO NOT TOUCH)
        }
        printf("\n"); // Move to the next line after sending each character
    }

    //Send termination signal
    for (int i = 0; i < 5; ++i)
    {
        gpioregister.bits.gpio0 = 0;
        *gpio_ptr = gpioregister.value;
        usleep(100000); //100 milliseconds delay to match the PI (DO NOT TOUCH)
    }
}

//Works great
//function that receives the binary message(just for testing)
void receive_binary_message(void)
{
    printf("Receiving Message now\n");

    //loop to receive bits
    while (1)
    {
        gpioregister = (GpioRegister)(*gpio_ptr);
        if (gpioregister.bits.gpio2 == 1)
        {
            printf("Received bit: 1\n");
        }
        else
        {
            printf("Received bit: 0\n");
        }

        usleep(100000);
    }
}

//function that checks for synchronization signal(Works perfectly)
int check_sync_signal(void)
{
    printf("Waiting for synchronization signal...\n");
    int sync_sequence[6] = {0, 1, 0, 1, 0, 1};
    int sync_index = 0;

    while (1)
    {
        gpioregister = (GpioRegister)(*gpio_ptr);
        usleep(100000); //delay matches pi code

        if (gpioregister.bits.gpio2 == sync_sequence[sync_index])
        {
            sync_index++;
            if (sync_index == 6)
            {
                printf("Synchronization signal detected\n");
                return 1;//exit loop and return 1
            }
        }
        else
        {
            sync_index = 0; //reset sync index if sequence is broken
        }
    }
}


//Works well(can be a hit or miss)
void receive_binary_message1(void) {
    if (!check_sync_signal()) {
        printf("Synchronization signal not detected\n");
        return; //exit function if synchronization signal not detected
    }

    printf("Starting message reception...\n");

    char message[100]; //array to store the received message characters
    int index = 0; //index to keep track of the current position in the message array

    unsigned char byte = 0; //variable to accumulate bits into a byte
    int bit_count = 0; //counter to keep track of the number of bits accumulated

    while (1) {
        GpioRegister gpioregister = (GpioRegister)(*gpio_ptr);
        int value_received = gpioregister.bits.gpio1; //read input from gpio1
        usleep(100000); //do not change this

        // Accumulate bits to form a byte
        byte = (byte << 1) | value_received;
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
            for (int i = 7; i >= 0; i--) {
                printf("%d", (byte >> i) & 1);
            }
            printf(" - Character: %c\n", byte);

            //reset for next byte
            byte = 0;
            bit_count = 0;

        }
        usleep(1000000);
    }

    message[index] = '\0';

    //print the complete message
    printf("Received message: %s\n", message);
}

int main()
{
    int fd = -1;         // used to open /dev/mem for access to physical addresses
    void *LW_virtual;    // used to map physical addresses for the light-weight bridge

    // Open /dev/mem to give access to physical addresses
    fd = open_physical();
    if (fd == -1)
        return (-1);

    // Create virtual memory access to the FPGA light-weight bridge
    LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    if (LW_virtual == NULL)
    {
        close_physical(fd);
        return (-1);
    }

    //Connecting to the gpio and setting gpio0 as an output
    gpio_ptr = (unsigned int *)(LW_virtual + JP1_BASE);
    *(volatile unsigned int *)(LW_virtual + JP1_BASE + 4) |= (1 << 0);

    //Initialize gpioregister
    gpioregister = (GpioRegister)(*gpio_ptr);



   int choice;

       while (1) {
           printf("How would you like communicate:\n");
           printf("1. Send binary message\n");
           printf("2. Receive binary message\n");
           printf("3. Exit\n");
           scanf("%d", &choice);

           switch (choice) {
               case 1:
                   send_binary_message("HelloWorld123");
                   break;
               case 2:
                   receive_binary_message1();
                   break;
               case 3:
                   printf("Exiting...\n");
                   return 0;
               default:
                   printf("Invalid choice. Choose from one of the options.");
           }

           printf("\nPress enter to make a new choice.\n");
           while (getchar() != '\n');
           getchar(); //wait for user to press enter
       }




    // Unmap the memory and close /dev/mem
    unmap_physical(LW_virtual, LW_BRIDGE_SPAN);

    close_physical(fd);

    return EXIT_SUCCESS;
}
