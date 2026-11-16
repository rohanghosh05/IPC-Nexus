#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#define N 256
#define RQ "/tmp/ipcn_req"
#define RS "/tmp/ipcn_resp"
#define SN "/ipc_nexus_demo_shm"
static int wr(int f,const void*b,size_t n){const char*p=b;size_t d=0;while(d<n){ssize_t k=write(f,p+d,n-d);if(k<0&&errno==EINTR)continue;if(k<=0)return -1;d+=k;}return 0;}
static int rd(int f,void*b,size_t n){char*p=b;size_t d=0;while(d<n){ssize_t k=read(f,p+d,n-d);if(k<0&&errno==EINTR)continue;if(k<=0)return -1;d+=k;}return 0;}
static void waitchild(pid_t p){int s;waitpid(p,&s,0);if(WIFEXITED(s)&&WEXITSTATUS(s)==0)puts("[PASS] child exited successfully");else puts("[FAIL] child failed");}
static int pipe_demo(void){int x[2],y[2];if(pipe(x)||pipe(y)){perror("pipe");return 1;}pid_t p=fork();if(p<0)return 1;if(!p){close(x[1]);close(y[0]);char m[N]={0};if(rd(x[0],m,N))_exit(2);printf("[pipe child] %s\n",m);char a[N]="ACK via anonymous pipe";int e=wr(y[1],a,N);_exit(e?3:0);}close(x[0]);close(y[1]);char m[N]="Hello via anonymous pipe",a[N]={0};int e=wr(x[1],m,N);close(x[1]);if(!e)e=rd(y[0],a,N);close(y[0]);if(!e)printf("[pipe parent] %s\n",a);waitchild(p);return e!=0;}
static int fifo_demo(void){unlink(RQ);unlink(RS);if(mkfifo(RQ,0600)||mkfifo(RS,0600)){perror("mkfifo");return 1;}pid_t p=fork();if(!p){int f=open(RQ,O_RDONLY);char m[N]={0};if(f<0||rd(f,m,N))_exit(2);close(f);printf("[FIFO child] %s\n",m);f=open(RS,O_WRONLY);char a[N]="ACK via named FIFO";int e=f<0||wr(f,a,N);if(f>=0)close(f);_exit(e?3:0);}int f=open(RQ,O_WRONLY);char m[N]="Hello via named FIFO",a[N]={0};int e=f<0||wr(f,m,N);if(f>=0)close(f);f=open(RS,O_RDONLY);if(f<0)e=1;else{if(!e)e=rd(f,a,N);close(f);}if(!e)printf("[FIFO parent] %s\n",a);waitchild(p);unlink(RQ);unlink(RS);return e;}
struct qm{long type;char text[N];};
static int queue_demo(void){int q=msgget(IPC_PRIVATE,IPC_CREAT|0600);if(q<0){perror("msgget");return 1;}pid_t p=fork();if(!p){struct qm m;if(msgrcv(q,&m,N,1,0)<0)_exit(2);printf("[queue child] %s\n",m.text);m.type=2;strcpy(m.text,"ACK via System V queue");_exit(msgsnd(q,&m,N,0)?3:0);}struct qm m={.type=1};strcpy(m.text,"Hello via System V queue");int e=msgsnd(q,&m,N,0);if(!e&&msgrcv(q,&m,N,2,0)>=0)printf("[queue parent] %s\n",m.text);else e=1;waitchild(p);msgctl(q,IPC_RMID,NULL);return e;}
struct block{sem_t ready,done;char text[N];};
static int shm_demo(void){shm_unlink(SN);int f=shm_open(SN,O_CREAT|O_EXCL|O_RDWR,0600);if(f<0){perror("shm_open");return 1;}ftruncate(f,sizeof(struct block));struct block*s=mmap(NULL,sizeof *s,PROT_READ|PROT_WRITE,MAP_SHARED,f,0);close(f);if(s==MAP_FAILED){perror("mmap");shm_unlink(SN);return 1;}if(sem_init(&s->ready,1,0)||sem_init(&s->done,1,0)){perror("sem_init");return 1;}pid_t p=fork();if(!p){sem_wait(&s->ready);printf("[shared-memory child] %s\n",s->text);strcpy(s->text,"ACK via shared memory");sem_post(&s->done);_exit(0);}strcpy(s->text,"Hello via shared memory");sem_post(&s->ready);sem_wait(&s->done);printf("[shared-memory parent] %s\n",s->text);waitchild(p);sem_destroy(&s->ready);sem_destroy(&s->done);munmap(s,sizeof *s);shm_unlink(SN);return 0;}
static int socket_demo(void){int v[2];if(socketpair(AF_UNIX,SOCK_STREAM,0,v)){perror("socketpair");return 1;}pid_t p=fork();if(!p){close(v[0]);char m[N]={0};if(rd(v[1],m,N))_exit(2);printf("[socket child] %s\n",m);char a[N]="ACK via local socket";int e=wr(v[1],a,N);close(v[1]);_exit(e?3:0);}close(v[1]);char m[N]="Hello via local socket",a[N]={0};int e=wr(v[0],m,N);if(!e)e=rd(v[0],a,N);if(!e)printf("[socket parent] %s\n",a);close(v[0]);waitchild(p);return e!=0;}
static int all(void){int e=0;puts("\n=== Anonymous pipe ===");e|=pipe_demo();puts("\n=== Named FIFO ===");e|=fifo_demo();puts("\n=== System V message queue ===");e|=queue_demo();puts("\n=== Shared memory + semaphores ===");e|=shm_demo();puts("\n=== Local stream socket ===");e|=socket_demo();puts(e?"\nSome tests failed.":"\nAll five IPC demonstrations passed.");return e;}
static void usage(char*p){printf("Usage: %s demo-all|pipe|fifo|queue|shm|socket|benchmark\n",p);}
int main(int c,char**v){if(c!=2){usage(v[0]);return 1;}if(!strcmp(v[1],"demo-all"))return all();if(!strcmp(v[1],"pipe"))return pipe_demo();if(!strcmp(v[1],"fifo"))return fifo_demo();if(!strcmp(v[1],"queue"))return queue_demo();if(!strcmp(v[1],"shm"))return shm_demo();if(!strcmp(v[1],"socket"))return socket_demo();if(!strcmp(v[1],"benchmark")){struct timespec a,b;clock_gettime(CLOCK_MONOTONIC,&a);volatile unsigned long z=0;for(unsigned long i=0;i<1000000;i++)z+=i;clock_gettime(CLOCK_MONOTONIC,&b);printf("Timer smoke test: %.3f ms (checksum %lu)\n",(b.tv_sec-a.tv_sec)*1000.0+(b.tv_nsec-a.tv_nsec)/1e6,z);puts("This is not the IPC comparison benchmark.");return 0;}usage(v[0]);return 1;}
