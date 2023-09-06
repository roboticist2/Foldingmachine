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
	char *rptr;
};

void init_packet(struct packet *packet, char *buf) {
	packet->base = buf;
	packet->rptr = buf;

	packet->ptr = buf + sizeof(short);
}

int get_int32(struct packet *packet) {
	int ret = *(int *)packet->rptr;
	packet->rptr += sizeof(int);
	return ntohl(ret);
}

short get_int16(struct packet *packet) {
	short ret = *(short *)packet->rptr;
	packet->rptr += sizeof(short);
	return ntohs(ret);
}

ssize_t readn(int fd, void *vptr, size_t n)
{
	size_t  nleft;
	ssize_t nread;
	char    *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;  
			else
				return(-1);
		} else if (nread == 0)
			break;         

		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);      /* return >= 0 */
}

struct packet {
	char *base;
	char *ptr;
	char *rptr;
};

void init_packet(struct packet *packet, char *buf) {
	packet->base = buf;
	packet->rptr = buf;

	packet->ptr = buf + sizeof(short);
}

int get_int32(struct packet *packet) {
	int ret = *(int *)packet->rptr;
	packet->rptr += sizeof(int);
	return ntohl(ret);
}

short get_int16(struct packet *packet) {
	short ret = *(short *)packet->rptr;
	packet->rptr += sizeof(short);
	return ntohs(ret);
}

ssize_t readn(int fd, void *vptr, size_t n)
{
	size_t  nleft;
	ssize_t nread;
	char    *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;  
			else
				return(-1);
		} else if (nread == 0)
			break;         

		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);      /* return >= 0 */
}

#define layer_3_1_mot 4
#define layer_3_4_pwm 28

#define layer_3_2_a 24
#define layer_3_2_b 25
//#define layer3_2_pwm 15

#define layer_4_1_a 22
#define layer_4_1_b 23
//#define layer4_1_pwm 5

#define layer_4_2_a 30
#define layer_4_2_b 21
//#define layer4_2_pwm 6

#define layer_4_3_a 15
#define layer_4_3_b 16
//#define layer4_3_pwm 11

#define unloading_a 2
#define unloading_b 3
#define unloading_pwm 29

#define edge_3_mot 5
#define edge_3_hall 4
#define edge_4_1_hall 6
#define edge_4_1_mot 7
#define edge_4_2_hall 31
#define edge_4_2_mot 11

#define edge_3_photo 26
#define edge_4_1_photo 27
#define edge_4_2_photo 28 

#define front 0 
#define back 1

#define off 11
#define on 10

#define ready 0
#define start 1
#define finish 2
#define middle1 3
#define middle2 4
#define middle3 5
#define middle4 6
#define middle5 7
#define middle6 8

#define pause 9
#define comeback 10

int plate_L_cmd=off;
int plate_R_cmd=off;

int layer_3_1_state=ready;
int layer_3_2_state=ready;
int layer_4_1_state=ready;
int layer_4_2_state=ready;
int layer_4_3_state=ready;
int layer_4_state=ready;
int reset_state=ready;
int reset_state_2=ready;
int reset_sw_2=off;

int edge_3_1_cmd=off;
int edge_4_1_cmd=off;
int edge_4_2_cmd=off;

int layer_3_4_spd=100;
int unloading_spd=40;

int stop_state=0;
int start_state=0;
int front_photo=0;
int rear_photo=0;
int length=660; // length fixed!
int plate_cnt=0;

int v=250;

void *start_thread(void *arg) 
{
	while(1)
	{
		if( start_state == on)
		{
			printf("start!\n");
			layer_3_1_state=start;
			layer_3_2_state=start;
			delay(100);
			
		}
	}
}

void *stop_thread(void *arg) 
{
	while(1)
	{
		if( stop_state == on)
		{
			printf("stop!\n");
			layer_3_1_state=off;
			layer_3_2_state=off;
			layer_4_state=off;
			plate_cnt=0;
			delay(500);
		}
	}
}

void *layer_3_1_thread(void *arg) 
{
	while(1)
	{
		switch(layer_3_1_state)
		{
			case ready : //0
				if(plate_cnt == 1)
				{
					layer_3_1_state=start;
				}
				break;
				
			case start: // 1
				digitalWrite(layer_3_1_mot,1);
				printf("layer3_1 start move!\n");				
				layer_3_1_state=middle1;
				break;
				
			case middle1 : //3
				if( digitalRead(edge_3_photo) == 0 )
				{
					delay(1000);
					layer_3_1_state=middle2;
				}
				break;
				
			case middle2 : //4
				if( digitalRead(edge_3_photo) == 1 )
				{
					layer_3_1_state=off;
				}
				break;
				
			case off: //11
				digitalWrite(layer_3_1_mot,0);
				layer_3_1_state=ready;
				break;
		}
	}
}

void *layer_3_2_thread(void *arg) 
{
	while(1)
	{
		switch(layer_3_2_state)
		{
			case ready: //0
				//if( digitalRead(edge_3_photo) == 0 )
				if( plate_cnt == 1 )
				{
					layer_3_2_state=start;
				}
				break;
			case start: //1
				printf("layer3_2 start move!\n");
				digitalWrite(layer_3_2_a,1);
				digitalWrite(layer_3_2_b,0);
				layer_3_2_state=middle1;
				layer_4_state=start;
				break;
			case middle1: //3
				if( digitalRead(edge_4_1_photo) == 0 )
				{
					delay(2000);
					layer_3_2_state=off;
				}
				break;
		
			case off: //11
				digitalWrite(layer_3_2_a,0);
				digitalWrite(layer_3_2_b,0);
				layer_3_2_state=ready;
				break;

		}
	}
}

void *layer_4_thread(void *arg) 
{
	while(1)
	{
		switch(layer_4_state)
		{
			case ready: // 0
				
				if( digitalRead(edge_4_1_hall) == 1)
				{
					digitalWrite(edge_4_1_mot,1);
				}
				
				if( digitalRead(edge_4_1_hall) == 0 )
				{
					digitalWrite(edge_4_1_mot,0);
				}

				
				if( digitalRead(edge_4_2_hall) == 1 )
				{
					digitalWrite(edge_4_2_mot,1);
				}

				if( digitalRead(edge_4_2_hall) == 0 )
				{
					digitalWrite(edge_4_2_mot,0);
				}
				break;
			
			case start: // 1, phase1, convey
			
			
				delay(500);
				printf("layer4 start move!\n");
				digitalWrite(layer_4_1_a,0); 
				digitalWrite(layer_4_1_b,1);
				
				digitalWrite(layer_4_2_a,0);
				digitalWrite(layer_4_2_b,1);
				
				digitalWrite(layer_4_3_a,0);
				digitalWrite(layer_4_3_b,1);
				
				layer_4_state=middle1;
				
				break;
				
			case middle1: // 3
				if( digitalRead(edge_4_1_photo) == 0 )
				{
					delay(400*length/v);
					layer_4_state=pause;					
					edge_4_2_cmd = on;
				}
				break;
					
			case middle2: //4, phase2, 2/3 folding, lead-in
					
					delay(3000);
					digitalWrite(layer_4_1_a,1); 
					digitalWrite(layer_4_1_b,0);
									
					digitalWrite(layer_4_2_a,1);
					digitalWrite(layer_4_2_b,0);
					
					digitalWrite(layer_4_3_a,0);
					digitalWrite(layer_4_3_b,1);
					
					delay(400*length/v);
									
					layer_4_state=middle3;								
		
				break;
				
			case middle3: // 5, phase3, 2/3 emission
				//if( digitalRead(edge_4_1_photo) == 0 )
				{
					
					digitalWrite(layer_4_1_a,0);
					digitalWrite(layer_4_1_b,0);
					digitalWrite(layer_4_2_a,0);
					digitalWrite(layer_4_2_b,0);
					digitalWrite(layer_4_3_a,0);
					digitalWrite(layer_4_3_b,0);					
					
					delay(3000);

					digitalWrite(layer_4_1_a,0);
					digitalWrite(layer_4_1_b,1);
					
					digitalWrite(layer_4_2_a,0);
					digitalWrite(layer_4_2_b,1);
					
					digitalWrite(layer_4_3_a,1);
					digitalWrite(layer_4_3_b,0);
					
					delay(400*length/v);
					
					layer_4_state=middle4;								
				}
				break;
				
			case middle4: // 6, phase4, convey
				//if( digitalRead(edge_4_1_photo) == 0 )
				{

					digitalWrite(layer_4_1_a,0);
					digitalWrite(layer_4_1_b,0);
					digitalWrite(layer_4_2_a,0);
					digitalWrite(layer_4_2_b,0);
					digitalWrite(layer_4_3_a,0);
					digitalWrite(layer_4_3_b,0);					
					delay(3000);

					digitalWrite(layer_4_1_a,1);
					digitalWrite(layer_4_1_b,0);
					
					digitalWrite(layer_4_2_a,1);
					digitalWrite(layer_4_2_b,0);
					
					digitalWrite(layer_4_3_a,1);
					digitalWrite(layer_4_3_b,0);
					
					delay(400*length/v); 
					
					layer_4_state=middle5;
													
				}
				break;
			
			case middle5: // 7
				if( digitalRead(edge_4_1_photo) == 0 )
				{
					layer_4_state=pause;					
					edge_4_1_cmd = on;
				}
				break;
				
			case middle6: //8, phase5, 1/3 folding
				{
					delay(3000);
					digitalWrite(layer_4_1_a,1);
					digitalWrite(layer_4_1_b,0);
					
					digitalWrite(layer_4_2_a,0);
					digitalWrite(layer_4_2_b,1);
					
					digitalWrite(layer_4_3_a,0);
					digitalWrite(layer_4_3_b,1);
					
					delay(400*length/v);
					
					layer_4_state=finish;													
				}
				break;
				
							
			case pause: //9

				digitalWrite(layer_4_1_a,0);
				digitalWrite(layer_4_1_b,0);
				digitalWrite(layer_4_2_a,0);
				digitalWrite(layer_4_2_b,0);
				digitalWrite(layer_4_3_a,0);
				digitalWrite(layer_4_3_b,0);				
				
				break;
			
			case finish: // 2, emission
				//if( digitalRead(edge_4_1_photo) == 1 )
				{

					digitalWrite(layer_4_1_a,0);
					digitalWrite(layer_4_1_b,0);
					digitalWrite(layer_4_2_a,0);
					digitalWrite(layer_4_2_b,0);
					digitalWrite(layer_4_3_a,0);
					digitalWrite(layer_4_3_b,0);
					
					layer_4_state=off;
					
				}
				break;
			
			case off: // 11
				digitalWrite(layer_4_1_a,0);
				digitalWrite(layer_4_1_b,0);
				digitalWrite(layer_4_2_a,0);
				digitalWrite(layer_4_2_b,0);
				digitalWrite(layer_4_3_a,0);
				digitalWrite(layer_4_3_b,0);
				layer_4_state=ready;
				break;
		}		
	}
}

void *edge_3_thread(void *arg) 
{
	while(1)
	{

	}
}

void *edge_4_1_thread(void *arg) 
{
	while(1)
	{
		switch(edge_4_1_cmd)
		{
		    case on: // 10
			printf("edge 4_1 move!\n");
			digitalWrite(edge_4_1_mot,1);
			delay(300);
			edge_4_1_cmd=middle1;
			break;
		    case off: // 11
			digitalWrite(edge_4_1_mot,0);
			edge_4_1_cmd = ready;
			break;
		    case ready: // 0
			break;
		    case middle1: //2
			if( (digitalRead(edge_4_1_hall) == 0) || (stop_state==on) )
			{
				//softPwmWrite(edge_4_1_mot,0);
				edge_4_1_cmd=off;
				layer_4_state=middle6;
			}
			break;
		}
	}
}

void *edge_4_2_thread(void *arg) 
{
	while(1)
	{
		switch(edge_4_2_cmd)
		{
		    case on: // 10
			printf("edge 4_1 move!\n");
			digitalWrite(edge_4_2_mot,1);
			delay(300);
			edge_4_2_cmd=middle1;
			break;
		    case off: // 11
			digitalWrite(edge_4_2_mot,0);
			edge_4_2_cmd = ready;
			break;
		    case ready: // 0
			break;
		    case middle1: //2
			if( (digitalRead(edge_4_2_hall) == 0) || (stop_state==on) )
			{
				//softPwmWrite(edge_4_1_mot,0);
				edge_4_2_cmd=off;
				layer_4_state=middle2;                	
			}
			break;
		}
	}
}

void *layer_4_1_thread(void *arg) 
{
	while(1)
	{

	}
}

void *layer_4_2_thread(void *arg) 
{
	while(1)
	{
		
	}
}

void *layer_4_3_thread(void *arg) 
{
	while(1)
	{
		
	}
}

void *unloading_thread(void *arg) 
{
	while(1)
	{
		
	}
}

void *reset_thread(void *arg)
{
        while(1)
        {
		switch(reset_state_2)
		{
			case ready: //0
				if(reset_sw_2 == on)
				{
					printf("reset!\n");
					reset_state_2=start;
				}
				break;
				
			case start: //1
				if( digitalRead(edge_4_1_hall) == 1)
				{
					digitalWrite(edge_4_1_mot,1);
				}
				reset_state_2=middle1;
				break;
				
			case middle1:
				if( digitalRead(edge_4_1_hall) == 0 )
				{
					digitalWrite(edge_4_1_mot,0);
					reset_state_2=middle2;
				}
				break;
				
			case middle2:
				if( digitalRead(edge_4_2_hall) == 1 )
				{
					digitalWrite(edge_4_2_mot,1);
				}
				reset_state_2=middle3;
				break;
				
			case middle3:
				if( digitalRead(edge_4_2_hall) == 0 )
				{
					digitalWrite(edge_4_2_mot,0);
					reset_state_2=ready;
				}
				break;
				
		}			
        }
}

int main() 
{
    int ssock = socket(PF_INET, SOCK_STREAM, 0);
	if (ssock == -1) {
		perror("socket");
		return 1;
	}
        
    int option = 1;
	setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof option);

	struct sockaddr_in saddr = {0, };
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(5000);
	if (bind(ssock, (struct sockaddr *)&saddr, sizeof saddr) == -1) {
		perror("bind");
		return 1;
	}

	if (listen(ssock, SOMAXCONN) == -1) {
		perror("listen");
		return 1;
	}
        
	wiringPiSetup();
	
	//  pinÐ  t ¨P \1T

    
       	pinMode(layer_3_1_mot, OUTPUT);
      	pinMode(layer_3_2_a, OUTPUT);
	pinMode(layer_3_2_b, OUTPUT);
      	pinMode(layer_4_1_a, OUTPUT);
	pinMode(layer_4_1_b, OUTPUT);
	pinMode(layer_4_2_a, OUTPUT);
	pinMode(layer_4_2_b, OUTPUT);
	pinMode(layer_4_3_a, OUTPUT);
	pinMode(layer_4_3_b, OUTPUT);
	pinMode(unloading_a, OUTPUT);
	pinMode(unloading_b, OUTPUT);
	pinMode(edge_3_mot, OUTPUT);
	pinMode(edge_4_1_mot, OUTPUT);
	pinMode(edge_4_2_mot, OUTPUT);
	
//	pinMode(edge_3_hall, INPUT);
	pinMode(edge_4_1_hall, INPUT);
	pinMode(edge_4_2_hall, INPUT);
	pinMode(edge_3_photo, INPUT);
	pinMode(edge_4_1_photo, INPUT);
	
    softPwmCreate(layer_3_4_pwm,0,40);
	softPwmWrite(layer_3_4_pwm,100);


//	pthread_t thread;
//	pthread_create(&thread, NULL, fnd_thread, NULL);
//	&ðÜÀ,NULL,ä`ðÜ,NULL
//	pthread_detach(thread);
//	//------------------------

//	pthread_t rollerthread;
//	pthread_create(&rollerthread, NULL, roller_thread, NULL); 
//	pthread_detach(rollerthread);
	
	pthread_t layer3_1thread;
	pthread_create(&layer3_1thread, NULL, layer_3_1_thread, NULL);
	pthread_detach(layer3_1thread);
	
	pthread_t layer3_2thread;
	pthread_create(&layer3_2thread, NULL, layer_3_2_thread, NULL);
	pthread_detach(layer3_2thread);
	
	pthread_t layer4thread;
	pthread_create(&layer4thread, NULL, layer_4_thread, NULL);
	pthread_detach(layer4thread);
	
	pthread_t layer4_1thread;
	pthread_create(&layer4_1thread, NULL, layer_4_1_thread, NULL);
	pthread_detach(layer4_1thread);
	
	pthread_t layer4_2thread;
	pthread_create(&layer4_2thread, NULL, layer_4_2_thread, NULL);
	pthread_detach(layer4_2thread);
	
	pthread_t layer4_3thread;
	pthread_create(&layer4_3thread, NULL, layer_4_3_thread, NULL);
	pthread_detach(layer4_3thread);
	
	pthread_t unloadingthread;
	pthread_create(&unloadingthread, NULL, unloading_thread, NULL);
	pthread_detach(unloadingthread);
	
	pthread_t edge3thread;
	pthread_create(&edge3thread, NULL, edge_3_thread, NULL);
	pthread_detach(edge3thread);
	
	pthread_t edge4_1thread;
	pthread_create(&edge4_1thread, NULL, edge_4_1_thread, NULL);
	pthread_detach(edge4_1thread);
	
	pthread_t edge4_2thread;
	pthread_create(&edge4_2thread, NULL, edge_4_2_thread, NULL);
	pthread_detach(edge4_2thread);
	

	pthread_t resetthread;
	pthread_create(&resetthread, NULL, reset_thread, NULL);
	pthread_detach(resetthread);
	
	pthread_t stopthread;
	pthread_create(&stopthread, NULL, stop_thread, NULL);
	pthread_detach(stopthread);
 
	pthread_t startthread;
	pthread_create(&startthread, NULL, start_thread, NULL);
	pthread_detach(startthread);
        
	char buf[64];
	int ret;

	int i, j;
	short len;
	struct packet packet;
        
        
	while(1)
	{
		struct sockaddr_in caddr = {0, };
		socklen_t caddrlen = sizeof caddr;
		int csock = accept(ssock, (struct sockaddr *)&caddr, &caddrlen);
		if (csock == -1) 
		{
			perror("accept");
			return 1;
		}

		printf("client: %s\n", inet_ntoa(caddr.sin_addr));

		int ret;
		char buf[8];
		int i = 0;
		struct packet packet;
		while (1) 
		{
			char *p;
			short len;
			ret = readn(csock, buf, sizeof len);
			if (ret <= 0)
				break;

			len = ntohs(*(short *)buf);
			ret = readn(csock, buf, len);
			if (ret <= 0) {
				break;
			}
			
			init_packet(&packet, buf);
			stop_state = get_int32(&packet);
			reset_state = get_int32(&packet);
			front_photo = get_int32(&packet);
			rear_photo = get_int32(&packet);
			//length = get_int32(&packet);
			plate_cnt = get_int32(&packet);
			start_state = get_int32(&packet);
			reset_sw_2 = get_int32(&packet);
			
			//printf("stop:%d, reset:%d, ly1_pht_f:%d, ly1_pht_r:%d, length:%d, plate_cnt:%d\n", stop_state,reset_state,front_photo,rear_photo,length,plate_cnt);
			printf("plt_cnt:%d, ly3_pht:%d, layer3_1:%2d, layer3_2:%2d, ly4_1_pht:%d, ly4_2_pht:%d, layer4:%d, edge_4_f:%d, edge_4_r:%d, length:%d\n",plate_cnt,digitalRead(edge_3_photo),layer_3_1_state,layer_3_2_state,digitalRead(edge_4_1_photo),digitalRead(edge_4_2_photo),layer_4_state,digitalRead(edge_4_1_hall),digitalRead(edge_4_2_hall),length);
		}
		
		close(csock);
	}
	
	close(ssock);
	return 0;
}
