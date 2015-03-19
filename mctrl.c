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


int main(int argc, char **argv)
{
	int fd;
	int ret;
	int count;
	int opt;
	struct termios2 newtio;
	unsigned int input, output;
	count = 0;
	
	if(argc != 2){
		printf("%s [tty]\n", argv[0]);
		return 0;
	}

	fd = open(argv[1], O_RDWR);
	if(fd < 0){
		printf("failed to open fd\n");
		return 0;
	}


	ret = ioctl(fd, TCGETS2, &newtio);
	newtio.c_iflag &= ~(ISTRIP|IUCLC|IGNCR|ICRNL|INLCR|ICANON|IXON|PARMRK);
	newtio.c_iflag |= (IGNBRK|IGNPAR);
	newtio.c_lflag &= ~(ECHO|ICANON|ISIG);
	newtio.c_cflag &= ~CBAUD;
	newtio.c_cflag |= BOTHER;//|CRTSCTS;
	newtio.c_cflag &= (~CRTSCTS);
	newtio.c_ospeed = 921600;
	newtio.c_ispeed = 921600;
	ret = ioctl(fd, TCSETS2, &newtio);
	ret = ioctl(fd, TCGETS2, &newtio);


	count = 100;

	do{
		output = 0;
		opt = (count%4);

		switch(opt){
		case 0:
		default:
			break;
		case 1:
			output |= TIOCM_DTR;
			break;
		case 2:
			output |= TIOCM_RTS;
			break;
		case 3:
			output |= (TIOCM_DTR|TIOCM_RTS);
			break;
		}
/*		opt = (count % 2);
		switch(opt){
			case 0:
			default:
				output |= TIOCM_DTR;
				break;
			case 1:
				break;
		}*/
	
		ret = ioctl(fd, TIOCMSET, &output);
		if(ret){
			printf("failed to set\n");
			return 0;
		}
		sleep(1);
		ret = ioctl(fd, TIOCMGET, &input);
		if(ret){
			printf("failed to get\n");
			return 0;
		}

		printf("\rDTR");
		if(output & TIOCM_DTR){
			printf("*");
		}else{
			printf("_");
		}
		
		printf("RTS");
		if(output & TIOCM_RTS){
			printf("*");
		}else{
			printf("_");
		}

		printf("CTS");
		if(input & TIOCM_CTS){
			printf("*");
		}else{
			printf("_");
		}
		printf("DSR");
		if(input & TIOCM_DSR){
			printf("*");
		}else{
			printf("_");
		}
		
		printf("CAR");
		if(input & TIOCM_CAR){
			printf("*");
		}else{
			printf("_");
		}
		printf("RI");
		if(input & TIOCM_RI){
			printf("*");
		}else{
			printf("_");
		}
		printf("...   ");

		fflush(stdout);

	}while(--count > 0);



	close(fd);

	return 0;
}

