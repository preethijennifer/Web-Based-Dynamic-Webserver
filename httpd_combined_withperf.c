/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 *  1) Comment out the #include <pthread.h> line.
 *  2) Comment out the line that defines the variable newthread.
 *  3) Comment out the two lines that run pthread_create().
 *  4) Uncomment the line that runs accept_request().
 *  5) Remove -lsocket from the Makefile.
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include<assert.h>
#include<sys/time.h>

#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

struct{
unsigned int st_contime;
unsigned int st_concount;
unsigned int st_contotal;
pthread_mutex_t st_mutex;
}stats;

char * pch, *pch1;

time_t start;	

void accept_request(int);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
//int startup(u_short *);
void unimplemented(int);
/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(int client)
{
//printf("\nScript started at %s", ctime(&start));

 /*time_t rawtime;
struct tm * timeinfo;

	time ( &rawtime );
timeinfo = localtime ( &rawtime );

int hour = timeinfo->tm_hour;
	  int min = timeinfo->tm_min;
int sec=timeinfo->tm_sec;*/

//printf("\nHour is %d and Minute is %d and second is %d", hour, min, sec);
	(void)pthread_mutex_lock(&stats.st_mutex);
	stats.st_concount++;
	//printf("\nConnection %d\n", stats.st_concount);
	(void)pthread_mutex_unlock(&stats.st_mutex);

 char buf[1024];
 int numchars;
 char method[255];
 char url[255];
 char path[512];
 size_t i, j;
 struct stat st;
 int cgi = 0;      /* becomes true if server decides this is a CGI
                    * program */
 char *query_string = NULL;

 numchars = get_line(client, buf, sizeof(buf));
//printf("\nBuffer:%s", buf);
 i = 0; j = 0;
 while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
 {
  method[i] = buf[j];
  i++; j++;
 }
 method[i] = '\0';

 if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
 {
  unimplemented(client);
  return;
 }

 if (strcasecmp(method, "POST") == 0)
  cgi = 1;

 i = 0;
 while (ISspace(buf[j]) && (j < sizeof(buf)))
  j++;
 while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
 {
  url[i] = buf[j];
  i++; j++;
 }
 url[i] = '\0';
//printf("\nURL:%s", url);
 if (strcasecmp(method, "GET") == 0)
 {
  query_string = url;
  while ((*query_string != '?') && (*query_string != '\0'))
   query_string++;
  if (*query_string == '?')
  {
   cgi = 1;
   *query_string = '\0';
   query_string++;
  }
 }

 sprintf(path, "htdocs%s", url);
 if (path[strlen(path) - 1] == '/')
  strcat(path, "index.html");
 if (stat(path, &st) == -1) {
  while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
   numchars = get_line(client, buf, sizeof(buf));
  not_found(client);
 }
 else
 {
  if ((st.st_mode & S_IFMT) == S_IFDIR)
   strcat(path, "/index.html");
  if ((st.st_mode & S_IXUSR) ||
      (st.st_mode & S_IXGRP) ||
      (st.st_mode & S_IXOTH)    )
   cgi = 1;
  if (!cgi)
   serve_file(client, path);
  else
   execute_cgi(client, path, method, query_string);
 }
//printf("\nClosing client fd:%d", client);
//printf("\nHere!");

	(void)pthread_mutex_lock(&stats.st_mutex);
	stats.st_contotal++;
	stats.st_concount--;
        //printf("\nThe time now is %f", time(0));
	stats.st_contime+=time(0)-start;
	printf("\nConnection time:%f\n", stats.st_contime);
	(void)pthread_mutex_unlock(&stats.st_mutex);

 close(client);
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "Content-type: text/html\r\n");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "<P>Your browser sent a bad request, ");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "such as a POST without a Content-Length.\r\n");
 send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, FILE *resource)
{
 char buf[10240];
//printf("\nInside cat:%d and fetching contents of %s",client, resource);

 fgets(buf, sizeof(buf), resource);
 while (!feof(resource))
 {

  send(client, buf, strlen(buf), 0);
  fgets(buf, sizeof(buf), resource);
 }
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
 send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
 perror(sc);
 exit(1);
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/


void execute_cgi(int client, const char *path,
                 const char *method, const char *query_string)
{
 char buf[1024];
 int cgi_output[2];
 int cgi_input[2];
 pid_t pid;
 int status;
 int i;
 //char c;
 int numchars = 1;
 int content_length = -1;
 char buffer[1024];
 int ret;
 char content_type[255];
//printf("\nPath:%s, method:%s, query:%s", path, method, query_string);
 buf[0] = 'A'; buf[1] = '\0';
 if (strcasecmp(method, "GET") == 0)
  while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
   numchars = get_line(client, buf, sizeof(buf));
 else    /* POST */
 {
  numchars = get_line(client, buf, sizeof(buf));
  while ((numchars > 0) && strcmp("\n", buf))
  {
   //printf("[raw: %s]\n", buf);
   if (strncasecmp(buf, "Content-Length:", 15) == 0) {
    content_length = atoi(&(buf[16]));
    //printf("Content-Length: %d\n", content_length);
  }
   if (strncasecmp(buf, "Content-Type:", 13) == 0) {
     char *p = &buf[14];
     for(i=0; i<numchars && p[i] != ';'; i++) {
	content_type[i] = p[i];
     }
	content_type[i] = 0;
	//printf("Content-Type:%s\n", content_type);
   	
  }
   numchars = get_line(client, buf, sizeof(buf));
  }
  
  if (content_length == -1) {
   bad_request(client);
   return;
 }
}

 sprintf(buf, "HTTP/1.0 200 OK\r\n");
 send(client, buf, strlen(buf), 0);

 if (pipe(cgi_output) < 0) {
  cannot_execute(client);
  return;
 }
 if (pipe(cgi_input) < 0) {
  cannot_execute(client);
  return;
 }

 if ( (pid = fork()) < 0 ) {
  cannot_execute(client);
  return;
 }
 if (pid == 0)  /* child: CGI script */
 {
  char meth_env[255];
  char query_env[255];
  char length_env[255];
  char content_env[255];
  

  dup2(cgi_output[1], 1);
  dup2(cgi_input[0], 0);
  close(cgi_output[0]);
  close(cgi_input[1]);
  sprintf(meth_env, "REQUEST_METHOD=%s", method);
  putenv(meth_env);
  if (strcasecmp(method, "GET") == 0) {
   sprintf(query_env, "QUERY_STRING=%s", query_string);
   putenv(query_env);
  }
  else {   /* POST */
   sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
   putenv(length_env);
   sprintf(content_env, "CONTENT_TYPE=%s", content_type);
   putenv(content_env);
  }
  execl(path, path, NULL);
  exit(0);
 } else {    /* parent */
  close(cgi_output[1]);
  close(cgi_input[0]);
  if (strcasecmp(method, "POST") == 0)
   //printf("Content[%d]:\n", content_length);
   for (i = 0; i < content_length;) {
    if ((ret = recv(client, buffer, sizeof(buffer), 0)) < 0) {
	perror("recv");
	break;
    }
    if (write(cgi_input[1], buffer, ret) < 0) {
	perror("write");
	break;
    }
    buffer[ret] = 0;
    //printf("%s", buffer);
    i+=ret;
   }
  //printf("Going to read\n");
  while ((ret = read(cgi_output[0], buffer, sizeof(buffer))) > 0) {
   send(client, buffer, ret, 0);
   //printf("%s", buffer);
 }

  close(cgi_output[0]);
  close(cgi_input[1]);
  waitpid(pid, &status, 0);
 }
 //printf("End CGI ...\n");
}


/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
 int i = 0;
 char c = '\0';
 int n;

 while ((i < size - 1) && (c != '\n'))
 {
  n = recv(sock, &c, 1, 0);
  /* DEBUG printf("%02X\n", c); */
  if (n > 0)
  {
   if (c == '\r')
   {
    n = recv(sock, &c, 1, MSG_PEEK);
    /* DEBUG printf("%02X\n", c); */
    if ((n > 0) && (c == '\n'))
     recv(sock, &c, 1, 0);
    else
     c = '\n';
   }
   buf[i] = c;
   i++;
  }
  else
   c = '\n';
 }
 buf[i] = '\0';
 
 return(i);
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(int client, const char *filename)
{
 char buf[1024];
 (void)filename;  /* could use filename to determine file type */
//printf("\nInside headers");
 strcpy(buf, "HTTP/1.0 200 OK\r\n");
 send(client, buf, strlen(buf), 0);
//printf("\nBuf:%s\nSize:%d \nClient socket:%d", buf,strlen(buf), client);
//printf("\nHere!");
 strcpy(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
//printf("\nBuf:%s \nClient socket:%d", buf, client);
//printf("\npch1 is %s", pch1);
if(strcmp(pch1, "jpg")==0)
 sprintf(buf, "Content-Type: image/jpg\r\n");
else
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
//printf("\nBuf:%s \nClient socket:%d", buf, client);
 strcpy(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
//printf("\nBuf:%s \nClient socket:%d", buf, client);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "your request because the resource specified\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "is unavailable or nonexistent.\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</BODY></HTML>\r\n");
 send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
//printf("\nINside serve_FILE:%s", filename);
char filename1[100];
strcpy(filename1, filename);

pch = strtok (filename1,".");
  while (pch != NULL)
  {
    //printf ("\nFilename and Extension: %s\n",pch);
    pch1=pch;
    pch = strtok (NULL, " ,.-");
  }	
 FILE *resource = NULL;
 int numchars = 1;
 char buf[1024];

 buf[0] = 'A'; buf[1] = '\0';
 while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
  numchars = get_line(client, buf, sizeof(buf));
//printf("\nOpening file");
 resource = fopen(filename, "r");
 if (resource == NULL)
{
  not_found(client);
}
 else
 {
//printf("\nFetching contents");
  headers(client, filename);
//printf("\nCat contents of index.html");
  cat(client, resource);
 }
 fclose(resource);
}

void unimplemented(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</TITLE></HEAD>\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</BODY></HTML>\r\n");
 send(client, buf, strlen(buf), 0);
}

/**********************************************************************/

void prstats(void)
{
	time_t now;
	while(1)
	{
		(void)sleep(5);
		
		(void)pthread_mutex_lock(&stats.st_mutex);
		
		(void)printf("---%s", ctime(&now));
		(void)printf("%-32s:%u\n","Current Connections",stats.st_concount);
		(void)printf("Completed Connections:%u\n",stats.st_contotal);
		stats.st_contime+=time(0)-start;
		(void)printf("\nConnectioon time:%f", stats.st_contime);
		(void)pthread_mutex_unlock(&stats.st_mutex);
	}
}


int main(void)
{

//(void) time(&start);
start=time(0);
 int serv_sock = -1;
 pid_t pid;

	pthread_t th;
	pthread_attr_t ta;

	(void)pthread_attr_init(&ta);
	(void)pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
	(void)pthread_mutex_init(&stats.st_mutex, 0);

	if(pthread_create(&th, &ta, (void *(*)(void *))prstats, 0)<0)
		printf("\nPthread create error!");
		


int max_fd, max_index;
int client[FD_SETSIZE];
fd_set ready_set, read_set;
int nready, i, j, k;

 //u_short serv_port = 0;
 int client_sock = -1;
 struct sockaddr_in serv_addr;
 socklen_t client_name_len = sizeof(serv_addr);
 pthread_t newthread;

serv_addr.sin_family=AF_INET;
serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
serv_addr.sin_port=htons(10000);

serv_sock=socket(PF_INET, SOCK_STREAM, IPPROTO_IP);

printf("\nServer Socket created!:%d", serv_sock);

if(bind(serv_sock, (struct sockaddr *)&serv_addr, client_name_len)<0)
	printf("\nBind Error!");

if(listen(serv_sock, 100)<0)
	printf("\nListen Error!");

printf("\nSelect the server model:\n 1. Multithreaded server\n 2. Multiprocess server\n 3. Asynchronously multiplexed server\n");
int choice=0;
scanf("%d", &choice);
printf("\nChoice is %d", choice);
switch(choice)
	{
	case 1:
		printf("\nYou entered multithreaded server!");
		while (1)
 		{
  		client_sock = accept(serv_sock,
                       (struct sockaddr *)NULL, NULL);
  		if (client_sock == -1)
   		error_die("accept");
 		/* accept_request(client_sock); */
 		if (pthread_create(&newthread , NULL, (void *(*)(void *))accept_request, (void *)client_sock) != 0)
   		perror("pthread_create");
		}
break;

	case 2:
		printf("\nYou entered multiprocess server!");
		while (1)
 		{
  		client_sock = accept(serv_sock,
                       (struct sockaddr *)NULL,NULL);
		//printf("\nHere!");
  		if (client_sock == -1)
   			error_die("accept");

		pid = fork();
	
		if(pid == 0)
		{
			//close(server_sock);
			//printf("fork successful\n");
			//close(serv_sock);
			//printf("\nInside child process!");
			accept_request(client_sock);
			
			
	 	}

		else if(pid > 0)
		{
			//printf("\nInside Parent process!");
			close(client_sock);
			//break;
		}
		
		}
		break;
	case 3:
	printf("\nYou entered asynchronously multiplexed server!!!");
	max_fd=serv_sock;
	max_index=-1;

	for(i=0;i<FD_SETSIZE;i++)
		client[i]=-1;
	FD_ZERO(&read_set);
	FD_SET(serv_sock, &read_set);


 

 	while (1)
 	{
		ready_set=read_set;
		nready=select(max_fd+1, &ready_set, NULL, NULL, NULL);
		if(FD_ISSET(serv_sock, &ready_set))
		{
		
		
  		client_sock = accept(serv_sock,
                       (struct sockaddr *)NULL, NULL);
  		if (client_sock == -1)
  		  error_die("accept");
		if(client_sock)
			{
			//printf("\nA valid Client connection with client FD:%d", client_sock);
			for(j=0;j<FD_SETSIZE;j++)
				if(client[j]<0)
				{
					//printf("\nInhere, client[j]:%d",client[j]);
					client[j]=client_sock;
					//printf("\nAfter, client[j]:%d",client[j]);
					break;
				}
			FD_SET(client_sock, &ready_set);
			if(client_sock>max_fd)
				max_fd=client_sock;
			if(j>max_index)
				max_index=j;
			}
		}
		for(k=0;k<=max_index;k++)
		{
			//printf("\n12345678");
			if(client[k]>0)
			{
			accept_request(client[k]);
			FD_CLR(client[k], &ready_set);
			client[k]=-1;
			}
		}
		
 
	 }
	break;

 	}
	


 
 

 close(serv_sock);

 return(0);
}
