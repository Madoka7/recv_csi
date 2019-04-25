/*
 * =====================================================================================
 *       Filename:  main.c
 *
 *    Description:  Here is an example for receiving CSI matrix 
 *                  Basic CSi procesing fucntion is also implemented and called
 *                  Check csi_fun.c for detail of the processing function
 *        Version:  1.0
 *
 *         Author:  Yaxiong Xie
 *         Email :  <xieyaxiongfly@gmail.com>
 *   Organization:  WANDS group @ Nanyang Technological University
 *   
 *   Copyright (c)  WANDS group @ Nanyang Technological University
 =======================================================================================
 *   Modified by Xiangqi Kong, 2017
 * . Dalian University of Technology
 *   Description: Extract CSI from NIC then real-time write to MATLAB using socket to plot graph.
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>


//  
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/socket.h>
#include <netinet/in.h>
#define SERV_PORT 8000  //

#include "csi_fun.h"

#define BUFSIZE 4096 

int quit;
unsigned char buf_addr[BUFSIZE];
unsigned char data_buf[1500];

COMPLEX csi_matrix[3][3][114];
csi_struct*   csi_status;

void sig_handler(int signo)
{
    if (signo == SIGINT)
        quit = 1;
}

int main(int argc, char* argv[])
{
    FILE*       fp;
    int         fd;
    int         i;
    int         total_msg_cnt,cnt;
    int         log_flag;
    unsigned char endian_flag;
    u_int16_t   buf_len;




	//
    int         payload_len;
	char        serv_ip[16];
	int         sockfd;
	struct      sockaddr_in servaddr;
	
	/* add a socket */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket error\n");
		return 0;
	}

	/* initialize serveraddr struct */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	//servaddr.sin_addr.s_addr = INADDR_ANY;    //

	
	


    log_flag = 1;
    csi_status = (csi_struct*)malloc(sizeof(csi_struct));
    /* check usage */
    if (1 == argc){
        /* If you want to log the CSI for off-line processing,
         * you need to specify the name of the output file
         */
        log_flag  = 0;
        printf("/**************************************/\n");
        printf("/*   Usage: recv_csi <ipaddress>    */\n");
        printf("/**************************************/\n");


    }
    if (2 == argc){

		//DULT
        /* 
		*fp = fopen(argv[1],"w");
        *if (!fp){
        *    printf("Fail to open <output_file>, are you root?\n");
        *    fclose(fp);
        *    return 0;
        *}
		*/

		strcpy(serv_ip, argv[1]);
		if(inet_pton(AF_INET, serv_ip, &servaddr.sin_addr)<0)
		{
			printf("inet_pton error\n");
			return 0;
		}
		
		if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
		{
			printf("connect error to: %s \n", serv_ip);
			return 0;
		}                                                         ////    
        endian_flag = 0x01;
        //  //fwrite(&endian_flag,1,1,sockfd);        

		



    }
    if (argc > 2){
        printf(" Too many input arguments !\n");
        return 0;
    }

    fd = open_csi_device();
    if (fd < 0){
        perror("Failed to open the device...");
        return errno;
    }
    
    printf("#Receiving data! Press Ctrl+C to quit!\n");

    quit = 0;
    total_msg_cnt = 0;
    
	sleep(1);

	write(sockfd, &endian_flag, sizeof(endian_flag));


    while(1){
        if (1 == quit){
            
            //fclose(sockfd);
            close_csi_device(fd);
			return 0;
        }

        /* keep listening to the kernel and waiting for the csi report */
        cnt = read_csi_buf(buf_addr,fd,BUFSIZE);

        if (cnt){
            total_msg_cnt += 1;

            /* fill the status struct with information about the rx packet */
            record_status(buf_addr, cnt, csi_status);

            /* 
             * fill the payload buffer with the payload
             * fill the CSI matrix with the extracted CSI value
             */
            record_csi_payload(buf_addr, csi_status, data_buf, csi_matrix); 
            
            /* Till now, we store the packet status in the struct csi_status 
             * store the packet payload in the data buffer
             * store the csi matrix in the csi buffer
             * with all those data, we can build our own processing function! 
             */
            //porcess_csi(data_buf, csi_status, csi_matrix);   
            



			/* standard buff_len is: 989 payload_len is: 124 msg rage is: 0x8e*/
            printf("Recv %dth msg with rate: 0x%02x | payload len: %d\n",total_msg_cnt,csi_status->rate,csi_status->payload_len);
            printf("buf_len is :%d \n", csi_status->buf_len);
            /* log the received data for off-line processing */
            if (log_flag){
                buf_len = csi_status->buf_len;

				if(buf_len > 1000)
					//continue;

				payload_len = csi_status->payload_len;


				// Even I changed data here, the data in buffer is still not changed. So I have to chang it in buffer
				// Or on the receive part


				// Trans to Network Endian ? why did he trans that Endian in csi_fun.c

				// Not to trans this.


				csi_status->payload_len = htons(payload_len);

				// I decided let tstamp died it self.
				//csi_status->tstamp = htonl(csi_status->tstamp); 
				csi_status->channel = htons(csi_status->channel);
				csi_status->csi_len = htons(csi_status->csi_len);
				


				/*
                *fwrite(&buf_len,1,2,sockfd);
                *fwrite(buf_addr,1,buf_len,sockfd);  ////
				*/                                  ///
				
				/* Current PC is little endian, trans data to network endian*/
				buf_len = htons(buf_len);
				write(sockfd, &buf_len, sizeof(buf_len));
				buf_len = ntohs(buf_len);
				write(sockfd, buf_addr, csi_status->buf_len);

            }
        }
    }
   // fclose(fp);
    close_csi_device(fd);
    free(csi_status);
    return 0;
}
