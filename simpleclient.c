#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#define BUFSIZE 8196

struct{
pthread_mutex_t st_mutex;
unsigned int st_concount;
unsigned int st_contotal;
unsigned long st_contime;
}stats;

void client()
{
	time_t start;
	start=time(0);
	int sockfd;
 	int len;

	
 	struct sockaddr_in address;
 	int result;
 
	int cc;
	



 	sockfd = socket(AF_INET, SOCK_STREAM, 0);
 	address.sin_family = AF_INET;
 	address.sin_addr.s_addr = inet_addr("127.0.0.1");
 	address.sin_port = htons(10000);
 	len = sizeof(address);
	result = connect(sockfd, (struct sockaddr *)&address, len);

 	if (result == -1)
 	{
  	perror("oops: client1");
  	pthread_exit((void *)-1);
 	}

	
	
	
	char ch[BUFSIZE] = "HEAD /index.html HTTP/1.1\n\r\n";
	write(sockfd, ch, strlen(ch));
	int j;	
	//printf("\nBefore read:%s", ch);
memset(ch, 0, sizeof(ch));
	
	while( (j=read(sockfd,ch,BUFSIZE)) > 0)
		write(1,ch,j);
		
 
 	close(sockfd);

	
}

int main(int argc, char *argv[])
{
time_t start;
start=time(0);
pthread_t th[10];
pthread_attr_t ta;
int i;

(void)pthread_attr_init(&ta);
(void)pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_JOINABLE);
(void)pthread_mutex_init(&stats.st_mutex, 0);


 for(i=1;i<2;i++)
{
 //printf("\nCreating threads!");
 pthread_create(&th[i], &ta, (void *(*)(void *))client, (void *)i);
}
printf("\nWaiting for all threads to complete!");

 for (i= 1; i < 2; i++) 
 {
	void * currentThread;
	if (pthread_join(th[i], &currentThread) != 0) 
	{
	} else {
	printf("main()=>: completed thread[%d]!\n",
	(int)currentThread);
	}
 }
client();
 


 exit(0);
}
