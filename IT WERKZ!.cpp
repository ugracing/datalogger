#include "mbed.h"
//#include "MSCFileSystem.h"
#define FSNAME "msc"

#include "cmath"
#include <stdio.h>
#include <string>

//MSCFileSystem msc("msc");                                       //USB file
    

Serial pc(USBTX, USBRX); // tx, rx
Serial msquirt(p9, p10); // tx, rx
DigitalOut myled(LED1);
AnalogOut LED_driver(p18);
DigitalOut LED_CLT(p21);

Timer t;
int byte_count;
string str;
char logged_str[300];
bool fetch;
int count;

struct car_datastruct{
    float RPM;
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
     
                      
void rx_data(){
    //__disable_irq();    // Disable Interrupts
    count++;
    
    str = "";
    byte_count=0;
    pc.printf("Reading data\n\r");
    while (1){

        if (msquirt.readable())
        {
            //pc.printf("got byte - %d\r\n", byte_count);
            str += msquirt.getc();
            byte_count++;
        }
        
        if (byte_count >= 209)
        {
            //pc.printf("we got data\n\r");
            break;
        }
    }

    union
    {
        uint8_t b[2];
        int i;
    };
    b[1] = str[22]; b[0] = str[23];
    car_data.coolant = (i-320) * 0.05555;
    pc.printf("CLT = %f \r\n",car_data.coolant);
    
    b[1] = str[6]; b[0] = str[7];
    car_data.RPM = i;
    pc.printf("rpm %f \n\r",car_data.RPM);
    
    b[1] = str[28];b[0] = str[29];
    car_data.air_fuel_1 = i*0.1;
    pc.printf("Air_fuel_1 %f \n\r",car_data.air_fuel_1);
    
    b[1] = str[18];b[0] = str[19];
    car_data.map = i*0.1;
    pc.printf("MAP %f \n\r",car_data.map);
    
    b[1] = str[20];b[0] = str[21];
    car_data.mat = (i-320)*0.05555;
    pc.printf("MAT %f \r\n", car_data.mat);

    
    b[1] = str[24];b[0] = str[25];
    car_data.throttle = i/10;
    pc.printf("Throttle %f \r\n", car_data.throttle);

    b[1] = str[8];b[0] = str[9];
    car_data.advance = i*0.1;
    pc.printf("Advance %f \r\n", car_data.advance);
    
    b[1] = str[26];b[0] = str[27];
    car_data.battery = i*0.1;
    pc.printf("Battery %f \r\n", car_data.battery);

    /*b[0] = str[108];
    car_data.throttle = i*0.1;
    pc.printf("Throttle %f \r\n", car_data.throttle);*/
    
    b[1] = str[28];b[0] = str[29];
    car_data.air_fuel_1 = i*0.1;
    b[1] = str[28];b[0] = str[29];
    car_data.air_fuel_2 = i*0.1;
    int offset;
    
    pc.printf("injectors ");
    b[0] = str[10];
    for(offset = 0; offset<7; offset++){
        car_data.injectors_status[offset] = ((b[0] >> offset) & 0x01);
        pc.printf("%d", car_data.injectors_status[offset]);
    }
    
    pc.printf("engine ");
     b[0] = str[11];
    for(offset = 0; offset<7; offset++){
        car_data.engine_status[offset] = ((b[0] >> offset) & 0x01);
        pc.printf("%d", car_data.engine_status[offset]);
    }
    
    
    pc.printf("got %d bytes\r\n \r\n", byte_count);

    sprintf(logged_str, "%.3f  %.3f  %.3f  %.3f  %.3f  %.3f  %.3f \r\n", t.read(), car_data.RPM, car_data.mat, car_data.map, car_data.coolant, car_data.throttle, car_data.battery );

    //fprintf(fp, logged_str);
    str = "";
    byte_count = 0;
    //pc.printf("set to zer mofofoker");
    fetch = true;
    // __enable_irq();     // Enable Interrupts
    //pc.printf("fetch set true");
    //wait(0.2);
}

void flush()
{
    char b;
    while(msquirt.readable())
    {
        b = msquirt.getc();
    }
}

void log_data()
{
    //fprintf(fp,"%.1f\370C\t%.1f\370C\n\r",temp(Tin), temp(Tout));
}


int main() {     
             
                      
    //FILE *fp = fopen( "/msc/usb4.txt", "w");                    //open USB file
    
    pc.printf("\r\nData Logging Started\r\n\r\n");              //print instructions to terminal     
    //fprintf(fp,"TIME     RPM  MAT  MAP  CLT  THROTTLE  BATTERY \r\n\r\n");   //log rpm and time to USB

    //fclose (fp); 
    
    //t.start();       
    msquirt.baud(38400);
    pc.baud(57600);
    pc.printf("main\r\n");
    //msquirt.attach(&rx_data, Serial::RxIrq);
    fetch = true;
    flush();
    while(1){

        //wait_us(100);
        if(fetch)
        {   
            //FILE *fp = fopen( "/msc/usb4.txt", "a");                    //open USB file
            
            flush();
            msquirt.putc('A');
            wait(0.1);
            rx_data();
            //fclose(fp);
                                                     //close USB file

            
        }
    }        
            
            

}
