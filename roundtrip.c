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
#define SELTIME 1000

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

	if(argc != 4){
		printf("%s [tty_path] [buad_rate] [wlen <= 1000]\n", argv[0]);
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
	int baud_rate = atoi(argv[2]);
	printf("baud_rate = %d\n", baud_rate);
	int m_wlen = atoi(argv[3]);
	printf("write_len = %d\n", m_wlen);
	
	fd = open(argv[1], O_RDWR);
	printf("fd = %d\n", fd);


	ret = ioctl(fd, TCGETS2, &newtio);
	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);
	newtio.c_iflag &= ~(ISTRIP|IUCLC|IGNCR|ICRNL|INLCR|ICANON|IXON|PARMRK);
	newtio.c_iflag |= (IGNBRK|IGNPAR);
	newtio.c_lflag &= ~(ECHO|ICANON|ISIG);
	newtio.c_cflag &= ~CBAUD;
	newtio.c_cflag |= BOTHER;
	newtio.c_ospeed = baud_rate;
	newtio.c_ispeed = baud_rate;
	ret = ioctl(fd, TCSETS2, &newtio);
	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);
	ret = ioctl(fd, TCGETS2, &newtio);
	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);
	
	 _tty_flush(fd);

	wlen = 0;
	int w_index = 0;
	int max_rt = 0;
	int max_rtindex = 0;

	int a_rt[1024];
	int r_cnt = 0;
	int rt_average = 0;

	int a_rt_cnt = (sizeof(a_rt)/sizeof(a_rt[0]));

	do{
		FD_ZERO(&wfds);
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		printf("\r");
		printf("[%d] round_trip max %d max_index %d average %d", w_index, max_rt, max_rtindex, rt_average);
		fflush(stdout);
		
		if(wlen == 0 ){
			FD_SET(fd, &wfds);
		}


		tv.tv_sec = (SELTIME)/1000;
		tv.tv_usec = (SELTIME)%1000;
		retval = select( fd+1 , &rfds, &wfds, 0, &tv);
		if(retval == 0){
			printf("select nothing\n");
			wlen = 0;
			break;
		}

		if(FD_ISSET(fd, &wfds)){
			wlen = write(fd, buf, m_wlen);
			w_index++;
			//printf("write len = %d\n", wlen);
		}
		if(FD_ISSET(fd, &rfds)){
			int left_tv;
			int roundtrip_tv;
			int a_index;
			rlen = read(fd, tmpbuf, DATALEN);
			a_index = r_cnt % a_rt_cnt;
			r_cnt++;
			diff = memcmp(tmpbuf, buf, rlen);
			
			wlen = 0;
			if(diff || rlen != m_wlen){
				printf("relen = %d diff = %d \n", rlen, diff);
				for(i = 0; i < rlen; i++){
					printf("%x ", tmpbuf[i]);
				}
				printf("\n");
				break;
			}
			
			left_tv = (tv.tv_sec * 1000) + (tv.tv_usec/1000);
			roundtrip_tv = (SELTIME) - left_tv; // select time is 1000 msecs:
			a_rt[a_index] = roundtrip_tv;
			
			if(r_cnt >= a_rt_cnt){
				int i;
				int sum_rt;
				sum_rt = 0;
				for(i = 0; i < a_rt_cnt; i++){
					sum_rt += a_rt[i];
				}
				rt_average = sum_rt/a_rt_cnt;
			}else{
				int i;
				int sum_rt;
				sum_rt = 0;
				for(i = 0; i < r_cnt; i++){
					sum_rt += a_rt[i];
				}
				rt_average = sum_rt/r_cnt;
			}
			if(roundtrip_tv >= max_rt){
				max_rt = roundtrip_tv;
				max_rtindex = w_index;
			}
		}

		//sleep(2);
	}while(1);
	if(fd > 0){
		close(fd);
	}


	return 0;
}

