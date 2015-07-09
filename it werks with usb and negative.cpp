#include "mbed.h"
#include "MSCFileSystem.h"

#include <string>
#include "N5110.h"

#define ENGINE_TEMP_ID      10
#define WHEEL_SPEED_1_ID    700
#define WHEEL_SPEED_2_ID    710
#define WHEEL_SPEED_3_ID    730
#define WHEEL_SPEED_4_ID    740
#define FUEL_GAUGE_ID       1800

#define PC_DEBUG 1
#define FSNAME "msc"
MSCFileSystem msc(FSNAME);

Serial pc(USBTX, USBRX);
Ticker usbLog;
Timeout pollChange;
Timer timePassed;
DigitalOut led1(LED1);
DigitalOut led2(LED2);
CAN can1(p30, p29);

char counter = 0;
char temp;
float x;

struct canData{
    float engineTemp;
    uint8_t fuel;
    uint8_t wheelSpeed[4];
};

canData logBuffer;

void setTempPoll(){
    //Set the temp sensor to poll at a new frequency, def in us.
    union{
        char c[4];
        float f;
    };
    f = 1.0;
    char send[5];
    send[0] = 0x01; send[1] = c[0]; send[2] = c[1]; send[3] = c[2]; send[4] = c[3];
    if(can1.write(CANMessage(500, send, 5))){ pc.printf("polling rate changed"); }
}



void bufferLogDataFloat(CANMessage msg, float data){
    //these functions would be more populated if we didnt only have 6 sensors
    if(msg.id == ENGINE_TEMP_ID){ logBuffer.engineTemp = data; }
}

void bufferLogDataChar(CANMessage msg, char* data){
    if(msg.id == FUEL_GAUGE_ID){ logBuffer.fuel = data[0]; }
    if(msg.id == WHEEL_SPEED_1_ID){ logBuffer.wheelSpeed[0] = data[0]; }
    if(msg.id == WHEEL_SPEED_2_ID){ logBuffer.wheelSpeed[1] = data[0]; }
    if(msg.id == WHEEL_SPEED_3_ID){ logBuffer.wheelSpeed[2] = data[0]; }
    if(msg.id == WHEEL_SPEED_4_ID){ logBuffer.wheelSpeed[3] = data[0]; }
}

void bufferLogDataInt(CANMessage msg, int data){
    //I make all these fancy data types and everyone just uses chars...
}

void bufferLogDataLong(CANMessage msg, long data){
    //I am confident that no one will ever send a long.
}


void CANRecivedMessage(CANMessage msg){
        //This is an ungodly union to convert from the characters to whatever you need.
        union{
            char c[4];
            float f;
            int32_t i;
            uint32_t ui;
        };
        
        pc.printf("Message recieved of length %d :  ", msg.len);
        //Print all the characters from the message in order.
        for(int j = 0; j < msg.len; j++){
            pc.printf("%d, ", msg.data[j]);
        }
        
        //fill the union
        for(int j = 1; (j < 5) && (j < msg.len); j++){
            c[j - 1] = msg.data[j];
        }
        //Print the data type defined by the first character and convery the characters to that type and display. 
        //The bitwise AND seperates the character to give us only the first two digits.
        switch(msg.data[0] & 0x03){
            case 0:
                //message is a character
                pc.printf("character: %d ", c[0]);
                bufferLogDataChar(msg, c);
                break;
            case 1:
                //message is a float
                pc.printf("float: %f ", f);
                bufferLogDataFloat(msg, f);
                break;
            case 2:
                //message is an int
                pc.printf("integer: %d ", i);
                break;
            case 3:
                //message is a long
                pc.printf("long: %u ", ui);
                break;
        }
        pc.printf("from ID %d\n\r", msg.id);
        pc.printf("Reed errors:%d\n\r", can1.rderror());
}


void logToUSB(){
    FILE *fp = fopen( "/msc/can_log.csv", "a"); //open USB file
    //Write log to file
    if(fp != NULL){
        fprintf(fp, "%d,%f,%d,%d,%d,%d,%d,\n\r", timePassed.read(), logBuffer.engineTemp, 
                                                logBuffer.wheelSpeed[0], logBuffer.wheelSpeed[1], logBuffer.wheelSpeed[2], logBuffer.wheelSpeed[3], 
                                                logBuffer.fuel);
        fclose(fp);                         //close USB file
    } else if(PC_DEBUG){ pc.printf("Write to USB failed\n\r"); }
}

int main() {
    pc.baud(115200);
    led2 = 1;
    timePassed.start();
    CANMessage msg;
    
    
    //Set a timer to change the polling rate for the temp sensor after 10 seconds. See if it actually works and all that...
    pollChange.attach(setTempPoll, 10.0);
    
    FILE *fp = fopen( "/msc/can_log.csv", "w"); //open USB file
    //Write log to file
    if(fp != NULL){
        fprintf(fp, "Time,Engine Temperature,Wheel Speed 1,Wheel Speed 2,Wheel Speed 3,Wheel Speed 4,Fuel Gauge\n\r\n\r");
        fclose(fp);                         //close USB file
    } else if(PC_DEBUG){ pc.printf("Write to USB failed\n\r"); }

    wait(1);
    
    //usbLog.attach(logToUSB, 3);
    
    while(1) {
        led1 = !led1;
        if(can1.read(msg)) {
            CANRecivedMessage(msg);
            led2 = !led2;
            
                FILE *fp = fopen( "/msc/can_log.csv", "a"); //open USB file
    //Write log to file
    if(fp != NULL){
        fprintf(fp, "%f,%f,%d,%d,%d,%d,%d,\n\r", timePassed.read(), logBuffer.engineTemp, 
                                                logBuffer.wheelSpeed[0], logBuffer.wheelSpeed[1], logBuffer.wheelSpeed[2], logBuffer.wheelSpeed[3], 
                                                logBuffer.fuel);
        fclose(fp);                         //close USB file
    } else if(PC_DEBUG){ pc.printf("Write to USB failed\n\r"); }
 
        }
    }
}