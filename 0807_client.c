#include <stdio.h>
#include <wiringPi.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <softPwm.h>

/*
pin number define
*/

struct packet {
	char *base;
	char *ptr;
};

void init_packet(struct packet *packet, char *buf) {
	packet->base = buf;
	packet->ptr = buf + sizeof(short);
}

void put_int32(struct packet *packet, int value) {
	*(int *)packet->ptr = htonl(value);
	packet->ptr += sizeof(int);
}

void put_int16(struct packet *packet, short value) {
	*(short *)packet->ptr = htons(value);
	packet->ptr += sizeof(short);
}

int build_packet(struct packet *packet) {
	short ret = packet->ptr - packet->base;
	*(short *)packet->base = htons(ret);

	return ret + sizeof(short);
}

#define loader_mot 8 //PWMO
#define loader_a 4 //DO
#define loader_b 5 //DO
#define loader_F_hall 6 //DI
#define loader_R_hall 26 //DI 

#define layer_1_mot 9 //PWMO
#define layer_2_mot 7 //PWMO

#define plate_L_mot 3 //PWMO
#define plate_L_hall 28 //DI
#define plate_R_mot 11 //PWMO or DO
#define plate_R_hall 29 //DI

#define rear_photo 24 //DI
#define front_photo 22 //DI

#define start_sw 25 //DI
#define reset_sw 27 //DI
#define stop_sw 31 //DI

#define front 0 
#define back 1

#define off 11
#define on 10

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
int plate_ready_cnt=0;
int plate_L_cmd=off;
int plate_R_cmd=off;
int plate_cnt;
int ready_state=off;
clock_t starttime;
clock_t endtime;
int length;
int length_state=ready;
int layer_1_state=ready;
int layer_2_state=ready;
int reset_state=ready;
int start_state=off;
int stop_state=off;
int reset_sw_2=off;

int plate_spd=100;
int layer_1_spd=100;
int layer_2_spd=100;
int loader_spd=70;

void reset_switch(void);
void start_switch(void);
void stop_switch(void);

void start_switch(void)
{
    //if( (ready_state==ready) && (loading_state==ready) ) 
    {
        //loading_state=start;
	start_state=on;
	stop_state=off;
        printf("start!\n");
        delay(1000);
	start_state=off;
    }
}

void stop_switch(void)
{
    printf("Stop!\n");
    stop_state=on;
    
    ready_state=off;
    
    loading_state=off;
    layer_1_state=ready;
    layer_2_state=ready;
    length_state=ready;
    reset_state=ready;
    
    plate_L_cmd=off;
    plate_R_cmd=off;
    plate_cnt=0;
    delay(1000);
    stop_state=off;
    
}

void reset_switch(void)
{
        reset_state=start;
	reset_sw_2=on;
        stop_state=off;
        printf("reset!\n");
        delay(200);
	reset_sw_2=off;
}

void *loading_thread(void *arg) 
{
	while (1)
	{
                switch (loading_state)
                {
                    case off : //11
                        softPwmWrite(loader_mot,0);
                        loading_state = ready;
                        break;
                        
                    case ready : //0
                        break;
                        
                    case start : //1
                        softPwmWrite(loader_mot,loader_spd); // 속도확인 필요
                        digitalWrite(loader_a,1);
                        digitalWrite(loader_b,0);
                        printf("loader move backward!\n");
                        loading_state=middle1;
                        break;
                        
                    case middle1 : //2
                        if( digitalRead(loader_R_hall)==0 ) // loader hall sensor 후방 자석 감지 시
                        {
                                softPwmWrite(loader_mot,0);
                                printf("loader touch backside!\n");
                                loader_pos=back;
                                
                                //loader 정지
                                loading_state=finish;
                        }
                        break;            	
                    case finish : //3
                        if( digitalRead(front_photo) ==1) // f.포토센서에 의류 감지 시
                        {
                                // 의류가 1차layer를 충분히 지나갔다고 판단, loader 복귀시킴
                                softPwmWrite(loader_mot,loader_spd);
                                digitalWrite(loader_a,0); // 방향확인 필요
                                digitalWrite(loader_b,1);
                                printf("loader move forward!\n");
                                //loader 전진
                                loading_state=comeback;
                        }
                        break;            	
                    case comeback :
                        if( digitalRead(loader_F_hall)==0 ) // loader hall sensor 전방 자석 감지 시
                        {
                                softPwmWrite(loader_mot,0);
                                loader_pos=front;
                                printf("loader return!\n");
                                //loader 정지
                                loading_state=ready;
                                ready_state=ready;
                                
                        }
                        break;
                }
	}
}

void *layer_1_thread(void *arg) 
{
	while (1)
	{
                switch (layer_1_state)
                {
                        case ready : //0
                                softPwmWrite(layer_1_mot,0);
                                if( (digitalRead(loader_R_hall)==0) && (ready_state==ready) && (loading_state==finish) ) // loader position이 back 일때
                                {
                                        layer_1_state=start;
                                        ready_state=off;
                                }
                                break;
                                
                        case start : //1
                                softPwmWrite(layer_1_mot,layer_1_spd); // 속도확인 필요
                                //layer1 후진
                                layer_1_state=middle1;
                                printf("layer1 start move!\n");
                                break;   	
     
                        case middle1 : //3
                                if( digitalRead(rear_photo)== 0 ) // r.포토센서에 옷 감지시
                                {
                                        softPwmWrite(layer_1_mot,30);
                                        //layer1 정지
                                        layer_1_state=middle2;
                                }
                                break;
			
                        case middle2 : //4
                                //if( digitalRead(front_photo)==1 ) // f.포토센서에 옷 감지시
                                if( plate_cnt == 2)
				{
                                        softPwmWrite(layer_1_mot,0);
                                        //layer1 정지
                                        layer_1_state=ready;
                                }
                                break;
                }
	}
}

void *layer_2_thread(void *arg) 
{
    while(1)
    {
        switch( layer_2_state )
        {
            case ready : //0
                softPwmWrite(layer_2_mot,0);
                //if( (digitalRead(rear_photo) == 0) && (ready_state==1)) // r.포토센서에 옷 감지
                if ( layer_1_state == middle1 ) // if layer_1 started moving(state == 2)
                {
                        delay(600);
                        softPwmWrite(layer_2_mot,layer_2_spd); // 속도확인 필요
                        layer_2_state=middle1;
                        printf("layer2 start move!\n");
                }
                break;
            case middle1 : //2
                if( digitalRead(front_photo) == 1 ) // f.포토센서에 옷 감지
                {
                	softPwmWrite(layer_2_mot,0); // 이송 정지
                	plate_L_cmd = on;
                	plate_R_cmd = on; // plate에 작동 명령 전달
                	layer_2_state=pause;
                	plate_cnt=1;
                }
                break;
            case pause : //8
                //if( (plate_L_cmd == off) && (plate_R_cmd == off) ) // plate가 모두 작동을 멈추고 나면
            	if( plate_ready_cnt == 2 )
                {
                    plate_ready_cnt = 0;
                    softPwmWrite(layer_2_mot,layer_2_spd); // 이송 재시작
                    if(plate_cnt==1)
                    {
                        length_state=start; // 길이 측정 시작
                        layer_2_state=middle2;
                    }
                    else if(plate_cnt==2)
                    {
			softPwmWrite(layer_2_mot,20);
                        layer_2_state=middle3;
                    }
                }
                break;
            case middle2 : //4
                if( digitalRead(rear_photo) == 1 ) // r.포토센서에 옷 감지->미감
                {
			delay(600);
                	softPwmWrite(layer_2_mot,0); // 이송 정지
                	plate_L_cmd = on;
                	plate_R_cmd = on; // plate에 작동 명령 전달
                	layer_2_state=pause;
                	plate_cnt=2;
                }   
                break;  
            case middle3 : //5
               	if( digitalRead(front_photo) == 0 ) // f.포토센서에 옷 미감지
            	{
			delay(2000);
                	softPwmWrite(layer_2_mot,0); // 이송 정지
                        layer_2_state=ready;
                        plate_cnt=0;
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
            case on: // 10
                printf("plate L move!\n");
                softPwmWrite(plate_L_mot,plate_spd); // 속도확인 필요
                delay(130);
		softPwmWrite(plate_L_mot,2);
		delay(80);
                plate_L_cmd=middle1;
                break;
            case off: // 11
                softPwmWrite(plate_L_mot,0);
                plate_L_cmd = ready;
                break;
            case ready: // 0
                break;
            case middle1: //2
                if( (digitalRead(plate_L_hall)==0) || (stop_state==on) ) // hall sensor 측정위치 복귀 시 or STOP switch
               	{
                        plate_ready_cnt+=1;
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
                printf("plate R move!\n");
                softPwmWrite(plate_R_mot,plate_spd); // 속도확인 필요
		delay(130);
		softPwmWrite(plate_R_mot,2);
		delay(70);
                plate_R_cmd=middle1;
                break;
            case off:
                softPwmWrite(plate_R_mot,0);
                plate_R_cmd = ready;
                break;
            case ready:
                break;
            case middle1:
                if( (digitalRead(plate_R_hall)==0) || (stop_state==on)) // hall sensor 측정위치 복귀 시 or STOP switch
               	{
                        plate_ready_cnt+=1;
                   	softPwmWrite(plate_R_mot,0);
                   	plate_R_cmd = off;                  	
               	}
               	break;
        }
    }
}


void *measure_length(void *arg)
{
    double timegap;
    
    while(1)
    {
        switch (length_state)
        {
            case start :
                starttime=micros();
                //printf("starttime:%d\n",starttime);
                length_state=middle1;
                break;
                
            case middle1 :
                if( digitalRead(rear_photo) == 1) // 옷 끝부분이 r.포토센서를 지나는 순간
                {
                    endtime=micros();
                    //printf("endtime:%d\n",endtime);
                    length_state=finish;
                }
                break;
                
            case finish :
                //timegap=(double)((endtime-starttime)/((double)3000));
                timegap=(endtime-starttime)/(double)1000000;
                length=1*timegap*250+450; //v(mm/s)정보 필요, timegap*v
                printf("length : %d[mm]\n",length);
                length_state=ready; // 길이 측정 완료
                
            case ready :
                break;

        }
    }
}

void *reset_thread(void *arg)
{
        while(1)
        {
                switch(reset_state)
                {
                        case ready: //0
                                break;
                        case start: //1
                                //if(digitalRead(plate_L_hall)==1) // plate_L이 정위치가 아닐때
                                {
                                        softPwmWrite(plate_L_mot,100);
                                        printf("plate L reset...\n");
					delay(130);
					softPwmWrite(plate_L_mot,2);
					delay(80);
					//delay(10);
                                }
				
                                reset_state=middle1;
                                break;
                        case middle1: //2
                                if( digitalRead(plate_L_hall)==0 ) // hall sensor 측정위치 복귀 시
                                {
                                        softPwmWrite(plate_L_mot,0);
                                        printf("plate L reset finish!\n");
                                        reset_state=middle2;
                                }
                                break;
                        case middle2: //4
                                //if(digitalRead(plate_R_hall)==1) //plate_R is not on initial position
                                {
                                        softPwmWrite(plate_R_mot,100);
                                        printf("plate R reset...\n");
					delay(130);
					softPwmWrite(plate_R_mot,2);
					delay(70);
                                }
				
                                reset_state=middle3;
                                break;
                        case middle3: //5
                                if( digitalRead(plate_R_hall)==0 ) // hall sensor 측정위치 복귀 시
                                {
                                        softPwmWrite(plate_R_mot,0);
                                        printf("plate R reset finish!\n");
                                        reset_state=middle4;
                                }
                                break;
                        case middle4: //6
                                if( digitalRead(loader_F_hall) == 1) // loader가 front가 아닐때
                                {
                                        softPwmWrite(loader_mot,loader_spd); // 속도확인 필요
                                        digitalWrite(loader_a,0); // 전진 방향확인 필요
                                        digitalWrite(loader_b,1);
                                        printf("loader reset...\n");
                                }
                                reset_state=middle5;
                        case middle5: //7
                                if( digitalRead(loader_F_hall) == 0) // loader가 front.
                                {
                                        softPwmWrite(loader_mot,0);
                                        printf("loader reset finish!\n");
                                        loading_state=ready;
                                        reset_state=ready;
                                        ready_state=ready;
                                        
                                }

                                
                }
        }
}

int main() 
{
        
        int csock = socket(PF_INET, SOCK_STREAM, 0);
	if (csock == -1) {
		perror("socket");
		return 1;
	}
        
        struct sockaddr_in saddr = {0, };
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("10.179.14.59");
	saddr.sin_port = htons(5000);

	if (connect(csock, (struct sockaddr *)&saddr, sizeof saddr) == -1) {
		perror("connect");
		return 1;
	}
        
	wiringPiSetup();
	
	// 각 pin에 대해서 모두 활성화

    
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
		
        softPwmCreate(loader_mot,0,30); 
        softPwmCreate(layer_1_mot,0,30);
        softPwmCreate(layer_2_mot,0,30);
        softPwmCreate(plate_L_mot,0,30);
        softPwmCreate(plate_R_mot,0,30);        
                        	
	wiringPiISR(start_sw, INT_EDGE_FALLING, start_switch); // 버튼, edge_falling방식, 실행할함수
	wiringPiISR(reset_sw, INT_EDGE_FALLING, reset_switch); // 버튼, edge_falling방식, 실행할함수
	wiringPiISR(stop_sw, INT_EDGE_FALLING, stop_switch);
	
	/*
	구동 전 정렬부
	*/
	

    
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

	pthread_t measurelength;
	pthread_create(&measurelength, NULL, measure_length, NULL);
	pthread_detach(measurelength);
        
        pthread_t resetthread;
	pthread_create(&resetthread, NULL, reset_thread, NULL);
	pthread_detach(resetthread);
        
        char buf[64];
	int ret;

	int i, j;
	short len;
	struct packet packet;
        
        
        while(1)
        {
                init_packet(&packet, buf);
                put_int32(&packet, stop_state);
		put_int32(&packet, reset_state);
                put_int32(&packet, digitalRead(front_photo));
                put_int32(&packet, digitalRead(rear_photo));
                //put_int32(&packet, length);
                put_int32(&packet, plate_cnt);
		put_int32(&packet, start_state);
		put_int32(&packet, reset_sw_2);
		

		len = build_packet(&packet);
		write(csock, packet.base, len);
                
                //printf("[[Operating]] loader:%d, load_F:%d load_R:%d, pht_F:%d, pht_R:%d, layer1:%d, layer2:%d, PlateL:%d(hall:%d), PlateR:%d(hall:%d), PlateCnt:%d Ready:%d Reset:%d\n",loading_state,digitalRead(loader_F_hall), digitalRead(loader_R_hall), digitalRead(front_photo), digitalRead(rear_photo), layer_1_state,layer_2_state,plate_L_cmd, digitalRead(plate_L_hall),plate_R_cmd,digitalRead(plate_R_hall),plate_cnt,ready_state,reset_state);
                printf("loader:%d, loader_r_hall:%d, pht_F:%d, pht_R:%d, ly1:%d, ly2:%d, Plt_L:%d, Plt_R:%d, Plt_Cnt:%d Ready:%d Reset:%d\n",loading_state, digitalRead(loader_R_hall), digitalRead(front_photo), digitalRead(rear_photo), layer_1_state,layer_2_state,plate_L_cmd, plate_R_cmd,plate_cnt,ready_state,reset_state);
                
		delay(100);
        }
        
        close(csock);
          
	return 0;
}
