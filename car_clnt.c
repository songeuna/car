#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "my_str.h"
#include "getch.h"

void error_handling(char *message);

int main(int argc, char* argv[])
{
		int i;
		struct dir d1;
		int sock;
		struct sockaddr_in serv_addr;
		char message[30];
		int str_len;
		char* test_addr;
		int temp;

		if(argc != 3){
				printf("Usage : %s <IP> <PORT>\n", argv[0]);
				exit(1);
		}

		sock = socket(PF_INET, SOCK_STREAM, 0);
		if(sock == -1)
				error_handling("socket() error");

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
		serv_addr.sin_port=htons(atoi(argv[2]));

		test_addr = inet_ntoa(serv_addr.sin_addr);
		printf("serv_addr = %s\n",test_addr);

		if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
				error_handling("connect() error");


		printf("Welcome! This is client\n");

		while(1)
		{
//				printf("hi\n");
				d1.input_h = getch();

				if(d1.input_h == 91)
				{
						d1.input_h = getch();

				}

	//			printf("input_h = %c\n", d1.input_h);
				

				write(sock, &d1, sizeof(d1));
		}

//		write(sock, &d1, sizeof(d1));

		close(sock);
		return 0;
}

void error_handling(char *message)
{
		fputs(message, stderr);
		fputc('\n', stderr);
		exit(1);
}
						





					
					

