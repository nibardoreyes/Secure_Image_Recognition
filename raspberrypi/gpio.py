#Name         :	Nibardo Reyes Felix
#Author       :	Me
#Version      :	2.0
#Date  	      : February 11, 2024
#Description  :	Milestone 2
#References   : https://learn.sparkfun.com/tutorials/raspberry-gpio/python-rpigpio-example

import RPi.GPIO as GPIO
import time
#Usiong gpio_pin 17 on the pi
def receive_binary_message(gpio_pin=17):
    print("Waiting for synchronization signal...")

    # Define synchronization sequence
    sync_sequence = [1] * 5  #setting up thje sequence we wait for (11111)
    sync_index = 0
    message = ""
    received_word = ""

    try:
        #Set up gpio pins for input
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(gpio_pin, GPIO.IN)

        #wait for synchronization signal
        while sync_index < len(sync_sequence):
            value_received = GPIO.input(gpio_pin)
            if value_received == sync_sequence[sync_index]:
                sync_index += 1
            else:
                sync_index = 0
            time.sleep(0.1)

        print("Synchronization signal detected")
        print("Starting message reception...")

        #Start looking at the signals being recieived
        #Also need to check if we are terminating the communication(may need to set up differenly later)
        while not (GPIO.input(gpio_pin) == 0 and message.endswith("00000")):
            value_received = GPIO.input(gpio_pin)
            message += str(value_received)
            if len(message) == 8:
                char_value = chr(int(message, 2))
                received_word += char_value  #save the characters received
                print("Received character:", char_value)  #print each character(remove later)
                message = ""
            time.sleep(0.1)

        print("Received word:", received_word)  #print the entire messafe received
        print("Termination signal received")

    except KeyboardInterrupt:
        pass

    finally:
        GPIO.cleanup()

try:
    receive_binary_message()

except KeyboardInterrupt:
    pass  # Handle Ctrl+C gracefully
