#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <wiringPi.h>
#include <linux/i2c-dev.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <iostream>
#include "opencv2/opencv.hpp" 

#include "my_def.h"
#include "my_str.h"
#include "getch.h"

#define NUM_THREAD 100
#define VIDEO_FPS	30
#define TEXT_POS	230, 30
#define DURATION	10

using namespace cv;
using namespace std;

int flag;
int fd;
int loop = 1;
int run = 1;
unsigned char buffer[3] = {0};
unsigned short front_wheel;
short back_wheel;
unsigned short cam_UD;
unsigned short cam_LR;
struct dir d1;
int serv_sock;
int clnt_sock;

void error_handling(char *message);

int reg_read8(unsigned char addr)
{
	int length = 1;
	buffer[0] = addr;

	if(write(fd, buffer, length) != length)
	{
		printf("Failed to write frome the i2c bus\n");
	}

	if(read(fd , buffer, length) != length)
	{
		printf("Failed to read from the i2c bus\n");
	}

	//printf("addr[%d] = %d\n", addr, buffer[0]);

	return 0;

}

int reg_read16(unsigned char addr)
{
	unsigned short temp;

	reg_read8(addr);
	temp = 0xff & buffer[0];         // 상위 8bit 값만 temp에 입력

	reg_read8(addr + 1);
	temp |= (buffer[0] << 8);        // 하위 8bit 값만 temp에 입력


	//printf("addr = 0x%x, data = 0x%x, data = %d\n", addr, temp, temp);

	return 0;
}

int reg_write8(unsigned char addr,unsigned char data)
{
	int length = 2;

	buffer[0] = addr;
	buffer[1] = data;

	if(write(fd , buffer, length) != length)
	{
		printf("Failed to write from the i2c bus\n");
		return -1;
	}

	return 0;
}

int reg_write16(unsigned char addr, unsigned short data)
{
	int length = 2;

	reg_write8(addr, (data & 0xff));
	reg_write8(addr+1, ((data>>8) & 0xff));

	return 0;
}

int pca9685_restart(void)
{
	int length; 

	if(ioctl(fd, I2C_SLAVE, PCA_ADDR) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave\n");
		return -1;
	}

	reg_write8(MODE1, 0x00);
	reg_write8(MODE2, 0x04); // 0x10
	return 0;
}

int pca9685_freq()
{
	int freq = 50;
	uint8_t prescale_val = (CLOCK_FREQ / 4096 / freq) -1; // preescale value. ref)Data sheet
	printf("prescale_val = %d\n",prescale_val);

	// OP : OSC OFF
	reg_write8(MODE1, 0x10);

	// OP : WRITE PRE_SCALE VALUE
	reg_write8(PRE_SCALE,prescale_val);

	// OP : RESTART
	reg_write8(MODE1, 0x80);

	// OP : TOTEM POLE
	reg_write8(MODE2, 0x04);

	return 0;
}

void servoOFF(void)
{
	reg_write8(MODE1, 0x10);
}

int Move(short speed)
{
	short max = 4095;
	short min = -4095;

	if(speed > 0)
	{
		digitalWrite(GPIO17, LOW);
		digitalWrite(GPIO27, LOW);
	}

	if(speed < 0)
	{
		speed *= -1;
		digitalWrite(GPIO17, HIGH);
		digitalWrite(GPIO27, HIGH);
	}

	reg_write16(LED4_ON_L, 0);
	reg_write16(LED5_ON_L, 0);
	reg_write16(LED4_OFF_L, speed);
	reg_write16(LED5_OFF_L, speed);
}


void *move(void * arg)
{
	int i;
	int input_h;
	char input_c;
	char input_n;
	int loop = 1;
	struct dir d2;

	while(loop)
	{
		read(clnt_sock, &d2, sizeof(d2));

		switch(d2.input_h)
		{
			//REG 15 // front_wheel
		case LEFT:                              
			if(front_wheel > 195)
				front_wheel -= 5;
			break;

		case RIGHT:
			if(front_wheel < 405)
				front_wheel += 5;   
			break;

			//back_wheel
		case UP:
			if(back_wheel <= AX)                       
				back_wheel += 40;

			break;

		case DOWN:
			if(back_wheel > -AX)                   
				back_wheel -= 40;
			break;
		}


		switch(d2.input_h)
		{
			//REG 14 // cam_UD
		case 'w':
			if(cam_UD > C_MIN)
				cam_UD -= 5;
			break;

		case 's':
			if(cam_UD < C_MAX)
				cam_UD += 5;   
			break;

			//REG 13 // cam_LR
		case 'd':
			if(cam_LR > C_MIN)
				cam_LR -= 5;
			break;

		case 'a':
			if(cam_LR < C_MAX)
				cam_LR += 5;
			break;

			// MAX motol speed
		case 'f':
			if(back_wheel > 0)
				back_wheel = AX;
			else
				back_wheel = -AX;
			break;

			// MIN motol speed
		case 'r':
			if(back_wheel > 0)
				back_wheel = IN;
			else
				back_wheel = -IN;
			break;

			// STEP motol speed
		case 'n':
			back_wheel = 0;
			break;

			// Change direction
		case 'c':
			back_wheel *= -1;
			break;

			// STOP    
		case 'p':  
			loop = 0;
			run = 0;
			break;       
		}

		Move(back_wheel);
		// printf("input_h = %c\n",d2.input_h);
		// printf("back_wheel = %d\n",back_wheel);
		reg_write16(LED13_ON_L, 0);
		reg_write16(LED13_OFF_L, cam_LR);

		reg_write16(LED14_ON_L, 0);
		reg_write16(LED14_OFF_L, cam_UD);

		reg_write16(LED15_ON_L, 0);
		reg_write16(LED15_OFF_L, front_wheel);
		usleep(5000);
		//printf("front_wheel : %d\n", front_wheel);

	}
}

void *ocv(void *arg)
{
	pthread_t p_thread;
	IplImage *frame = 0;

	CvCapture *capture = cvCreateCameraCapture(0);
	cvNamedWindow("video", CV_WINDOW_AUTOSIZE);

	cvGrabFrame(capture);
	frame = cvRetrieveFrame(capture);

	while (run) {
		//cvGrabFrame(동영상) : 하나의 프레임을 잡음
		cvGrabFrame(capture);
		//cvRetrieveFrame(동영상) : 잡은 프레임으로부터 이미지를 구함
		frame = cvRetrieveFrame(capture);

		if (!frame || cvWaitKey(10) >= 0) { break; }


		//if (!frame || cvWaitKey(10) >= 0) { break; }

		//cvWriteFrame(동영상, 프레임) : 지정한 동영상에 프레임을 쓴다
		//cvWriteFrame(writer, frame);

		//화면에 출력하는 함수
		cvShowImage("video", frame);
	}

	cvReleaseCapture(&capture);
	cvDestroyWindow("video");

	//printf("%ld.%d\n",UTCtime_r.tv_sec,UTCtime_r.tv_usec);
	//return 0;

}


/*int led_on(unsigned short value)
{
char key;
unsigned short time_val = 4095;

while(1)
{

if(value <= time_val)
{
while(value <= time_val)
{
value += LED_STEP;               
printf("O : value = %d\n",value);
reg_write16(LED15_ON_L, 0);     //16bit이기 때문에 High, Low 둘다 써짐
reg_read16(LED15_ON_L);

reg_write16(LED15_OFF_L, value);
reg_read16(LED15_OFF_L);
}       
}

else if(value >= time_val)
{   
while(value >= LED_STEP)
{
value -= LED_STEP;
printf("U : value = %d\n",value);
reg_write16(LED15_ON_L, 0);                 // LED ON 지점은 고정
reg_read16(LED15_ON_L);

reg_write16(LED15_OFF_L, value);            // LED OFF 지점을 옮김
reg_read16(LED15_OFF_L);


}
}    
}

return 0;
}*/



int main(int argc, char *argv[])
{

	pthread_t thread_id[NUM_THREAD];

	char message[30];

	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;


	if(argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
		error_handling("socket() error");

		memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock,(struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
		 error_handling("bind() error");

		if(listen(serv_sock, 5) == -1)
			error_handling("listen() error");

			clnt_addr_size = sizeof(clnt_addr);

	clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
	if(clnt_sock == -1)
		error_handling("accept() error");

		// write(clnt_sock, &d1, sizeof(d1));
		// read(clnt_sock, &d1, sizeof(d1));


		unsigned short value = 2047;

	if((fd = open(I2C_DEV, O_RDWR)) < 0)
	{
		printf("Failed open i2c-1 bus\n");
		return -1;
	}

	if(ioctl(fd,I2C_SLAVE,PCA_ADDR) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave\n");
		return -1;
	}


	//write(clnt_sock, &d1, sizeof(d1));
	//read(clnt_sock, &d1, sizeof(d1));

	wiringPiSetup();
	pinMode(GPIO17, OUTPUT);
	pinMode(GPIO27, OUTPUT);
	pca9685_restart();
	pca9685_freq();

	front_wheel     = 300;
	back_wheel      = 1000;
	cam_UD          = 300; 
	cam_LR          = 300;

	printf("Set start value\n");
	printf("front_wheel = %d\n",front_wheel);
	printf("back_wheel = %d\n",back_wheel);
	printf("cam_UD = %d\n",cam_UD);
	printf("cam_LR = %d\n",cam_LR);


	printf("Start to move\n");
	pthread_create(&(thread_id[0]), NULL, move, NULL);
	printf("Start to video\n");
	pthread_create(&(thread_id[1]), NULL, ocv, NULL);
	pthread_join(thread_id[0],NULL);
	pthread_join(thread_id[1],NULL);

	servoOFF();

	return 0;
}
void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}