#Name         :	Nibardo Reyes Felix
#Author       :	Me
#Version      :	4.0
#Date  	      : April 21, 2024
#Description  :	Milestone 4
#References   : https://learn.sparkfun.com/tutorials/raspberry-gpio/python-rpigpio-example


import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)
GPIO.setup(22, GPIO.IN)  #GPIO 22 for recieving synchronization signals
GPIO.setup(23, GPIO.OUT)  #GPIO 23 for sending synchronization signals
GPIO.setup(27, GPIO.OUT)  #GPIO 27 for sending character bits

#=================================================  
def encrypt_decrypt(char, mode):

    shift = 7
    if mode == 1:  #encrypt the character
        # ecrypt by shifting to the right in ASCII table
        encrypted_value = (ord(char) + shift) % 256  # wrap around the byte
        return chr(encrypted_value)
    elif mode == 0:  #decrypt the character
        #decrypt by shifting to the left in ASCII table
        decrypted_value = (ord(char) - shift) % 256  
        return chr(decrypted_value)
    else:
        raise ValueError("Mode must be 1 (encrypt) or 0 (decrypt)")



#=================================================            
def continuous_sync_check():
    while True:
        for _ in range(1):
            GPIO.output(23, GPIO.HIGH)  # Set GPIO 23 high
            time.sleep(0.025)
            state = GPIO.input(22)
            
            #print(f"GPIO 22 State (HIGH expected): {state}")  # Print state of GPIO 22
            if state != GPIO.HIGH:  # Verify GPIO 22 is also high
                #print("Synchronization failed: GPIO 22 is not HIGH")
                return False  # Synchronization failed if GPIO 22 doesn't match
            GPIO.output(23, GPIO.LOW)  # Set GPIO 23 low
            time.sleep(0.025)  # Short delay for stability
            state = GPIO.input(22)
            #print(f"GPIO 22 State (LOW expected): {state}")  # Print state of GPIO 22
            if state != GPIO.LOW:  # Verify GPIO 22 is also low
                #print("Synchronization failed: GPIO 22 is not LOW")
                return False  # Synchronization failed if GPIO 22 doesn't match
        return True  # If all iterations pass without synchronization failure

#=================================================       
def send_binary_message(message):
    message = encrypt_decrypt(message, 1)
    print("Attempting to synchronize...")
    successful_syncs = 0  # Counter for successful high/low cycles
    # Loop to check for 3 successful high and low cycles
    while successful_syncs < 3:
        if continuous_sync_check():
            successful_syncs += 1  # Increment counter on successful high/low cycle
            print(f"Synchronization cycle {successful_syncs} successful.")
        else:
            print("Sync failed")
                
    print("Synchronization confirmed with 3 high/low cycles. Starting message transmission.")
    for char in message:
        print(f"Sending binary value for character '{char}': ", end="")
        for j in range(8):
            #wait for synchronization before sending each bit
            while not continuous_sync_check():
                time.sleep(0.025)  # Wait a bit before checking again
                print("failed")
            # Once synchronized, proceed to send the bit
            value_to_send = (ord(char) >> (7 - j)) & 0x01
            GPIO.output(27, value_to_send)  # Send the actual message bit
            time.sleep(0.025)  # Short delay
            print(value_to_send, end="")
        print()  
    #Send termination signal
    print("\nSending termination signal...")
    termination_signal = '11111111'  # Ten '1's in a row
    for bit in termination_signal:
        while not continuous_sync_check():
                time.sleep(0.025)  #wait a bit before checking again
        #time.sleep(0.025)
        value_to_send = int(bit)  # Convert each '1' to integer 1
        GPIO.output(27, value_to_send)  # Send each bit of the termination signal
        time.sleep(0.05)  

    print("Termination signal sent. Message transmission completed.") 
    
    
    
#=================================================       
# Function to receive the binary message
def receive_binary_message(gpio_pin):
    message = ""
    received_word = ""
    # Set up GPIO pins for input
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(gpio_pin, GPIO.IN)
    sync_attempts = 0  #counter for synchronization attempts
    while sync_attempts < 3:
        if continuous_sync_check():
            print("Synchronization successful.")
            sync_attempts += 1
        else:
            print("Synchronization failed. Retrying...")
    print("Synchronization signal detected")
    print("Starting message reception...")
    
   #termination signal
    termination_signal = "11111111"
    current_bit_sequence = "" 
    while current_bit_sequence != termination_signal:
        while not continuous_sync_check():
            time.sleep(0.025)      
        time.sleep(0.025)
        value_received = GPIO.input(gpio_pin)
        
        print(value_received)
        
        current_bit_sequence += str(value_received)
        
        if len(current_bit_sequence) == 8:  # After reading 8 bits
            if current_bit_sequence == termination_signal:
                print("Termination signal received.")
                break
            char_value = chr(int(current_bit_sequence, 2))
            received_word += char_value
            print(f"Received character: {char_value}")
            current_bit_sequence = ""  # Reset for the next byte
    else:
        print("Synchronization lost. Attempting to re-synchronize...")

    print(f"Received word encrypted: {received_word}")
    decrypt = encrypt_decrypt(received_word, 0)
    print(f"Received word decrypted: {decrypt}")
    



#===========================================
def main_menu():
    try:
        GPIO.setmode(GPIO.BCM)
        while True:
            # Wait for user input to choose GPIO pin
            print("Enter 1 to receive or 2 to send, can use option 3 to check sync. Press Ctrl+C to exit.")
            choice = input()
            if choice == '1':
                receive_binary_message(17)
                break
            elif choice == '2':
                # Setup GPIO for sending message
                GPIO.setup(27, GPIO.OUT)
                message = "b"
                send_binary_message(message)
                break
            elif choice == '3':
                continuous_sync_check()
                break
            else:
                print("Invalid choice. Please enter 1 or 2.")

    except KeyboardInterrupt:
        pass  # Handle Ctrl+C gracefully

    finally:
        GPIO.cleanup()
        
if __name__ == '__main__':
    main_menu()
