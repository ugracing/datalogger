#include "mbed.h"
#include "MSCFileSystem.h"
#define FSNAME "msc"
#define PC_DEBUG 0
#define MAX_RPM_DISPLAY 12000
#define COOLANT_WARNING_THRESHOLD 95

#include "cmath"
#include <stdio.h>
#include <string>

MSCFileSystem msc("msc");                                       //USB file
    

Serial pc(USBTX, USBRX); // tx, rx
Serial msquirt(p9, p10); // tx, rx
DigitalOut myled(LED1);
AnalogOut LED_driver(p18);
DigitalOut LED_CLT(p21);
DigitalOut debugLED (LED1);

Timer t;
Timer serialTimeout;
char serialBuffer[300];
int serialBufferEnd = 0;
char loggerBuffer[300]; //Massive ass logger buffer
bool fetch;
int count;
int flashCount;


struct car_datastruct{
    unsigned int seconds;
    unsigned int rpm;
    float map;
    float mat;
    float coolant;
    float throttle;
    float battery;
    float air_fuel_1;
    float air_fuel_2;
    float advance;
    bool engine_status[8];
    bool injectors_status[7];
};

car_datastruct car_data; 
                      
void megasquirtRequest(){
    
    //Send request
    msquirt.putc('A');
    
    count++;
    serialBufferEnd = 0;
    if(PC_DEBUG){ pc.printf("Reading data\n\r");}
    
    //Collect responce into buffer and resend if no responce after an amount of time
    serialTimeout.reset();
    while (1){
        if (msquirt.readable())
        {
            serialBuffer[serialBufferEnd] = msquirt.getc();
            serialBufferEnd++;
        }

        if (serialBufferEnd >= 209){ break; }
        
        if (serialTimeout.read_ms() > 100){
            msquirt.putc('A');
            serialTimeout.reset();
            if(PC_DEBUG){ pc.printf("Serial connection lost\n\r");}
        }
    }

    union { uint8_t b[2]; int16_t i; uint16_t ui; };
    
    
    b[1] = serialBuffer[0]; b[0] = serialBuffer[1];
    car_data.seconds = ui;
    if(PC_DEBUG){ pc.printf("Engine seconds = %u \n\r", car_data.seconds); }
    
    b[1] = serialBuffer[22]; b[0] = serialBuffer[23];
    car_data.coolant = (i-320) * 0.05555;
    if(car_data.coolant >= COOLANT_WARNING_THRESHOLD){ LED_CLT = 1;} else {LED_CLT = 0;}   //Coolant warning light code
    if(PC_DEBUG){ pc.printf("CLT = %f \r\n",car_data.coolant);}
    
    b[1] = serialBuffer[6]; b[0] = serialBuffer[7];
    car_data.rpm = ui;
    debugLED = 0;
    LED_driver.write_u16((65535 * car_data.rpm) / MAX_RPM_DISPLAY);
    debugLED = 1;
    if(PC_DEBUG){ pc.printf("rpm %u \n\r",car_data.rpm);}
    
    b[1] = serialBuffer[28];b[0] = serialBuffer[29];
    car_data.air_fuel_1 = i/10.0;
    if(PC_DEBUG){ pc.printf("Air_fuel_1 %f \n\r",car_data.air_fuel_1);}
    
    b[1] = serialBuffer[18];b[0] = serialBuffer[19];
    car_data.map = i*0.1;
    if(PC_DEBUG){ pc.printf("MAP %f \n\r",car_data.map);}
    
    b[1] = serialBuffer[20];b[0] = serialBuffer[21];
    car_data.mat = (i-320)*0.05555;
    if(PC_DEBUG){ pc.printf("MAT %f \r\n", car_data.mat);}

    
    b[1] = serialBuffer[24];b[0] = serialBuffer[25];
    car_data.throttle = i/10.0;
    if(PC_DEBUG){ pc.printf("Throttle %f \r\n", car_data.throttle);}

    b[1] = serialBuffer[8];b[0] = serialBuffer[9];
    car_data.advance = i*0.1;
    if(PC_DEBUG){ pc.printf("Advance %f \r\n", car_data.advance);}
    
    b[1] = serialBuffer[26];b[0] = serialBuffer[27];
    car_data.battery = i*0.1;
    if(PC_DEBUG){ pc.printf("Battery %f \r\n", car_data.battery);}

    /*b[0] = serialBuffer[108];
    car_data.throttle = i*0.1;
    pc.printf("Throttle %f \r\n", car_data.throttle);*/
    
    b[1] = serialBuffer[28];b[0] = serialBuffer[29];
    car_data.air_fuel_1 = i*0.1;
    b[1] = serialBuffer[28];b[0] = serialBuffer[29];
    car_data.air_fuel_2 = i*0.1;
    int offset;
    
    if(PC_DEBUG){ pc.printf("injectors ");}
    b[0] = serialBuffer[10];
    for(offset = 0; offset<7; offset++){
        car_data.injectors_status[offset] = ((b[0] >> offset) & 0x01);
        if(PC_DEBUG){ pc.printf("%d", car_data.injectors_status[offset]);}
    }
    
    if(PC_DEBUG){ pc.printf("engine ");}
     b[0] = serialBuffer[11];
    for(offset = 0; offset<7; offset++){
        car_data.engine_status[offset] = ((b[0] >> offset) & 0x01);
        
    }
    
    
    if(PC_DEBUG){ 
        pc.printf("got %d bytes\r\n \r\n", serialBufferEnd);
        pc.printf("TIMEEEE  %f \r\n", t.read());
    }
    
    sprintf(loggerBuffer, "%.2f\t%u\t%u\t%.3f\t%.1f\t%.1f\t%.1f\t%.1f \r\n\0", t.read(), car_data.seconds, car_data.rpm, car_data.mat, car_data.map, car_data.coolant, car_data.throttle, car_data.battery );

    fetch = true;
}

void flush()
//Removed the character to write to, idk if it will still work, it should, but this is the mbed
{
    while(msquirt.readable())
    {
        msquirt.getc();
    }
}


int main() {
    //Wait for megasquirt to initialise
    wait(1);
    //This baud rate does work and has been tested through dodgy dodgy extention cables, but may be flaky so change if neccessary
    msquirt.baud(115200);
    pc.baud(115200);
    LED_driver = 1.0;
    serialTimeout.start();
    
    //File initialisation
    
    FILE *fp = fopen( "/msc/usb5.tsv", "w");                    //open USB file
    if(fp != NULL){
        pc.printf("\r\nData Logging Started\r\n\r\n");              //print instructions to terminal     
        fprintf(fp,"TIME\tENGINE TIME\tRPM\tMAT\tMAP\tCLT\tTHROTTLE\tBATTERY\r\n\r\n");   //Print the field names
        fclose (fp);
    } else { pc.printf("Write to USB failed\n\r"); }
    
    t.start();       

    pc.printf("main\r\n");
    fetch = true;
    while(1){
        //if(LED_driver.read() >= 1.0){ LED_driver = 0.0; }
        //LED_driver = LED_driver + 0.1;


        if(fetch){   
            
            flush();
            megasquirtRequest();
            
            FILE *fp = fopen( "/msc/usb5.tsv", "a"); //open USB file
            //Write log to file
            if(fp != NULL){
                fprintf(fp, loggerBuffer);
                fclose(fp);                         //close USB file
            } else if(PC_DEBUG){ pc.printf("Write to USB failed\n\r"); }
        }
    }        
            
            

}