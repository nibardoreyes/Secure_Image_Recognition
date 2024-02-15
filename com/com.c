#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
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
        unsigned int gpio2 : 4;
        unsigned int gpio3 : 4;
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

//Function that sends the words across to the pi(Currently working perfectly)
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

    //Connecting to the gpio and setting it to an output
    gpio_ptr = (unsigned int *)(LW_virtual + JP1_BASE);
    *(volatile unsigned int *)(LW_virtual + JP1_BASE + 4) |= (1 << 0);

    //Initialize gpioregister
    gpioregister = (GpioRegister)(*gpio_ptr);



    // Send the binary message "hello" to GPIO pin 17
    send_binary_message("HelloWorld12345!");

    // Unmap the memory and close /dev/mem
    unmap_physical(LW_virtual, LW_BRIDGE_SPAN);

    close_physical(fd);

    return EXIT_SUCCESS;
}
