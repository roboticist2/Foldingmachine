#include <stdio.h>
#include <wiringPi.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <softPwm.h>

/*
pin number define
*/

#define loader_mot 8 //PWMO
#define loader_a 4 //DO
#define loader_b 5 //DO
#define loader_F_hall 6 //DI
#define loader_R_hall 26 //DI 

#define layer_1_mot 9 //PWMO
#define layer_2_mot 7 //PWMO

#define plate_L_mot 3 //PWMO
#define plate_L_hall 28 //DI
#define plate_R_mot 12 //PWMO or DO
#define plate_R_hall 29 //DI

#define rear_photo 24 //DI
#define front_photo 14 //DI

#define start_sw 25 //DI
#define reset_sw 27 //DI
#define stop_sw 13 //DI

#define front 0 
#define back 1
#define off 0
#define on 1

#define ready 0
#define start 1
#define middle1 2
#define finish 3
#define middle2 4
#define middle3 5
#define middle4 6
#define middle5 7

#define pause 8
#define comeback 9

int loader_pos;
int loading_state=off;
int plate_L_cmd=off;
int plate_R_cmd=off;
int plate_cnt;
int stop=off;
int ready_state=off;
clock_t starttime;
clock_t endtime;
int timegap;
int length;
int length_state=ready;
int layer_1_state=ready;
int layer_2_state=ready;
int reset_state=ready;

int plate_spd=100;
int layer_1_spd=50;
int layer_2_spd=80;
int loader_spd=40;

void reset_switch(void);
void start_switch(void);
void stop_switch(void);

void start_switch(void)
{
     plate_L_cmd = on;
}

void *plate_L_thread(void *arg)
{
    while(1)
    {
	
        switch(plate_L_cmd)
        {
            case on: // 1
		starttime=micros();
                softPwmWrite(plate_L_mot,100);
		delay(300);
		softPwmWrite(palte_L_mot,30);
		plate_L_cmd=middle1;
		break;
            
	    case off: // 0
		softPwmWrite(plate_L_mot,0);
                break;
            
	    case middle1: //2
	        if( (digitalRead(plate_L_hall)==0) )
               	{
			endtime=micros();
			timegap=endtime-starttime;
                   	softPwmWrite(plate_L_mot,0);
			printf("time : %d \n", timegap);
			plate_L_cmd=off;
		}
		break;
        }
    }
}

void *plate_R_thread(void *arg)
{
    while(1)
    {
	
        switch(plate_R_cmd)
        {
            case on: // 1
                softPwmWrite(plate_R_mot,plate_spd); // 속도확인 필요
		plate_R_cmd=middle1;
		printf("plate_R_Hall:%d, plate move!\n",digitalRead(plate_R_hall));
                delay(200);
                break;
            case off: // 0
		printf("plate_R_Hall:%d, waiting...\n",digitalRead(plate_R_hall));
		delay(100);
                softPwmWrite(plate_R_mot,0);
                break;
            case middle1: //2
	        //printf("plate_L_Hall:%d, plate on moving sequence..\n",digitalRead(plate_L_hall));
		
		delay(1);
		if( (digitalRead(plate_R_hall)==0) || (stop==on) ) // hall sensor 측정위치 복귀 시
               	{
                   	softPwmWrite(plate_R_mot,0);
			printf("plate_R_Hall:%d, plate return!\n",digitalRead(plate_R_hall));
		   	plate_R_cmd = off;                  	
               	}
               	break;
        }
    }
}

int main()
{

	wiringPiSetup();
	
	pinMode(start_sw, INPUT);
	pinMode(reset_sw, INPUT);
	pinMode(stop_sw, INPUT);
	pinMode(loader_mot, OUTPUT);
	pinMode(plate_L_mot, OUTPUT);
	pinMode(plate_R_mot, OUTPUT);
	pinMode(layer_1_mot, OUTPUT);
	pinMode(layer_2_mot, OUTPUT);
	pinMode(loader_a, OUTPUT);
	pinMode(loader_b, OUTPUT);

	pinMode(loader_F_hall, INPUT);
	pinMode(loader_R_hall, INPUT);
	pinMode(front_photo, INPUT);
	pinMode(rear_photo, INPUT);
	pinMode(plate_L_hall, INPUT);
	pinMode(plate_R_hall, INPUT);

	softPwmCreate(loader_mot,0,40); 
	softPwmCreate(layer_1_mot,0,40);
	softPwmCreate(layer_2_mot,0,40);
	softPwmCreate(plate_L_mot,0,40);
	softPwmCreate(plate_R_mot,0,40); 
	       
	wiringPiISR(start_sw, INT_EDGE_FALLING, start_switch);
	wiringPiISR(stop_sw, INT_EDGE_FALLING, stop_switch);
	  
	pthread_t plateLthread;
	pthread_create(&plateLthread, NULL, plate_L_thread, NULL);
	pthread_detach(plateLthread);

	pthread_t plateRthread;
	pthread_create(&plateRthread, NULL, plate_R_thread, NULL);
	pthread_detach(plateRthread);
	 
	while(1)
	{
		
	}

    
	return 0;
}
