// 服务器端  
#include <stdlib.h>
#include <iostream>  
#include <event.h>  
#include <sys/socket.h>  
#include <sys/types.h>  
#include <netinet/in.h>  
#include <string.h>  
#include <fcntl.h>  
#include <pthread.h>
#include <unistd.h>
using namespace std;  
  
struct event_base* main_base;  
static const char MESSAGE[] ="return from libevent write!\n";  
typedef struct{
  int index;
  int notify_recv_fd;
  int notify_send_fd;
  struct event_base *base;
  struct event notify_event; 
  //int conn_queue *new_conn_queue;
}LIBEVENT_THREAD;
static LIBEVENT_THREAD *threads;
static int nthreads = 4;


void accept_event_handler(const int fd, const short which, void *arg) {
    //? how to deal with read data ?
    if(EV_READ == which){
        printf("accept event handler ... ... fd:%d which:%d \n", fd, which);  
    }
}
  
int accept_fd;
struct event accept_ev;  
struct event_base* accept_main_base = event_init();  
void conn_new(const int sfd, const short event, void *arg)  
{  
    fprintf(stderr, "conn new\n");
    //cout<<"accept handle"<<endl;  
    struct sockaddr_in addr;  
    socklen_t addrlen = sizeof(addr);  
    accept_fd = accept(sfd, (struct sockaddr *) &addr, &addrlen); //处理连接  

    //try to using 
    //struct event accept_ev;  
    //struct event_base* accept_main_base = event_init();  
    //accept_main_base = event_init();  
    event_set(&accept_ev, accept_fd, EV_READ|EV_WRITE|EV_PERSIST, accept_event_handler, NULL); 
    //event_base_set(accept_main_base, &accept_ev);
    event_base_set(main_base, &accept_ev);
    if (event_add(&accept_ev, 0) == -1){
        fprintf(stderr, "[lxp error]event_add failed. \n");
        fflush(stderr);
        abort();
    }
    //event_base_loop(accept_main_base, 0);  
    //try END.


    struct bufferevent* buf_ev;  
    buf_ev = bufferevent_new(accept_fd, NULL, NULL, NULL, NULL);  
    buf_ev->wm_read.high = 4096;  
    //cout<<"event write"<<endl;  
    bufferevent_write(buf_ev, MESSAGE, strlen(MESSAGE));  
    //cq_push
    char buf[1];
    buf[0] = 'c';
    int index = rand()%nthreads;
    if (write(threads[index].notify_send_fd, buf, 1) != 1)
      printf("write pipe failed ... ... \r\n");
}  

static void *worker_libevent(void *arg)
{
   LIBEVENT_THREAD *me = (LIBEVENT_THREAD *)arg; 
   event_base_loop(me->base, 0);
   return NULL;
}
static void create_worker(void *(*func)(void *), void *arg)
{
  pthread_t thread;
  pthread_attr_t attr;
  int ret;
  pthread_attr_init(&attr);
  if ((ret=pthread_create(&thread, &attr, func, arg))  != 0)
  {
    printf("pthread create failed \r\n");
    exit(1);
  }
}
static void thread_libevent_proc(int fd, short int, void *arg)
{
  LIBEVENT_THREAD *me = (LIBEVENT_THREAD*)arg;
  char buf[1];
  memset(buf, 0, 1);
  if (read(fd, buf, 1) != 1)
  {
    //cq_pop
    printf("read from pipe failed... \r\n");
    exit(1);
  }
  printf("New client come, %c thread:%d is working. \r\n", buf[0], me->index);
}
int main()  
{  
    cout<<"Server start !"<<endl;  
    //start workthreads 
    threads = (LIBEVENT_THREAD *)calloc(nthreads, sizeof(LIBEVENT_THREAD));
    //error check
    for (int i=0; i<nthreads; ++i)
    {
      //thread data prepare
      threads[i].index = i;
      int fds[2]; // [recv, send]
      if (pipe(fds) != 0){
        printf("pipe failed !");
        exit(1);
      }
      threads[i].notify_recv_fd = fds[0];
      threads[i].notify_send_fd = fds[1];
      threads[i].base = event_init();
      LIBEVENT_THREAD *me = &threads[i];
      event_set(&me->notify_event, me->notify_recv_fd, EV_READ|EV_PERSIST, thread_libevent_proc, me);
      event_base_set(me->base, &me->notify_event);
      if (event_add(&me->notify_event, 0) == -1){
        printf("event_add failed \r\n");
        exit(1);
      }
      create_worker(worker_libevent, &threads[i]); 
    }   
/*
    buf[0] = 'c';
    if (write(threads[0].notify_send_fd, buf, 1) != 1)
      printf("write pipe failed ... ... \r\n");
    buf[0] = 's';
    if (write(threads[2].notify_send_fd, buf, 1) != 1)
      printf("write pipe failed ... ... \r\n");
  */
    // 1. 初始化EVENT  
    main_base = event_init();  
    if(main_base)  
        cout<<"init event ok!"<<endl;  
  
    // 2. 初始化SOCKET  
    int sListen;  
  
    // Create listening socket  
    sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
  
    // Bind  
    struct sockaddr_in server_addr;  
    bzero(&server_addr,sizeof(struct sockaddr_in));  
    server_addr.sin_family=AF_INET;  
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);  
    int portnumber = 8080;  
    server_addr.sin_port = htons(portnumber);  
  
    /* 捆绑sockfd描述符  */  
    if(bind(sListen,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)  
    {  
        cout<<"error!"<<endl;  
        return -1;  
    }  
  
    // Listen  
    ::listen(sListen, 3);  
    cout<<"Server is listening!\n"<<endl;  
  
    /*将描述符设置为非阻塞*/  
    int flags = ::fcntl(sListen, F_GETFL);  
    flags |= O_NONBLOCK;  
    fcntl(sListen, F_SETFL, flags);  
  
    // 3. 创建EVENT 事件  
    struct event ev;  
    event_set(&ev, sListen, EV_READ | EV_PERSIST, conn_new, (void *)&ev);  
  
    // 4. 事件添加与删除  
    event_add(&ev, NULL);  
  
    // 5. 进入事件循环  
    event_base_loop(main_base, 0);  

    cout<<"over!"<<endl;  
}  
