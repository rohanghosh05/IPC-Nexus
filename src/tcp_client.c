#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
static int fd;static volatile int running=1;static int send_all(int s,const void*b,size_t n){const char*p=b;while(n){ssize_t k=send(s,p,n,0);if(k<0&&errno==EINTR)continue;if(k<=0)return -1;p+=k;n-=k;}return 0;}static int line(int s,char*b,size_t z){size_t i=0;while(i+1<z){char c;ssize_t n=recv(s,&c,1,0);if(n<=0)return -1;if(c=='\n')break;b[i++]=c;}b[i]=0;return (int)i;}
static void *reader(void*x){(void)x;char b[2048];while(running){int n=line(fd,b,sizeof b);if(n<0)break;printf("\n%s\n> ",b);fflush(stdout);}running=0;return NULL;}
static const char *base(const char*p){const char*q=strrchr(p,'/');return q?q+1:p;}
int main(int argc,char**argv){if(argc<3){fprintf(stderr,"Usage: %s <server-ip> <port>\n",argv[0]);return 1;}int port=atoi(argv[2]);fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0){perror("socket");return 1;}struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons((uint16_t)port)};if(inet_pton(AF_INET,argv[1],&a.sin_addr)!=1||connect(fd,(struct sockaddr*)&a,sizeof a)<0){perror("connect");return 1;}char welcome[512];if(line(fd,welcome,sizeof welcome)>=0)puts(welcome);pthread_t t;pthread_create(&t,NULL,reader,NULL);char *buf=NULL;size_t cap=0;while(running){printf("> ");fflush(stdout);ssize_t n=getline(&buf,&cap,stdin);if(n<0)break;if(n&&buf[n-1]=='\n')buf[--n]=0;if(!strncmp(buf,"/send ",6)){const char*path=buf+6;FILE*f=fopen(path,"rb");if(!f){perror("fopen");continue;}struct stat st;if(stat(path,&st)<0||st.st_size<0){perror("stat");fclose(f);continue;}char h[512];snprintf(h,sizeof h,"FILE %llu %s\n",(unsigned long long)st.st_size,base(path));if(send_all(fd,h,strlen(h))<0){fclose(f);break;}char chunk[65536];size_t k;int bad=0;while((k=fread(chunk,1,sizeof chunk,f))>0)if(send_all(fd,chunk,k)<0){bad=1;break;}fclose(f);if(bad)break;puts("File bytes sent; server confirmation will appear in the incoming messages.");continue;}if(!strcmp(buf,"/quit")){send_all(fd,"QUIT\n",5);break;}char out[1100];snprintf(out,sizeof out,"CHAT %s\n",buf);if(send_all(fd,out,strlen(out))<0)break;}free(buf);running=0;shutdown(fd,SHUT_RDWR);close(fd);pthread_join(t,NULL);return 0;}
