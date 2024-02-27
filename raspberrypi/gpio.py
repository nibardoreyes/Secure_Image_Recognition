#Name         :	Nibardo Reyes Felix
#Author       :	Me
#Version      :	3.0
#Date  	      : February 25, 2024
#Description  :	Milestone 3
#References   : https://learn.sparkfun.com/tutorials/raspberry-gpio/python-rpigpio-example


import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)
GPIO.setup(22, GPIO.OUT)  #GPIO 22 for synchronization signals
GPIO.setup(27, GPIO.OUT)  #GPIO 27 for sending character bits

def send_binary_message(message):
    #send synchronization signal (010101), make sure it matches de10
    for _ in range(3):  
        GPIO.output(22, GPIO.LOW)
        time.sleep(0.1)
        GPIO.output(22, GPIO.HIGH)
        time.sleep(0.1)


    #pi is now synchronized and will now read the bits being sent
    print("Sending Message now")  
    for char in message:
        #send each character in the message
        print(f"Sending binary value for character '{char}': ", end="")
        for j in range(8):
            value_to_send = (ord(char) >> (7 - j)) & 0x01
            print(value_to_send, end="")  #print the binary value for that character
            GPIO.output(27, value_to_send)
            time.sleep(1.0)           
        print()  
        
        
        
        #send termination signal (11111111)
        for _ in range(8):  
            GPIO.output(22, GPIO.HIGH)
            time.sleep(0.1)

# Function to receive the binary message
def receive_binary_message(gpio_pin):
    print(f"Waiting for synchronization signal on GPIO {gpio_pin}...")

    # Define synchronization sequence
    sync_sequence = [1] * 5  # Setting up the sequence we wait for (11111)
    sync_index = 0
    message = ""
    received_word = ""
    # Set up GPIO pins for input
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(gpio_pin, GPIO.IN)

    # Wait for synchronization signal
    while sync_index < len(sync_sequence):
        value_received = GPIO.input(gpio_pin)
        if value_received == sync_sequence[sync_index]:
            sync_index += 1
        else:
            sync_index = 0
        time.sleep(0.1)

    print("Synchronization signal detected")
    print("Starting message reception...")

        # Start looking at the signals being received
        # Also need to check if we are terminating the communication (may need to set up differently later)
    while not (GPIO.input(gpio_pin) == 0 and message.endswith("00000")):
        value_received = GPIO.input(gpio_pin)
        message += str(value_received)
        if len(message) == 8:
            char_value = chr(int(message, 2))
            received_word += char_value  # Save the characters received
            print("Received character:", char_value)  # Print each character (remove later)
            message = ""
        time.sleep(0.1)

    print("Received word:", received_word)  # Print the entire message received
    print("Termination signal received")


try:
    # Setup GPIO
    GPIO.setmode(GPIO.BCM)

    while True:
        # Wait for user input to choose GPIO pin
        print("Enter 1 to receive or 2 to send. Press Ctrl+C to exit.")
        choice = input()
        if choice == '1':
            receive_binary_message(17)
            break
        elif choice == '2':
            # Setup GPIO for sending message
            GPIO.setup(27, GPIO.OUT)
            message = "HelloWorld"
            send_binary_message(message)
            break
        else:
            print("Invalid choice. Please enter 1 or 2.")

except KeyboardInterrupt:
    pass  # Handle Ctrl+C gracefully

finally:
    GPIO.cleanup()
