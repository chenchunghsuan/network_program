#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<signal.h>
#include<fcntl.h>
#include<ctype.h>
#include<sys/sendfile.h>
#define port "8080"
#define Backlog 10
char imageheader[] = 
"HTTP/1.1 200 Ok\r\n"
"Content-Type: image/jpeg\r\n\r\n";
void sigchld_handler(int s){
while(waitpid(-1,NULL,WNOHANG)>0);
}
void *get_in_addr(struct sockaddr *sa)
{
if(sa->sa_family-=AF_INET) return &(((struct sockaddr_in*)sa)->sin_addr);
return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int main(){
int sockfd,newfd,status,j,i;
struct addrinfo *seraddr_in,*p;
fd_set master;
fd_set read_fds; 
char remoteIP[INET6_ADDRSTRLEN];
socklen_t sin_len;
struct addrinfo hints;
struct sockaddr_storage client_addr;
int listener,fdmax,nbytes;
socklen_t addr_size;
char buffer[1024],buf[80];
struct sigaction sa;
FILE *pfile;
int len,new_jpg;
FD_ZERO(&master); 
FD_ZERO(&read_fds);
memset(&hints,0,sizeof hints);
hints.ai_family=AF_UNSPEC;
hints.ai_socktype=SOCK_STREAM;
hints.ai_flags=AI_PASSIVE;
if((status=getaddrinfo(NULL,port,&hints,&seraddr_in))!=0)
{
fprintf(stderr,"getaddrinfo error=%s\n",gai_strerror(status));
exit(1);
}
int sock,yes=1;
for(p=seraddr_in;p!=NULL;p=p->ai_next){
sock=socket(p->ai_family,p->ai_socktype,p->ai_protocol);
if(sock<0) {
fprintf(stderr,"cannot open socket\n");
continue;
}
if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1){
perror("setsockopt");
exit(1);
}
if((bind(sock,p->ai_addr,p->ai_addrlen))==-1)
{
perror("bind");
close(sock);
continue;
}
break;
}
if(p==NULL) {
perror("failed to bind");
return 2;
}
freeaddrinfo(seraddr_in);
if((listen(sock,Backlog))==-1){
perror("listen");
exit(1);
}
FD_SET(sock,&master);
fdmax=sock;
printf("server wating for connection\n");
while(1){
 read_fds = master;
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }
 for(i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { // 我們找到一個！！
        if (i == sock) {
          // handle new connections
          addr_size=sizeof client_addr;
   newfd=accept(sock,(struct sockaddr*)&client_addr,&addr_size);
	if(newfd==-1){
	perror("accept");
	} else {
            FD_SET(newfd, &master); // 新增到 master set
            if (newfd > fdmax) { // 持續追蹤最大的 fd
              fdmax = newfd;
            }
            printf("selectserver: new connection from %s on "
              "socket %d\n",
              inet_ntop(client_addr.ss_family,
                get_in_addr((struct sockaddr*)&client_addr),
                remoteIP, INET6_ADDRSTRLEN),
              newfd);
          }
}
} else {
nbytes = recv(i, buf, sizeof buf, 0);
          // 處理來自 client 的資料
          if (nbytes == 0) {
            // got error or connection closed by client
            if (nbytes == 0) {
              printf("selectserver: socket %d hung up\n", i);
            } else {
              perror("recv");
            }

            close(i); // bye!
            FD_CLR(i, &master); // 從 master set 中移除

          } else {
            // 我們從 client 收到一些資料
            for(j = 0; j <= fdmax; j++) {
              // 送給大家！
              if (FD_ISSET(j, &master)) {
                if (j != sock && j != i) {
	read(j,buf,2047);
	printf("%s\n",buf);
	if(strncmp(buf, "GET /tree.jpg", 13)){
	printf("nononono\n");
	write(j,imageheader,sizeof(imageheader)-1);
	new_jpg=open("tree.jpg",O_RDONLY);
	if(new_jpg==-1)perror("open");
	int s=sendfile(j,new_jpg,NULL,10000);
	printf("send =%d\n",s);
	close(new_jpg);                  
//if (send(j, buf, nbytes, 0) == -1) {
                    //perror("send");
                  //}
                }
              }
            }
          }
        } // END handle data from client
      } // END got new incoming connection
    } // END looping through file descriptors
  } // END for( ; ; )--and you thought it would never end!


return 0;
}



