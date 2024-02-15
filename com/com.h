/*
 * com.h
 *
 *  Created on: Feb 14, 2024
 *      Author: Nibardo Reyes
 */

#ifndef COM_H_
#define COM_H_

//==============
// Function that continuously monitors GPIO input and displays the bits
void receive_binary_message(void)
{
    printf("Receiving Message now\n"); // Indicate that we are receiving a message
    while (1) {
        char received_bit = gpioregister.bits.gpio1; // Read GPIO pin value
        printf("%d", received_bit); // Print the received bit
        fflush(stdout); // Flush the output buffer to ensure immediate display
        usleep(100000); // Delay to match the PI (DO NOT TOUCH)
    }
}





#endif /* COM_H_ */
