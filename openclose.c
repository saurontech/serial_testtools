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
#include <sys/time.h>
//#include <termios.h>
#include <asm-generic/termbits.h>

#define DATALEN  1000

char buf [1024];
char tmpbuf[1024];
int err = 0;
int to = 0;
int clr = 0;

int _tty_flush(int fd)
{
	int clnlen;
	fd_set rfds;
	struct timeval tv;
	int retval;
	int rlen;

	clnlen = 0;
	
	do{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		retval = select( fd+1 , &rfds, 0, 0, &tv);
		if(retval == 0){
			break;
		}
		rlen = read(fd, tmpbuf, DATALEN);
		if(rlen <= 0){
			printf("error durring clean\n");
			exit(0);
		}
		clnlen += rlen;
	}while(1);

	return clnlen;
}

void _tty_stress(int fd, int datalen, int count)
{
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

	widx = 0;
	ridx = 0;

	total_tx = 0;
	total_rx = 0;

	do{
		FD_ZERO(&wfds);
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		if(total_tx >= datalen){
			break;
		}else{
			FD_SET(fd, &wfds);
		}

		wlen = 0;
		rlen = 0;

		tv.tv_sec = 30;
		tv.tv_usec = 0;
		retval = select( fd+1 , &rfds, &wfds, 0, &tv);
		if(retval == 0){
			to++;
			break;
		}

		if(FD_ISSET(fd, &wfds)){
			wlen = write(fd, &buf[widx], DATALEN - widx);
			if(wlen <= 0 ){
				printf("\nwrite error\n");
				break;
			}
			total_tx += wlen;
			widx = (widx + wlen) % ('}' - '!' + 1);
			
		}
		if(FD_ISSET(fd, &rfds)){
			rlen = read(fd, tmpbuf, DATALEN - ridx);
			if(rlen <= 0){
				printf("\nfailed to read()\n");
				break;
			}
			diff = memcmp(tmpbuf, &buf[ridx], rlen);
			if(diff){
				err++;
/*				printf("\r\nrlen = %d\r\n", rlen);
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
				printf("\n");*/
				break;
			}
			total_rx += rlen;
			ridx = (rlen + ridx) % ('}' - '!' + 1);
		}

		printf("fd %d count: %d err: %d clr: %d tx: %d rx: %d.      \r",
			fd, count, err, clr, total_tx, total_rx);
		fflush(stdout);

	}while(1);
}

int main(int argc, char **argv)
{
	int fd;
	int i;
	int ret;
	int count;
	int clear;
	struct termios2 newtio;
	count = 0;
	
	if(argc != 2){
		printf("%s [tty]\n", argv[0]);
		return 0;
	}

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

	while(count < 100){
		count++;

		fd = open(argv[1], O_RDWR);
		if(fd < 0){
			printf("failed to open fd\n");
			break;
		}

		ret = ioctl(fd, TCGETS2, &newtio);
		newtio.c_iflag &= ~(ISTRIP|IUCLC|IGNCR|ICRNL|INLCR|ICANON|IXON|PARMRK);
		newtio.c_iflag |= (IGNBRK|IGNPAR);
		newtio.c_lflag &= ~(ECHO|ICANON|ISIG);
		newtio.c_cflag &= ~CBAUD;
		newtio.c_cflag |= BOTHER;//|CRTSCTS;
		newtio.c_cflag &=(~CRTSCTS);
		newtio.c_ospeed = 921600;
		newtio.c_ispeed = 921600;
		ret = ioctl(fd, TCSETS2, &newtio);
		ret = ioctl(fd, TCGETS2, &newtio);

		sleep(1);
//		tcflush(fd, TCIOFLUSH);
		ret = ioctl(fd, TCFLSH, TCIOFLUSH);
		if(ret){
			printf("failed to flush buffer\n");
			return 0;
		}

		clear = _tty_flush(fd);

		if(clear > clr){
			clr = clear;
		}

		_tty_stress(fd, 400000, count);

		close(fd);
	}

	return 0;
}

