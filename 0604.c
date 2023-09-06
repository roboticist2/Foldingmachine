//# -*- coding: utf-8 -*-
/*
Created on Mon May 27 16:21:03 2019

@author: choongho.lim
*/

#include <stdio.h>
#include <wiringPi.h>
#include <time.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <softPwm.h>

#define front 0
#define back 1
#define top 0
#define bottom 1
#define on 1
#define off 0

/*
pin number define
*/

#define loader_mot 8
//PWMO
#define loader_a 4
//DO
#define loader_b 5
#define loader_F_hall 6
//DI
#define loader_R_hall 26
//DI 


#define layer_1_mot 9
//PWMO
#define layer_1_a 15
//DO
#define layer_2_mot 16
//PWMO

#define plate_L_mot 3
//PWMO or DO
#define plate_L_hall 28
//DI
#define plate_R_mot 12
//PWMO or DO
#define plate_R_hall 29
//DI

#define rear_photo 31
//DI
#define front_photo 11
//DI

#define start_sw 25
//DI
#define reset_sw 27
//DI
#define stop_sw 22

#define ready 0
#define start 1
#define middle1 2
#define finish 3
#define middle2 4
#define middle3 5
#define pause 6



//int roller_pos;
int loader_pos;
int loading_state=off;
int plate_L_cmd=off;
int plate_R_cmd=off;
int plate_cnt;
int stop=off;
int ready_state;
clock_t starttime;
clock_t endtime;
int length;
int length_state=ready;

int plate_spd=100;
int layer_1_spd=30;
int layer_2_spd=60;
int loder_spd=50;

void reset(void);
void start_switch(void);
void stop_switch(void);

void start_switch(void)
{
    if( (ready_state==on) && (loading_state==ready) ) 
    {
        loading_state=loading_start;
        printf("loading start!\n");
    }
}

void stop_switch(void)
{

    stop=on;
    printf("Stop operation\n");
    delay(1);
    stop=off;
    ready_state=off;
    
    loading_state=ready;
    layer_1_state=ready;
    length_state=ready;
}

void *loading_thread(void *arg) 
{
	while (1)
	{
        switch (loading_state)
        {
            case ready :
                softPwmWrite(loader_mot,0);
                break;
                
            case loading_start :
                softPwmWrite(loader_mot,100); // 속도확인 필요
            	digitalWrite(loader_dir,TRUE); // 방향확인 필요
            	loading_state=loading_middle;
            	break;
            	
            case loading_middle :
                if( digitalRead(loader_R_hall)==0 ) // loader hall sensor 후방 자석 감지 시
            	{
                	softPwmWrite(loader_mot,0);
                	loader_pos=back;
                	//loader 정지
                	loading_state=loading_finish;
                }
            	break;            	
            case loading_finish :
                if( digitalRead(front_photo) ==1) // f.포토센서에 의류 감지 시
            	{
                	// 의류가 1차layer를 충분히 지나갔다고 판단, loader 복귀시킴
                	softPwmWrite(loader_mot,100);
                	digitalWrite(loader_dir,FALSE); // 방향확인 필요
                	//loader 전진
                	loading_state=loading_back;
            	}
            	break;            	
            case loading_back :
            	if( digitalRead(loader_F_hall)==0 ) // loader hall sensor 전방 자석 감지 시
            	{
                	softPwmWrite(loader_mot,0);
                	loader_pos=front;
                	//loader 정지
                	loading_state=ready;
                	
            	}
            	break;
        }
	}
}

void *layer_1_thread(void *arg) 
{
	while (1)
	{
    	switch layer_1_state
    	{
        	case ready :
            	if( loader_pos == back ) // loader position이 back 일때
            	{
                	layer_1_state=start;
                }
            	break;
            
            case start :
                softPwmWrite(layer_1_mot,layer_1_spd); // 속도확인 필요
            	digitalWrite(layer_1_dir,1); // 전진방향확인 필요
            	delay(0.5);
            	//layer1 0.5초동안 전진, 옷감 재정렬을 위함
        	
            	softPwmWrite(layer_1_mot,layer_1_spd); // 속도확인 필요
            	digitalWrite(layer_1_dir,0); // 후진방향확인 필요
            	//layer1 후진
            	layer_1_state=middle;  
            	break;   	
            
            case middle :
                if( digitalRead(front_photo)==1 ) // f.포토센서에 옷 감지시
            	{
                	softPwmWrite(layer_1_mot,0);
                	//layer1 정지
                	layer_1_state=ready;
                	break;
            	}
    	}
	}
}

void *layer_2_thread(void *arg) 
{
    while(1)
    {
        switch( layer_2_state )
        {
            case ready :
                if( digitalRead(rear_photo) == 1 ) // r.포토센서에 옷 감지
            	{
                	softPwmWrite(layer_2_mot,layer_2_spd); // 속도확인 필요
                    layer_2_state=middle1;
                }
                break;
            case middle1 :
                if( digitalRead(front_photo) == 1 ) // f.포토센서에 옷 감지
                {
                	softPwmWrite(layer_2_mot,0); // 이송 정지
                	plate_L_cmd = on;
                	plate_R_cmd = on; // plate에 작동 명령 전달
                	layer_2_state=pause;
                	plate_cnt=1;
                }
                break;
            case pause :
                if( (plate_L_cmd == off) && (plate_R_cmd == off) ) // plate가 모두 작동을 멈추고 나면
            	{
                	softPwmWrite(layer_2_mot,layer_2_spd); // 이송 재시작
                    if(plate_cnt==1)
                    {
                        length_state=start; // 길이 측정 시작
                        layer_2_state=middle2;
                    }
                    else if(plate_cnt==2)
                    {
                        layer_2_state=middle3;
                    }
                }
                break;
            case middle2 :
                if( digitalRead(rear_photo) == 0 ) // r.포토센서에 옷 감지->미감
                {
                	softPwmWrite(layer_2_mot,0); // 이송 정지
                	plate_L_cmd = on;
                	plate_R_cmd = on; // plate에 작동 명령 전달
                	layer_2_state=pause;
                	plate_cnt=2;
                }   
                break;  
            case middle3 :
               	if( digitalRead(front_photo) == 0 ) // f.포토센서에 옷 미감지
            	{
                	softPwmWrite(layer_2_mot,0); // 이송 정지
                }
                break;
        }
    }
}

void *plate_L_thread(void *arg)
{
    while(1)
    {
        switch(plate_L_cmd)
        {
            case on:
                softPwmWrite(plate_L_mot,100); // 속도확인 필요
                plate_L_cmd=middle1;
                for(int i=0; i<6; i++)
                {
                    delay(0.05);
                    if(stop==on)
                    {
                        softPwmWrite(plate_L_mot,0);
                        plate_L_cmd=off;
                        break;
                    }
                }
                break;
            case off:
                break;
            case middle1;
                if( digitalRead(plate_L_hall)==0 ) // hall sensor 측정위치 복귀 시
               	{
                   	softPwmWrite(plate_L_mot,0);
                   	plate_L_cmd = off;                  	
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
            case on:
                softPwmWrite(plate_R_mot,plate_spd); // 속도확인 필요
                plate_R_cmd=middle1;
                for(int i=0; i<8; i++)
                {
                    delay(0.05);
                    if(stop==on)
                    {
                        softPwmWrite(plate_R_mot,0);
                        plate_R_cmd=off;
                        break;
                    }
                }
                break;
            case off:
                break;
            case middle1;
                if( digitalRead(plate_R_hall)==0 ) // hall sensor 측정위치 복귀 시
               	{
                   	softPwmWrite(plate_R_mot,0);
                   	plate_R_cmd = off;                  	
               	}
               	break;
        }
    }
}


void *measure_length(void *arg)
{
    int timegap;
    while(1)
    {
        switch (length_state)
        {
            case start :
                starttime=clock();
                length_state=middle1;
                break;
                
            case middle1 :
                if( digitalRead(rear_photo) == 0) // 옷 끝부분이 r.포토센서를 지나는 순간
                {
                    endtime=clock();
                    length_state=finish;
                }
                break;
                
            case finish :
            
                timegap=float(endtime-starttime)/(CLOCKS_PER_SEC);
                length=1000*timegap*v+400; //v(mm/s)정보 필요
                printf("length : %d\n",length);
                length_state=ready; // 길이 측정 완료
                         
            case ready :
                break;

        }
    }
}

void reset(void)
{
    while(1)
    {
        switch(reset_state)
        {
            case ready:
                break;
            
            case start:
                reset_state=middle1;
                break;
            case middle1:
                reset_state=middle2;
                break;
            case middle2:
                reset_state=middle3;
                break;
            case middle3:
                reset_state=ready;
                break;
        }
    }
    
    
    if(digitalRead(plate_L_hall)==1) // plate_L이 정위치가 아닐때
    {
        softPwmWrite(plate_L_mot,plate_spd); // 속도확인 필요
       
        while(1)
        {
           	if( digitalRead(plate_L_hall)==0 ) // hall sensor 측정위치 복귀 시
           	{
               	softPwmWrite(plate_L_mot,0);
               	brake;
           	}
        }
    }
    
    if(digitalRead(plate_R_hall)==1) // plate_R이 정위치가 아닐때
    {
        softPwmWrite(plate_R_mot,plate_spd); // 속도확인 필요
       
        while(1)
        {
           	if( digitalRead(plate_R_hall)==0 ) // hall sensor 측정위치 복귀 시
           	{
               	softPwmWrite(plate_R_mot,0);
               	brake;
           	}
        }
    }
    
    if( digitalRead(loader_F_hall) != 0) // loader가 front가 아닐때
    {
        //loader 전진
        softPwmWrite(loader_mot,100); // 속도확인 필요
        digitalWrite(loader_dir,1); // 전진 방향확인 필요
        
        while(1)
        {
           	if( digitalRead(loader_F_hall) == 0 ) // loader가 front 복귀 시
           	{
               	softPwmWrite(loader_mot,0);
               	brake;
           	}
        }
    }
    
    return;
}

int main() 
{
	wiringPiSetup();
	
	// 각 pin에 대해서 모두 활성화
	softPwmCreate(loader_mot,0,100); 
    softPwmCreate(layer_1_mot,0,100);
    softPwmCreate(layer_2_mot,0,100);
    softPwmCreate(plate_L_mot,0,100);
    softPwmCreate(plate_R_mot,0,100);
    
	pinMode(start_sw, INPUT);
	pinMode(reset_sw, INPUT);
	pinMode(stop_sw, INPUT);
	pinMode(loader_dir, OUTPUT);
	pinMode(layer_1_a, OUTPUT);
	pinMode(layer_1_b, OUTPUT);
	pinMode(layer_2_a, OUTPUT);
	pinMode(layer_2_b, OUTPUT);
	pinMode(loader_F_hall, INPUT);
	pinMode(loader_R_hall, INPUT);
	pinMode(front_photo, INPUT);
	pinMode(rear_photo, INPUT);
	pinMode(plate_L_hall, INPUT);
	pinMode(plate_R_hall, INPUT);
		        	
	wiringPiISR(start_sw, INT_EDGE_FALLING, start_switch); // 버튼, edge_falling방식, 실행할함수
	wiringPiISR(reset_sw, INT_EDGE_FALLING, reset_switch); // 버튼, edge_falling방식, 실행할함수
	wiringPiISR(stop_sw, INT_EDGE_FALLING, stop_switch);
	
	/*
	구동 전 정렬부
	*/
	
	ready_state=off;

    reset();
    
    ready_state=on;
    
    
    
//	pthread_t thread;
//	pthread_create(&thread, NULL, fnd_thread, NULL);
//	&쓰레드변수명,NULL,실행할쓰레드,NULL
//	pthread_detach(thread);
//	//------------------------

//	pthread_t rollerthread;
//	pthread_create(&rollerthread, NULL, roller_thread, NULL); 
//	pthread_detach(rollerthread);
	
	pthread_t loadingthread;
	pthread_create(&loadingthread, NULL, loading_thread, NULL);
	pthread_detach(loadingthread);
	
	pthread_t layer1thread;
	pthread_create(&layer1thread, NULL, layer_1_thread, NULL);
	pthread_detach(layer1thread);
	
	pthread_t layer2thread;
	pthread_create(&layer2thread, NULL, layer_2_thread, NULL);
	pthread_detach(layer2thread);
	
	pthread_t plateLthread;
	pthread_create(&plateLthread, NULL, plate_L_thread, NULL);
	pthread_detach(plateLthread);

	pthread_t plateRthread;
	pthread_create(&plateRthread, NULL, plate_R_thread, NULL);
	pthread_detach(plateRthread);

	pthread_t _measurelength;
	pthread_create(&measurelength, NULL, measure_length, NULL);
	pthread_detach(measurelength);

	return 0;
}
