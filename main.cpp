#include <avr/io.h>
#include <avr/interrupt.h>
#include "data.pb.h"
#include <pb_encode.h>
#include <pb_decode.h>
#include <stdint.h>

#define BAUDRATE 9600 

//new struct that simply tracks if we have a message we are currently reading
//we are now assuming messages will come in the format [Byte repping length of bytes in message],data,data...
struct{

    size_t length; //represents how many bytes the current message will need from our buffer stream
    volatile uint8_t rxIndex = 0;//keeps track of how many bytes have been read so far
    uint8_t tempBuffer[100]; //temporary buffer that bytes will be read into until we reach the "length"
    bool hasMessage = false; //toggles to true everytime we get a message while we currently do not have one

} protocolHandler;

//flag that tells us if we have read bytes in from a message ready to process, MUST BE VOLATILE SINCE THE CHANGE DOESNT HAPPEN IN MAIN
//PROGRAM WON'T BOTHER WITH IT WHEN ITS CHANGED OUTSIDE OF MAIN IN ISR
volatile bool isReady = false;

//this function sets the baud rate by setting the bits for the upper and lower registers for it
//it also sets the bits in the recieve, transmit and recieveInterrupt registers so that it is enabled for UART
void initializeUart(void){


    //Set UART baud rate(UART BAUD RATE REGISTER)
    //made up of two 8-bit registers because we require 16-bits to achieve a wide range of baud rates
    //use bit time formula to derive bit time from baud rate and CPU freq, use normal speed since our baud rate is only 9600
    UBRR0H = 0; //first 8 bits are 0 so set higher to 0
    UBRR0L = (uint8_t) (16000000/ (16UL * BAUDRATE) - 1); //truncation to 8 bits since only the lower half matters

    //UCSR0 is responsible for configuring UART, B is used to toggle R/T and enbale interrupts
    //OR all of it to get the final 8-bit setting that toggles each
    //resulting 8bit string should be 10011000
    //indicates "for 0th UART port enable recieve, transmit and interrupt if recieve gets data"
    UCSR0B = (1 << RXEN0) |  //RXEN0=4, shift so that bit 4 is set
             (1 << TXEN0) |  //TXEN0= 3, shift so that bit 3 is set
             (1 << RXCIE0); //RXCIE0 = 7, shift so that bit 7 is set


    //set bits per frame
    UCSR0C = (1<<UCSZ01) | (1<<UCSZ00); //011 for 8-bit with 1 stop bit by default

    sei();

}

//read in the byte from UART, set flag if we have a full message 
ISR(USART0_RX_vect){


    //if we do not have a current message we are processing, this must be a new message, store the length and toggle flag
    if(!protocolHandler.hasMessage){
        protocolHandler.length = UDR0;
        protocolHandler.hasMessage = true;
    }
    else{//data byte, simply add it to the buffer and increment the counter

        protocolHandler.tempBuffer[protocolHandler.rxIndex] = UDR0;//store the byte into the buffer
        protocolHandler.rxIndex = protocolHandler.rxIndex + 1;//increment the index counter

        //if we have reached the total length of the message let main know that it can read bytes in and reset values
        if(protocolHandler.rxIndex == protocolHandler.length){
            isReady= true;
            protocolHandler.rxIndex = 0;
            protocolHandler.hasMessage = false;
        }

    }
}


int main(){

    initializeUart();
    DDRB |= (1 << PB7);
    uint8_t byteStream [100];
    uint8_t tempLength;

    while(1){

        //if the ready to process flag is toggled process message
        if(isReady){

            //temporarly disable interrupts so we don't have a possible write to the global buffer during copy
            //this takes tens of microseconds while 1 byte takes 1 millisecond to read so we should be able to copy before another byte is read
            cli();
            tempLength = protocolHandler.length;  
            memcpy(byteStream, protocolHandler.tempBuffer, tempLength); //copy the contents of buffer into our buffer we are going to use
            isReady = false;
            sei();                
            

            //Decode the protobuf message
            BlinkData msg = BlinkData_init_zero; //protobuff makes BlinkData into a class, basically create an object of it and initialize it to "zero"
            pb_istream_t stream = pb_istream_from_buffer(byteStream, tempLength); //creates a stream for nanopb that is able to go through our data in the buffer (lets it track which byte we're reading, how many left, etc), simply just lets nanopb read our data

            if (pb_decode(&stream, BlinkData_fields, &msg)) { //using our stream, read in bytes, BlinkData_fields tells us how ti interperet bytes and then it gets copied into msg so now we can represent the data in our protobuff messages format 
                if (msg.has_toggle && msg.toggle) {
                    PORTB |= (1 << PB7);   //toggle the LED on, done by setting the respective bit
                    //send message to frontend that LED status is ON, done by writing to UDR0 register
                    //won't get in the way or re-read back in since we go send bytes to MCU -> read in bytes -> toggle -> send status back
                    //and our interface expects this so it'll wait on it before sending bytes again
                    UDR0 = 0x01;

                } else {
                    PORTB &= ~(1 << PB7);  //toggle the LED off
                    UDR0 = 0x00; //same explanation as above
                }
            }
        }

    }



}