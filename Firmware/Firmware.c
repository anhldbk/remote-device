/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  HugeBrain Assistant's Firmware  v1.0
//                                        Programmed by: Le Duc Anh, Ho Hai Dang, Lai Minh Huy, Vu Hanh Hop
//                                        Team         : HugeBrain (Tin 2 K50)
//                                        Homepage     : HugeBrain.groups.google.com
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operation Description:
//                      1. WAITING MODE
//                         Our device's connected to a PC through a USB interface. When it's enumerated, the
//                         HugeBrain Assistant App (running on the PC) will send some commands to our device.
//                         If everything's OK, then turns on the Red LED, switch to the RECEIVING MODE.
//
//                       2.RECEIVING MODE
//                         From now on, every 20us our device will get the signal at the PIN_A0
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
         Counter++;
         if(Counter>= START_THRESHOLD) // Start THRESHOLD detected
         {
            Mode=MODE_RECEIVING; // so the next state will be the receiving mode
            output_bit(LED_SIGNAL_DETECTED,1);// inform that we've detected a new signal
            WaitToSync=1;
         
            // (re)initialize variables
            OutIndex=2;

         }
      }
      else
         Counter=0;
   }
   else{ // so we are in the receiving mode. 
   
      if(Received==LastReceived){
         if(!WaitToSync){
               Counter++;

            if(Counter>=STOP_THRESHOLD){// Stop THRESHOLD detected
               Mode=MODE_WAITING;
               Counter=0;// reset Counter
               
               OutData[0]=OutIndex;//size of array
               OutData[1]=0;
               IsSendingDataToHost=1;// set the status flag to inform that we've got st to send

               return;
            }
         }
      }
      else{
         
         if(!WaitToSync){
               if(LastReceived==0){
                  //Start a new bit
                  //Delta=(Counter0<<4) ^ Counter1;
                  if(OutIndex+1 < OUT_SIZE){
                     Counter>>1;// divide Counter by 2
                     OutData[OutIndex++]=Counter;
                  }
                  Counter=1;
               }
               else
                  Counter++;
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

               usb_put_packet(END_POINT_IN, OutData, 64, USB_DTS_TOGGLE);
               delay_ms(10);
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
