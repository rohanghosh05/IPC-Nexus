#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#define ROUNDS 2000
#define SZ 1024
#define FIFO_A "/tmp/ipcn_bench_a"
#define FIFO_B "/tmp/ipcn_bench_b"
#define SHM_NAME "/ipcn_bench_shm"
static long long ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (long long)t.tv_sec*1000000000LL+t.tv_nsec;}
static int wr(int f,const void*b,size_t n){const char*p=b;while(n){ssize_t k=write(f,p,n);if(k<0&&errno==EINTR)continue;if(k<=0)return -1;p+=k;n-=k;}return 0;}
static int rd(int f,void*b,size_t n){char*p=b;while(n){ssize_t k=read(f,p,n);if(k<0&&errno==EINTR)continue;if(k<=0)return -1;p+=k;n-=k;}return 0;}
static double pipe_test(void){int a[2],b[2];pipe(a);pipe(b);pid_t p=fork();if(!p){close(a[1]);close(b[0]);char x[SZ];for(int i=0;i<ROUNDS;i++){if(rd(a[0],x,SZ))_exit(2);if(wr(b[1],x,SZ))_exit(3);}_exit(0);}close(a[0]);close(b[1]);char x[SZ]={1};long long t=ns();for(int i=0;i<ROUNDS;i++){if(wr(a[1],x,SZ)||rd(b[0],x,SZ))break;}long long e=ns();close(a[1]);close(b[0]);waitpid(p,NULL,0);return (double)(e-t)/ROUNDS/1000.0;}
static double socket_test(void){int s[2];socketpair(AF_UNIX,SOCK_STREAM,0,s);pid_t p=fork();if(!p){close(s[0]);char x[SZ];for(int i=0;i<ROUNDS;i++){if(rd(s[1],x,SZ))_exit(2);if(wr(s[1],x,SZ))_exit(3);}_exit(0);}close(s[1]);char x[SZ]={1};long long t=ns();for(int i=0;i<ROUNDS;i++){if(wr(s[0],x,SZ)||rd(s[0],x,SZ))break;}long long e=ns();close(s[0]);waitpid(p,NULL,0);return (double)(e-t)/ROUNDS/1000.0;}
struct msg{long type;char data[SZ];};static double queue_test(void){int q=msgget(IPC_PRIVATE,0600|IPC_CREAT);pid_t p=fork();if(!p){struct msg m;for(int i=0;i<ROUNDS;i++){if(msgrcv(q,&m,SZ,1,0)<0)_exit(2);m.type=2;if(msgsnd(q,&m,SZ,0))_exit(3);}_exit(0);}struct msg m={.type=1};long long t=ns();for(int i=0;i<ROUNDS;i++){m.type=1;if(msgsnd(q,&m,SZ,0)||msgrcv(q,&m,SZ,2,0)<0)break;}long long e=ns();waitpid(p,NULL,0);msgctl(q,IPC_RMID,NULL);return (double)(e-t)/ROUNDS/1000.0;}
static double fifo_test(void){unlink(FIFO_A);unlink(FIFO_B);mkfifo(FIFO_A,0600);mkfifo(FIFO_B,0600);pid_t p=fork();if(!p){int r=open(FIFO_A,O_RDONLY),w=open(FIFO_B,O_WRONLY);char x[SZ];for(int i=0;i<ROUNDS;i++){if(rd(r,x,SZ)||wr(w,x,SZ))_exit(2);}_exit(0);}int w=open(FIFO_A,O_WRONLY),r=open(FIFO_B,O_RDONLY);char x[SZ]={1};long long t=ns();for(int i=0;i<ROUNDS;i++){if(wr(w,x,SZ)||rd(r,x,SZ))break;}long long e=ns();close(w);close(r);waitpid(p,NULL,0);unlink(FIFO_A);unlink(FIFO_B);return (double)(e-t)/ROUNDS/1000.0;}
struct shared{sem_t req,resp;char data[SZ];};static double shm_test(void){shm_unlink(SHM_NAME);int f=shm_open(SHM_NAME,O_CREAT|O_EXCL|O_RDWR,0600);ftruncate(f,sizeof(struct shared));struct shared*s=mmap(NULL,sizeof *s,PROT_READ|PROT_WRITE,MAP_SHARED,f,0);close(f);sem_init(&s->req,1,0);sem_init(&s->resp,1,0);pid_t p=fork();if(!p){for(int i=0;i<ROUNDS;i++){if(sem_wait(&s->req))_exit(2);s->data[0]++;sem_post(&s->resp);}_exit(0);}long long t=ns();for(int i=0;i<ROUNDS;i++){sem_post(&s->req);if(sem_wait(&s->resp))break;}long long e=ns();waitpid(p,NULL,0);sem_destroy(&s->req);sem_destroy(&s->resp);munmap(s,sizeof *s);shm_unlink(SHM_NAME);return (double)(e-t)/ROUNDS/1000.0;}
int main(int argc,char**argv){const char*out=argc>1?argv[1]:"results/benchmark.csv";FILE*f=fopen(out,"w");if(!f){perror(out);return 1;}fprintf(f,"mechanism,payload_bytes,round_trips,avg_round_trip_us,estimated_one_way_us,throughput_mib_s\n");struct{const char*n;double(*fn)(void);}tests[]={{"Anonymous pipe",pipe_test},{"Named FIFO",fifo_test},{"System V message queue",queue_test},{"POSIX shared memory + semaphores",shm_test},{"UNIX domain socketpair",socket_test}};for(size_t i=0;i<sizeof tests/sizeof tests[0];i++){double us=tests[i].fn();double mib=((double)SZ*ROUNDS*2.0)/(1024.0*1024.0)/(us*ROUNDS/1e6);fprintf(f,"%s,%d,%d,%.3f,%.3f,%.3f\n",tests[i].n,SZ,ROUNDS,us,us/2,mib);printf("%-34s avg round trip: %8.3f us | estimated throughput: %.2f MiB/s\n",tests[i].n,us,mib);}fclose(f);printf("CSV written to %s\n",out);return 0;}
