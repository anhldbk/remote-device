#include <18F4550.h>


#FUSES HSPLL
#FUSES NOWDT                    //No Watch Dog Timer

//#FUSES ECPIO                    //External Clock with PLL enabled, I/O on RA6
#FUSES NOPROTECT                //Code not protected from reading
#FUSES BROWNOUT                 //Reset when brownout detected
#FUSES BORV20                   //Brownout reset at 2.0V
#FUSES NOPUT                    //No Power Up Timer
#FUSES NOCPD                    //No EE protection
#FUSES STVREN                   //Stack full/underflow will cause reset
#FUSES NODEBUG                  //No Debug mode for ICD
#FUSES NOLVP                      //No Low Voltage Programming on B3(PIC16) or B5(PIC18)
#FUSES PLL5                     //Divide By 5(20MHz oscillator input)
#FUSES CPUDIV1                  //System Clock by 1
#FUSES USBDIV                   //USB clock source comes from PLL divide by 2
#FUSES VREGEN                   //USB voltage regulator enabled


#use delay(clock=48000000)      // operates at the frequency of 48Mhz
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Define some stuff for using USB interface

#build(reset=0x1, interrupt=0x8)          // Necessary for Bootloader
#ORG 0x0F00,0x0FFF {}                     // Necessary for Bootloader


// CCS Library dynamic defines

#define USB_HID_DEVICE  TRUE //Tells the CCS PIC USB firmware to include HID handling code.

#define USB_EP1_TX_ENABLE  USB_ENABLE_INTERRUPT   //turn on EP1 for IN bulk/interrupt transfers
#define USB_EP1_TX_SIZE    64  //allocate 64 bytes in the hardware for transmission
#define USB_EP2_RX_ENABLE  USB_ENABLE_INTERRUPT   //turn on EP2 for OUT bulk/interrupt transfers
#define USB_EP2_RX_SIZE    64  //allocate 64 bytes in the hardware for reception

#define USB_USE_FULL_SPEED   TRUE

// CCS USB Libraries
#include <pic18_usb.h>   //Microchip 18Fxx5x hardware layer for usb.c
#include "usb_desc_hid 8-byte.h" // This header contains an HID configuration for our device
#include "usb.c" // a sample program provided by CCS compiler

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           OUR OWN CONSTANT DEFINITIONS

#define IR_CONNECTED_PIN PIN_A0
#define LED_DEVICE_ACTIVE PIN_D0 // Our device's connected to a computer
#define LED_DEVICE_READY PIN_D1 // Our program's running on the computer
#define LED_SIGNAL_DETECTED PIN_D2 // Ir Signal detected

// define a basic command set to interchange information between the device and HugeBrain Assistant App
#define COMMAND_APP_READY 0   // HugeBrain Assistant App's Initialization's successfull
#define COMMAND_SET_TIMER_INTERVAL 1  // The App sets up a value for the timer 2's interval
#define COMMAND_APP_SHUTDOWN 2 // The Apps's shutdown

// define end points for the USB interface
// IN or OUT is based on the host's perspective
#define END_POINT_IN 01 // related to the 153rd line in usb_desc_hid 8-byte.h
#define END_POINT_OUT 02 // related to the 165th line in usb_desc_hid 8-byte.h

// define some mode used by the Timer 2's Interrupt Service Routine (ISR)
#define MODE_WAITING 01 // Waiting for the start token
#define MODE_RECEIVING 02 // Receiving data

//Start token: a series of Zero-valued signals detected
#define DEFAULT_START_THRESHOLD 400

// Stop THRESHOLD: technically, a stop THRESHOLD represents the minimum time between 2 consecutive the IrDecoder's frames
#define DEFAULT_STOP_THRESHOLD 1250

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           VARIABLE DEFINITIONS

int8 InData[8];// Array to store received data get from the host
//int8 OutData[8];// Array to send hash values of IrDecoder code to the host
int8 OUT_SIZE=64;
int8 OutData[64];
int8 OutIndex=0;

int8 IsSendingDataToHost;// If this variable's not zero, so the ISR will ignore any signal received.

int16 STOP_THRESHOLD=DEFAULT_STOP_THRESHOLD;
int16 START_THRESHOLD=DEFAULT_START_THRESHOLD;

// define some variable to use exclusively by the Timer 2 's Interrup Serivce Routine (ISR) 
int8 Mode=MODE_WAITING ;// Represent 2 modes: MODE_WAITING or MODE_RECEIVING
char Received=0, LastReceived=0;
int16 HashValue;
int16 Counter ;
int8 Serving=0, WaitToSync=0;

// define variables for adjusting the timer 2's interval
// By default, the timer 2 is set up to trigger the ISR every QuantumTime(us)  
//int8 QuantumTime=20;
int8 TimerPrescale=T2_DIV_BY_1, TimerPeriod=240, TimerPostscale=1;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
