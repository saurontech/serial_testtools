#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include "advioctl.h"
#include <sys/time.h>
//#include <termios.h>
#include <asm-generic/termbits.h>


char buf [1024];
char tmpbuf[1024];

int main(int argc, char **argv)
{
	int fd;
	int i;
	int ret;
	struct termios2 newtio;
	fd_set wfds;
	fd_set rfds;
	struct timeval tv;
	int retval;
	int wlen;
	int rlen;
	int diff;
	int widx;
	int ridx;
	int total_tx;
	int total_rx;
	int clnlen;
	int rd_size;
	char * dev;

	if(argc != 2){
		printf("%s [tty_path]\n", argv[0]);
		return 0;
	}

	dev = argv[1];

	for(i = 0; i < 1024; i++){
		if(i == 0){
			buf[i] = '!';
		}else{
			buf[i] = buf[i - 1] + 1;
			if(buf[i] > '}'){
				buf[i]= '!';
			}
		}
	}

	printf("done init\n");
	
	fd = open(dev, O_RDWR);
	printf("fd = %d\n", fd);


	ret = ioctl(fd, TCGETS2, &newtio);
	printf("original ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);
	newtio.c_iflag &= ~(ISTRIP|IUCLC|IGNCR|ICRNL|INLCR|ICANON|IXON|PARMRK);
	newtio.c_iflag |= (IGNBRK|IGNPAR);
	newtio.c_lflag &= ~(ECHO|ICANON|ISIG);
	newtio.c_cflag &= ~CBAUD;
	newtio.c_cflag |= BOTHER|CRTSCTS;
	newtio.c_ospeed = 921600;
	newtio.c_ispeed = 921600;
	ret = ioctl(fd, TCSETS2, &newtio);
//	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);
	ret = ioctl(fd, TCGETS2, &newtio);
	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);

#define DATALEN  1000
	
	widx = 0;
	ridx = 0;

	total_tx = 0;
	total_rx = 0;
	clnlen = 0;
	
	do{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		retval = select( fd+1 , &rfds, 0, 0, &tv);
		if(retval == 0){
			printf("%d bytes of data cleaned\n", clnlen);
			break;
		}
		rlen = read(fd, tmpbuf, sizeof(tmpbuf));
		if(rlen <= 0){
			printf("error durring clean\n");
			exit(0);
		}
		clnlen += rlen;
	}while(1);

	rd_size = DATALEN;
	rd_size = DATALEN/100;
	if(rd_size <= 0){
		rd_size = 1;
	}

	
	do{
		wlen = 0;
		rlen = 0;
		FD_ZERO(&wfds);
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		
		FD_SET(fd, &wfds);

		tv.tv_sec = 30;
		tv.tv_usec = 0;
		retval = select( fd+1 , &rfds, &wfds, 0, &tv);
		if(retval == 0){
			printf("select nothing\n");
			break;
		}

		if(FD_ISSET(fd, &wfds)){
			wlen = write(fd, &buf[widx], DATALEN - widx);
			if(wlen <= 0 ){
				printf("write error\n");
				break;
			}
			total_tx += wlen;
			widx = (widx + wlen) % ('}' - '!' + 1);
			
		}
		if(FD_ISSET(fd, &rfds)){
//			memset(tmpbuf, 0, sizeof(tmpbuf));
			rlen = read(fd, tmpbuf, rd_size);
			if(rlen <= 0){
				printf("failed to read()\n");
				break;
			}
			diff = memcmp(tmpbuf, &buf[ridx], rlen);
			if(diff){
				
				printf("\r\nrlen = %d\r\n", rlen);
				for(i = 0; i < rlen; i++){
					if(i != 0 && (i % 16 == 0)){
						printf("\n");
					}
					printf("%x", tmpbuf[i]);
					if(tmpbuf[i] != buf[ridx + i]){
						printf("!");
					}else{
						printf(" ");
					}
				}
				printf("\n");
				break;
			}
			total_rx += rlen;
			ridx = (rlen + ridx) % ('}' - '!' + 1);
		}

		printf("tx: %d rx: %d tx_idx = %d rx_idx = %d wlen = %d rlen = %d.      \r",
			total_tx, total_rx, widx, ridx, wlen, rlen);
		fflush(stdout);

	//	sleep(10);
	}while(1);

	printf("test end\n");

	if(fd > 0){
		printf("ready to close\n");
		close(fd);
	}

	printf("closed\n");


	return 0;
}

