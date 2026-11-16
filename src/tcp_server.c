#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT_DEFAULT 5050
#define MAX_CLIENTS 64
#define LINE_MAX_LEN 1024
#define CHUNK 65536
static int clients[MAX_CLIENTS];
static pthread_mutex_t clients_mu=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t log_mu=PTHREAD_MUTEX_INITIALIZER;
static void logmsg(const char *s){pthread_mutex_lock(&log_mu);fputs(s,stdout);fflush(stdout);pthread_mutex_unlock(&log_mu);}
static int send_all(int fd,const void *buf,size_t n){const char*p=buf;while(n){ssize_t k=send(fd,p,n,0);if(k<0&&errno==EINTR)continue;if(k<=0)return -1;p+=k;n-=(size_t)k;}return 0;}
static int recv_all(int fd,void *buf,size_t n){char*p=buf;while(n){ssize_t k=recv(fd,p,n,0);if(k<0&&errno==EINTR)continue;if(k<=0)return -1;p+=k;n-=(size_t)k;}return 0;}
static ssize_t recv_line(int fd,char *b,size_t cap){size_t i=0;while(i+1<cap){char c;ssize_t n=recv(fd,&c,1,0);if(n==0)return i? (b[i]=0,(ssize_t)i):0;if(n<0){if(errno==EINTR)continue;return -1;}if(c=='\n')break;b[i++]=c;}b[i]=0;return (ssize_t)i;}
static void broadcast(int from,const char *line){pthread_mutex_lock(&clients_mu);for(int i=0;i<MAX_CLIENTS;i++)if(clients[i]>=0&&clients[i]!=from){if(send_all(clients[i],line,strlen(line))<0){shutdown(clients[i],SHUT_RDWR);}}pthread_mutex_unlock(&clients_mu);}
static void remove_client(int fd){pthread_mutex_lock(&clients_mu);for(int i=0;i<MAX_CLIENTS;i++)if(clients[i]==fd){clients[i]=-1;break;}pthread_mutex_unlock(&clients_mu);}
static int add_client(int fd){int ok=0;pthread_mutex_lock(&clients_mu);for(int i=0;i<MAX_CLIENTS;i++)if(clients[i]<0){clients[i]=fd;ok=1;break;}pthread_mutex_unlock(&clients_mu);return ok;}
static int safe_name(const char *in,char *out,size_t n){size_t j=0;for(size_t i=0;in[i]&&j+1<n;i++){unsigned char c=(unsigned char)in[i];if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-')out[j++]=(char)c;}out[j]=0;return j>0&&strcmp(out,".")&&strcmp(out,"..");}
static void *worker(void *arg){int fd=*(int*)arg;free(arg);char line[LINE_MAX_LEN];char welcome[]="INFO Connected. Use CHAT <message> or FILE <bytes> <filename>\n";send_all(fd,welcome,sizeof(welcome)-1);
 while(1){ssize_t n=recv_line(fd,line,sizeof line);if(n<=0)break;
  if(!strncmp(line,"CHAT ",5)){char out[LINE_MAX_LEN+64];snprintf(out,sizeof out,"%s\n",line+5);broadcast(fd,out);continue;}
  if(!strncmp(line,"FILE ",5)){unsigned long long size=0;char raw[256]={0};if(sscanf(line+5,"%llu %255s",&size,raw)!=2||size>1024ULL*1024*1024){send_all(fd,"ERR Invalid file header\n",24);continue;}char name[256];if(!safe_name(raw,name,sizeof name)){send_all(fd,"ERR Invalid filename\n",22);continue;}mkdir("uploads",0755);char path[320];snprintf(path,sizeof path,"uploads/%s",name);FILE*f=fopen(path,"wb");if(!f){send_all(fd,"ERR Cannot create file\n",23);continue;}send_all(fd,"READY\n",6);char buf[CHUNK];unsigned long long left=size;int bad=0;while(left){size_t want=left>sizeof buf?sizeof buf:(size_t)left;if(recv_all(fd,buf,want)<0){bad=1;break;}if(fwrite(buf,1,want,f)!=want){bad=1;break;}left-=want;}fclose(f);if(bad){unlink(path);break;}char ack[320];snprintf(ack,sizeof ack,"OK File saved as %s (%llu bytes)\n",name,size);send_all(fd,ack,strlen(ack));continue;}
  if(!strcmp(line,"QUIT"))break;
  send_all(fd,"ERR Unknown command\n",20);
 }
 remove_client(fd);close(fd);logmsg("[server] client disconnected\n");return NULL;}
int main(int argc,char**argv){int port=argc>1?atoi(argv[1]):PORT_DEFAULT;if(port<1||port>65535){fprintf(stderr,"Invalid port\n");return 1;}signal(SIGPIPE,SIG_IGN);for(int i=0;i<MAX_CLIENTS;i++)clients[i]=-1;int s=socket(AF_INET,SOCK_STREAM,0);if(s<0){perror("socket");return 1;}int yes=1;setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes);struct sockaddr_in a={.sin_family=AF_INET,.sin_addr.s_addr=htonl(INADDR_ANY),.sin_port=htons((uint16_t)port)};if(bind(s,(struct sockaddr*)&a,sizeof a)<0||listen(s,32)<0){perror("bind/listen");close(s);return 1;}printf("IPC Nexus TCP server listening on port %d\n",port);while(1){struct sockaddr_in peer;socklen_t len=sizeof peer;int fd=accept(s,(struct sockaddr*)&peer,&len);if(fd<0){if(errno==EINTR)continue;perror("accept");continue;}if(!add_client(fd)){send_all(fd,"ERR Server full\n",16);close(fd);continue;}char msg[128];snprintf(msg,sizeof msg,"[server] client connected from %s\n",inet_ntoa(peer.sin_addr));logmsg(msg);int*p=malloc(sizeof *p);if(!p){remove_client(fd);close(fd);continue;}*p=fd;pthread_t t;if(pthread_create(&t,NULL,worker,p)){remove_client(fd);close(fd);free(p);}else pthread_detach(t);}close(s);return 0;}
