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
#include <asm-generic/termbits.h>
#include <pthread.h>

#define clear() printf("\033[H\033[J")
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))

char buf [1024];
int baud;

struct __data{
	int total_tx;
	int total_rx;
	int Bps;
	int failed;
	char * dev;
	char msg[2048];
};

void * stress(void * data)
{
	char tmpbuf[1024];
	struct __data * priv = data;
	char * dev = priv->dev;
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
	int widx;
	int ridx;
	int clnlen;
	int bps_cnt;
	int bps_ptr;

	bps_ptr = 0;
	bps_cnt = 0;
#define def_bps_pool_size 1024
	struct bps_para{
		int rlen;
		int time;
	}bps_pool[def_bps_pool_size];
	
	fd = open(dev, O_RDWR);
	if(fd < 0){
		snprintf(priv->msg, sizeof(priv->msg), "cannot open %s fd = %d\n", dev, fd);
		return 0;
	}


	ioctl(fd, TCGETS2, &newtio);
	newtio.c_iflag &= ~(ISTRIP|IUCLC|IGNCR|ICRNL|INLCR|ICANON|IXON|PARMRK);
	newtio.c_iflag |= (IGNBRK|IGNPAR);
	newtio.c_lflag &= ~(ECHO|ICANON|ISIG);
	newtio.c_cflag &= ~CBAUD;
	newtio.c_cflag |= BOTHER|CRTSCTS;
	newtio.c_ospeed = baud;
	newtio.c_ispeed = baud;
	ioctl(fd, TCSETS2, &newtio);
	ioctl(fd, TCGETS2, &newtio);
#define CLEAN_DATA_TIME 5
#define DATALEN  1000
	
	widx = 0;
	ridx = 0;

	clnlen = 0;
	
	do{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec = CLEAN_DATA_TIME;
		tv.tv_usec = 0;
		retval = select( fd+1 , &rfds, 0, 0, &tv);
		if(retval == 0){
			break;
		}
		rlen = read(fd, tmpbuf, DATALEN);
		if(rlen <= 0){
			snprintf(priv->msg, sizeof(priv->msg), "error durring clean\n");
			
			return 0;
		}
		clnlen += rlen;
	}while(1);

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
			continue;
		}

		if(FD_ISSET(fd, &wfds)){
			wlen = write(fd, &buf[widx], DATALEN - widx);
			if(wlen <= 0 ){
				snprintf(priv->msg, sizeof(priv->msg),"write error\n");
				
				priv->failed = 1;
				break;
			}
			priv->total_tx += wlen;
			widx = (widx + wlen) % ('}' - '!' + 1);
			
		}
		if(FD_ISSET(fd, &rfds)){
			int timepass;
			int total_rlen;
			rlen = read(fd, tmpbuf, DATALEN - ridx);
			if(rlen <= 0){
				snprintf(priv->msg, sizeof(priv->msg),"failed to read()\n");
				priv->failed = 1;
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
			priv->Bps = (total_rlen *1000)/timepass;

			
			
			diff = memcmp(tmpbuf, &buf[ridx], rlen);
			if(diff){
				snprintf(priv->msg, sizeof(priv->msg),"data error");
	/*			printf("\r\nrlen = %d\r\n", rlen);
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
				priv->failed = 1;
				break;
			}
			priv->total_rx += rlen;
			ridx = (rlen + ridx) % ('}' - '!' + 1);
		}

	}while(1);

	if(fd > 0){
		close(fd);
	}

	return 0;

}


int main(int argc, char **argv)
{
	int i;
	char running[] = {'-','\\','|', '/'};
	char *running_index;
	int peer_cnt = argc - 2;
	struct __data *data;
	baud = atoi(argv[argc - 1]);

	printf("peer_cnt = %d\n", peer_cnt);
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

	data = malloc(sizeof(struct __data) * peer_cnt);
	running_index = malloc(peer_cnt);
	printf("testing with baud %d\n", baud);
	
	for(i = 0; i < peer_cnt; i++)
	{
		pthread_t _p;
		data[i].dev = argv[i + 1];
		printf("%d %s\n", i, data[i].dev);
		data[i].failed = 0;
		data[i].Bps = 0;
		data[i].total_tx = 0;
		data[i].total_rx = 0;
		data[i].msg[0] = '\n';
		running_index[i] = 0;

		pthread_create(&_p, 0, stress, &data[i]);
	}
	sleep(5);
	clear();
	do{
		for(i = 0; i < peer_cnt; i++){
			gotoxy(1, i + 1);
			printf("%d tx:%d rx:%d bps:%d %c %s\n",
				i, 
				data[i].total_tx, 
				data[i].total_rx,
				data[i].Bps,
				data[i].failed?'x':running[(int)running_index[i]],
				data[i].failed?data[i].msg:"  ");
				running_index[i]++;
				running_index[i] %= sizeof(running);
		}
		sleep(1);
	}while(1);
}


