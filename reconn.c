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
	struct termios2 newtio;
	fd_set wfds;
	fd_set rfds;
	struct timeval tv;
	int retval;
	int wlen;
	int rlen;
	int diff;
	int total_rx;
	int mxrt;
	int rt;
	int clr, wr, pass, fail, wto, rto,  erlen;
		

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
	if(fd < 0){
		printf("fd = %d\n", fd);
		return 0;
	}

	ioctl(fd, TCGETS2, &newtio);
	newtio.c_iflag &= ~(ISTRIP|IUCLC|IGNCR|ICRNL|INLCR|ICANON|IXON|PARMRK);
	newtio.c_iflag |= (IGNBRK|IGNPAR);
	newtio.c_lflag &= ~(ECHO|ICANON|ISIG);
	newtio.c_cflag &= ~CBAUD;
	newtio.c_cflag |= BOTHER;
	newtio.c_ospeed = 50000;
	newtio.c_ispeed = 50000;
	ioctl(fd, TCSETS2, &newtio);

	printf("serial check & clean\n");
	printf("cleaned %d bytes of data\n",_tty_flush(fd));

	
	wlen = 0;
	clr = 0;
	wr = 0;
	pass = 0; 
	fail = 0;
	wto = 0;
	rto = 0;
	erlen = 0;
	total_rx = 0;
	mxrt = 0;
	rt = 0;

	do{
		printf("mxrt %d wr: %d pass: %d fail: %d wto: %d rto %d erl %d clr %d.      \r",
			mxrt, wr, pass, fail, wto, rto, erlen, clr);
		fflush(stdout);

		FD_ZERO(&wfds);
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		
		if(wlen == 0 ){
			FD_SET(fd, &wfds);
			total_rx = 0;
		}

#define SELTIME	3
		tv.tv_sec = SELTIME;
		tv.tv_usec = 0;
		retval = select( fd+1 , &rfds, &wfds, 0, &tv);
		if(retval == 0){
			if(wlen == 0){
				wto++;
			}else{
				rto++;
			}
			rt += SELTIME;
			wlen = 0;
			continue;
		}
		
		if(FD_ISSET(fd, &wfds)){
			wlen = write(fd, buf, DATALEN);
			wr++;
			if(wlen <= 0){
				printf("write error\n");
				return 0;
			}
		}
		if(FD_ISSET(fd, &rfds)){
			rlen = read(fd, &tmpbuf[total_rx], DATALEN - total_rx);
			if(rlen <= 0){
				printf("read error\n");
				return 0;
			}

			mxrt = (rt > mxrt)?rt:mxrt;
			rt = 0;

			wlen -= rlen;
			if(wlen < 0){
				erlen++;
				wlen = 0;
				clr += _tty_flush(fd);
				continue;
			}else if(wlen == 0){
				diff = memcmp(tmpbuf, buf, rlen);
				if(diff){
					fail++;
					clr += _tty_flush(fd);
					continue;
				}
				pass++;
			}
			total_rx += rlen;
			if(total_rx > DATALEN){
				erlen++;
				clr += _tty_flush(fd);
				wlen = 0;
				continue;
			}
		}

	}while(1);
	if(fd > 0){
		close(fd);
	}


	return 0;
}

