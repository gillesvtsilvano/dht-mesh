#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(){
	int fd, k=1;
	char inbuffer[100];
	fd = open("/dev/nbt", O_RDONLY);

	while(k>0){
		k = read(fd,inbuffer,14);
		if (k<0){ perror(" read "); break;}
		inbuffer[k]=0;
		printf(" read %2d : %s \n", k, inbuffer);
	}
}