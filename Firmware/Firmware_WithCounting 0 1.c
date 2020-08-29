/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  HugeBrain Assistant's Firmware  v1.0
//                                        Programmed by: Le Duc Anh, Ho Hai Dang, Lai Minh Huy, Vu Hanh Hop
//                                        Team         : HugeBrain (Tin 2 K50)
//                                        Homepage     : HugeBrain.groups.google.com
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operation Description:
//                      1. INITIALIZING MODE
//                         Our device's connected to a PC through a USB interface. When it's enumerated, the
//                         HugeBrain Assistant App (running on the PC) will send a configuration to our device.
//                         If everything's OK, then turns on the Red LED, switch to the OPERATING MODE.
//
//                         What's a configuration? Technically, a configuration contains a series of Timing point at 
//                         which we read the output signal of the Ir Decoder. Cos every Remote controls' manufacturer 
//                         has their own way of encoding Infrared signals, so we need to have appropriate configuration
//                         for each type. This makes our device is more capable in reality. The reasons are simple:
//                                  + We just want a single Firmware.
//                                  + Every time we work with a new remote control, we can create a new configuration (by
//                                     the support of HugeBrain Assistant App) to adapt to this modification
//                                     (without re-writing our firmware) ^_^
//
//                       2.OPERATING MODE
//                         From now on, every 100us our device will get the signal at the PIN_A0
//                                  + Check to see if it's the Start signal
//                                  + If true, afterward our device will get the total signal to send to HugeBrain Assistant App.
//                                     Then the App will take an appropriate action
//                         
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define __USB_PIC_PERIF__ 1
#include "Firmware.h"
#int_TIMER2
void  TIMER2_ISR(void) // this service routine will be called every 100us
{
   if(Serving)
      return;
   Serving=1;
   
   Received= input(IR_CONNECTED_PIN);// Get the current status of the pin connected with the IrDecoder

   if(Mode== MODE_WAITING){
      
      if(Received== 0){
         Counter0++;
         if(Counter0>= START_TOKEN) // Start token detected
         {
            Mode=MODE_RECEIVING; // so the next state will be the receiving mode
            output_bit(LED_SIGNAL_DETECTED,1);// inform that we've detected a new signal
            WaitToSync=1;
            
            // (re)initialize variables
            Counter0=0;
            Counter1=0;
            HashValue=0;
            OutIndex=2;

         }
      }
      else
         Counter0=0;
   }
   else{ // so we are in the receiving mode. 
   

      if(Received==LastReceived){
         if(!WaitToSync){
            if(Received==1)
               Counter1++;
            else
               Counter0++;

            if(Counter1>=STOP_TOKEN){// Stop token detected
               Mode=MODE_WAITING;
               Counter0=0;
               
               OutData[0]=OutIndex;//size of array
               OutData[1]=0;
               IsSendingDataToHost=1;
 
               // disable the timer 2 interrupt
               //disable_interrupts(INT_TIMER2);

               return;
            }
         }
      }
      else{
         
         if(!WaitToSync){
               if(LastReceived==0){
                  //Start a new bit
                  //Delta=(Counter0<<4) ^ Counter1;
                  if(OutIndex+2 < OUT_SIZE){
                     //Counter0+=Counter1;
                     //Counter0>>1;
                     OutData[OutIndex++]=Counter0;
                     OutData[OutIndex++]=Counter1;
                  }
                  Counter0=0;
                  Counter1=1;
               }
               else
                  Counter0=1;
         }
         
         WaitToSync=0;

      }
   }
   LastReceived=Received;

   Serving=0;
}

void Init(){
   // CCS generated code
   setup_adc_ports(NO_ANALOGS|VSS_VDD);
   setup_adc(ADC_OFF);
   setup_psp(PSP_DISABLED);
   setup_spi(SPI_SS_DISABLED);
   setup_wdt(WDT_OFF);
   setup_timer_0(RTCC_INTERNAL);
   setup_timer_1(T1_DISABLED);
   // setup timer 2
   setup_timer_2(TimerPrescale,TimerPeriod,TimerPostscale);
   setup_timer_3(T3_DISABLED);
   setup_comparator(NC_NC_NC_NC);
   setup_vref(FALSE);
   enable_interrupts(GLOBAL);

   // Our initialization   
   IsSendingDataToHost=0;// nothing to send :)
   Mode=MODE_WAITING;
   Received=0;
   LastReceived=0;
   //Counter=0;

   Serving=0;
   WaitToSync=0;
   // Initialize USB interface
   usb_init();
   delay_ms(100);
   
   // inform that our device is in active state
   output_bit(LED_DEVICE_ACTIVE,1);
}


void main()
{
   Init();

   while(1){ // an endless ...loop excepting the case of power loss
         usb_task();
         if (usb_enumerated()) {// if our device's enumerated, so we can communicate with the OS
           
           if(IsSendingDataToHost!=0){ // Need to send data to host
               // disable the timer 2 interrupt
               disable_interrupts(INT_TIMER2);
               
               while(OutIndex<OUT_SIZE)
                  OutData[OutIndex++]=0;

               usb_put_packet(END_POINT_IN, OutData, 8, USB_DTS_TOGGLE);
               delay_ms(10);
               usb_put_packet(END_POINT_IN, &OutData[8], 8, USB_DTS_TOGGLE);
               delay_ms(10);
               usb_put_packet(END_POINT_IN, &OutData[16], 8, USB_DTS_TOGGLE);
               delay_ms(10);
               usb_put_packet(END_POINT_IN, &OutData[24], 8, USB_DTS_TOGGLE);
               delay_ms(10);
               usb_put_packet(END_POINT_IN, &OutData[32], 8, USB_DTS_TOGGLE);
               delay_ms(10);
               usb_put_packet(END_POINT_IN, &OutData[40], 8, USB_DTS_TOGGLE);
               delay_ms(10);
               usb_put_packet(END_POINT_IN, &OutData[48], 8, USB_DTS_TOGGLE);
               delay_ms(10);
               usb_put_packet(END_POINT_IN, &OutData[56], 8, USB_DTS_TOGGLE);
               // Turn off the led
               output_bit(LED_SIGNAL_DETECTED,0);
               IsSendingDataToHost=0; // nothing to send
               
               Delay_ms(100);
               
               Serving=0;
                  
               //Reset the timer 2 counter
               set_timer2(0);

               //Enable the timer 2 interrupt
               enable_interrupts(INT_TIMER2);                  

            }

            if (usb_kbhit(END_POINT_OUT)) {// check to see if we receive any data
               usb_get_packet(END_POINT_OUT,InData,8);
               switch(InData[0]){
               
                  case COMMAND_APP_READY:
                        // HugeBrain App initialized successfully
                        // The we turn on the Active LED, IR Ready LED
                        output_bit(LED_DEVICE_READY,1);
                        
                        Serving=0;
                        
                        //Reset the timer 2 counter
                        set_timer2(0);
                        //Enable the timer 2 interrupt
                        enable_interrupts(INT_TIMER2);                  
                        
                        break;
                  
//                  case COMMAND_SET_TIMER_INTERVAL:
//                        break;
                  
                  case COMMAND_APP_SHUTDOWN:
                        // HugeBrain App's terminated
                        // The we turn off the Ready Led
                        output_bit(LED_DEVICE_READY,0);
                        
                        // disable the timer 2 interrupt
                        disable_interrupts(INT_TIMER2);
                        break;
               }
            }
            
 
         }
   }
}
