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
	unsigned int total_tx;
	unsigned int total_rx;
	int clnlen;
	int baud;
	int stop_tx;
	char * dev;
	//char *sel_str;
	int Bps;
	int bps_cnt;
	int bps_ptr;

	Bps = 0;
	bps_ptr = 0;
	bps_cnt = 0;
#define def_bps_pool_size 1024
	struct bps_para{
		int rlen;
		int time;
	}bps_pool[def_bps_pool_size];

	if(argc != 3){
		printf("%s [tty_path] [baud]\n", argv[0]);
		return 0;
	}

	dev = argv[1];
	baud = atoi(argv[2]);

	if(baud < 50 || baud > 921600){
		printf("baud rate should be between 50 and 921600\n");
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
	
	fd = open(dev, O_RDWR);
	if(fd < 0){
		printf("cannot open %s fd = %d\n", dev, fd);
		return 0;
	}


	ret = ioctl(fd, TCGETS2, &newtio);
	newtio.c_iflag &= ~(ISTRIP|IUCLC|IGNCR|ICRNL|INLCR|ICANON|IXON|PARMRK);
	newtio.c_iflag |= (IGNBRK|IGNPAR);
	newtio.c_lflag &= ~(ECHO|ICANON|ISIG);
	newtio.c_cflag &= ~CBAUD;
	newtio.c_cflag |= BOTHER|CRTSCTS;
	newtio.c_ospeed = baud;
	newtio.c_ispeed = baud;
	ret = ioctl(fd, TCSETS2, &newtio);
//	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);
	ret = ioctl(fd, TCGETS2, &newtio);
	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);
#define CLEAN_DATA_TIME 10
	printf("cleaning rx data for %d secs\n", CLEAN_DATA_TIME);

#define DATALEN  1000
	
	widx = 0;
	ridx = 0;

	total_tx = 0;
	total_rx = 0;
	clnlen = 0;
	
	do{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec = CLEAN_DATA_TIME;
		tv.tv_usec = 0;
		retval = select( fd+1 , &rfds, 0, 0, &tv);
		if(retval == 0){
			printf("%d bytes of data cleaned\n", clnlen);
			break;
		}
		rlen = read(fd, tmpbuf, DATALEN);
		if(rlen <= 0){
			printf("error durring clean\n");
			exit(0);
		}
		clnlen += rlen;
	}while(1);

//	close(fd);

//	fd = open(dev, O_RDWR);
//	printf("fd = %d\n", fd);


/*	ret = ioctl(fd, TCGETS2, &newtio);
//	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);
	newtio.c_iflag &= ~(ISTRIP|IUCLC|IGNCR|ICRNL|INLCR|ICANON|IXON|PARMRK);
	newtio.c_iflag |= (IGNBRK|IGNPAR);
	newtio.c_lflag &= ~(ECHO|ICANON|ISIG);
	newtio.c_cflag &= ~CBAUD;
	newtio.c_cflag |= BOTHER|CRTSCTS;
	newtio.c_ospeed = baud;
	newtio.c_ispeed = baud;
	ret = ioctl(fd, TCSETS2, &newtio);
	printf("ospeed %d ispeed %d ret = %d\n", newtio.c_ospeed, newtio.c_ispeed, ret);*/

	//sel_str = "select nothing";
	stop_tx = 0;
	printf("press any key to stop tx\n");

	do{
		wlen = 0;
		rlen = 0;
		FD_ZERO(&wfds);
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		FD_SET(STDIN_FILENO, &rfds);
		if(stop_tx == 0){
			//printf("\nselect write\n");	
			FD_SET(fd, &wfds);
		}

		tv.tv_sec = 30;
		tv.tv_usec = 0;
		retval = select( fd+1 , &rfds, &wfds, 0, &tv);
		if(retval == 0){
			//printf("\n%s\n", sel_str);
			//break;
			continue;
		}

		if(FD_ISSET(STDIN_FILENO, &rfds)){
			char tmp[8];
			read(STDIN_FILENO, tmp, 8);
			//sel_str = "test ended";
			stop_tx = !stop_tx;
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
			int timepass;
			int total_rlen;
			rlen = read(fd, tmpbuf, DATALEN - ridx);
			if(rlen <= 0){
				printf("failed to read()\n");
				break;
			}

			if(bps_cnt < def_bps_pool_size){
				bps_cnt++;
			}
			bps_pool[bps_ptr].time = 30000 - (tv.tv_sec * 1000) - (tv.tv_usec /1000);
			bps_pool[bps_ptr].rlen = rlen;
			++bps_ptr;
			bps_ptr%=def_bps_pool_size;

			timepass = total_rlen = 0;
			for(i = 0; i < bps_cnt; i++){
				total_rlen += bps_pool[i].rlen;
				timepass += bps_pool[i].time;
			}
			Bps = (total_rlen *1000)/timepass;

			
			
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

		printf("tx:%d rx:%d (%d Bps)", total_tx, total_rx, Bps);
		//printf(	"tx_idx = %d rx_idx = %d wlen = %d rlen = %d", widx, ridx, wlen, rlen);
		printf("%s\r", (stop_tx)?"tx off":"tx on");
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

