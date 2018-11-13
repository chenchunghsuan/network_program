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
int sockfd,newfd,status;
struct addrinfo *seraddr_in,*p;
socklen_t sin_len;
struct addrinfo hints;
struct sockaddr_storage client_addr;
socklen_t addr_size;
char buffer[1024],s[INET_ADDRSTRLEN],buf[2048];
struct sigaction sa;
FILE *pfile;
int len,new_jpg;
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
if(bind(sock,p->ai_addr,p->ai_addrlen)==-1)
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
if(listen(sock,Backlog)==-1){
perror("listen");
exit(1);
}
sa.sa_handler=sigchld_handler;
sigemptyset(&sa.sa_mask);
sa.sa_flags=SA_RESTART;
if(sigaction(SIGCHLD,&sa,NULL)==-1)
{
perror("sigaction");
exit(1);
}
printf("server wating for connection\n");
while(1){
addr_size=sizeof client_addr;
newfd=accept(sock,(struct sockaddr*)&client_addr,&addr_size);
if(newfd==-1){
perror("accept");
continue;
}
inet_ntop(client_addr.ss_family,get_in_addr((struct sockaddr*)&client_addr),s,sizeof s);
printf("server got connection from=%s\n",s);
int i,n,ret,new_index;
if(!fork()){
close(sock);
memset(buf,0,2048);
read(newfd,buf,2047);
printf("%s\n",buf);
if(strncmp(buf, "GET /tree.jpg", 13)){
printf("nononono\n");
write(newfd,imageheader,sizeof(imageheader)-1);
new_jpg=open("tree.jpg",O_RDONLY);
if(new_jpg==-1)perror("open");
int s=sendfile(newfd,new_jpg,NULL,10000);
printf("send =%d\n",s);
close(new_jpg);
}
}
close(newfd);
exit(0);
}
fclose(pfile);
return 0;
}



