#ifndef _HARDWARE_H_
#define _HARDWARE_H_


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








#endif
