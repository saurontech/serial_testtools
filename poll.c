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


char buf [1024];
char tmpbuf[1024];


#define DATALEN  1000

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

	if(argc != 2){
		printf("%s [tty_path]\n", argv[0]);
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
	
	fd = open(argv[1], O_RDWR);
	printf("fd = %d\n", fd);


	ret = ioctl(fd, TCGETS2, &newtio);
	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);
	newtio.c_iflag &= ~(ISTRIP|IUCLC|IGNCR|ICRNL|INLCR|ICANON|IXON|PARMRK);
	newtio.c_iflag |= (IGNBRK|IGNPAR);
	newtio.c_lflag &= ~(ECHO|ICANON|ISIG);
	newtio.c_cflag &= ~CBAUD;
	newtio.c_cflag |= BOTHER;
	newtio.c_ospeed = 50000;
	newtio.c_ispeed = 50000;
	ret = ioctl(fd, TCSETS2, &newtio);
	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);
	ret = ioctl(fd, TCGETS2, &newtio);
	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);
	
	 _tty_flush(fd);

	wlen = 0;

	do{
		FD_ZERO(&wfds);
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		
		if(wlen == 0 ){
			FD_SET(fd, &wfds);
		}


		tv.tv_sec = 3;
		tv.tv_usec = 0;
		retval = select( fd+1 , &rfds, &wfds, 0, &tv);
		if(retval == 0){
			printf("select nothing\n");
			break;
		}

		if(FD_ISSET(fd, &wfds)){
			wlen = write(fd, buf, DATALEN);
			printf("write len = %d\n", wlen);
		}
		if(FD_ISSET(fd, &rfds)){
			rlen = read(fd, tmpbuf, DATALEN);
			diff = memcmp(tmpbuf, buf, rlen);
			printf("relen = %d diff = %d \n", rlen, diff);
			wlen = 0;
			if(diff){
			for(i = 0; i < rlen; i++){
				printf("%x ", tmpbuf[i]);
			}
			printf("\n");
			}
		}

		sleep(2);
	}while(1);
	if(fd > 0){
		close(fd);
	}


	return 0;
}

