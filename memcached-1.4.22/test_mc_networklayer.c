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
#include <vector>
using namespace std;  
  
static int nthreads = 4;
struct event_base* main_base;  
static const char MESSAGE[] ="libevent say hello to you !\n";  
typedef struct conn conn;
struct conn{   //将accept到的fd，平均cq_push到各工作线程自己的CQ队列上去，通过pipe通知event_base_set到已经在loop的当前线程base上.
  size_t accept_fd;
  struct event accept_ev;  
};
typedef struct{
  pthread_t threadid; //unique thread id, if it is main thread... ...
  int index;
  int notify_recv_fd;
  int notify_send_fd;
  struct event_base *base;
  struct event notify_event; 
  //int conn_queue *new_conn_queue; /* queue of new connections to handle */
  vector<conn> new_conn_queue;
  
}LIBEVENT_THREAD;
static LIBEVENT_THREAD *threads;

void drive_machine(conn *con){
    /*
    switch(con->state){
    conn_lisening:
        ;//cq_push item with conn_listening
    conn_read:
        ;//cq_pop
    default:
        ;
    }*/
}

//handler: work at worker-thread.
void accepted_event_handler(const int fd, const short which, void *arg) {
    LIBEVENT_THREAD *me = (LIBEVENT_THREAD*)arg;

    char buf[1024];
    memset(buf, 0, 1024 );
    size_t res; 
    //throw below if...else... to drive_machine --> 
    printf("->handler thread_id:%lu \n", pthread_self());
    if(EV_READ == which){
        res = read(fd, buf, 1024);
        printf("accept event handler read... ... res:%d fd:%d which:%d buf:%s \n", res, fd, which, buf);  
    }
    else if(EV_WRITE == which){
        res = read(fd, buf, 1024);
        printf("accept event handler write... ... res:%d fd:%d which:%d buf:%s \n", res, fd, which, buf);  
    }
    //while(true){} //make sure that hanler process in block way.
}
  
//must let thread do this job, or not it will block... ...
void conn_new(const int sfd, const short event, void *arg)  
{  
    fprintf(stderr, "conn new\n");
    //cout<<"accept handle"<<endl;  
    struct sockaddr_in addr;  
    socklen_t addrlen = sizeof(addr);  
    size_t accept_fd = accept(sfd, (struct sockaddr *) &addr, &addrlen); //处理连接  

    struct bufferevent* buf_ev;  
    buf_ev = bufferevent_new(accept_fd, NULL, NULL, NULL, NULL);  
    buf_ev->wm_read.high = 4096;  
    bufferevent_write(buf_ev, MESSAGE, strlen(MESSAGE));  

    //cq_push -> create CQ_ITEM  
    int index = rand()%nthreads;
    conn con; //must be deleted
    con.accept_fd = accept_fd; 
    threads[index].new_conn_queue.push_back(con);

    char buf[1];
    buf[0] = 'c';
    if (write(threads[index].notify_send_fd, buf, 1) != 1)
      printf("write pipe failed ... ... \r\n");
    printf("write end. \n");
    fflush(stdout);
}  

static void *worker_libevent(void *arg)
{
   LIBEVENT_THREAD *me = (LIBEVENT_THREAD *)arg; 
   event_base_loop(me->base, 0);//event_base_set到base的ev绑定了一个event_handler作为回调
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
  //LIBEVENT_THREAD *me = (LIBEVENT_THREAD *)arg; 
  //me->threadid = thread.thread_id;
  printf("thread create thread_id:%lu ... ... \n",thread);
}

static void pipe_callback(int fd, short int, void *arg)
{
  LIBEVENT_THREAD *me = (LIBEVENT_THREAD*)arg;
  char buf[1];
  memset(buf, 0, 1);
  if (read(fd, buf, 1) != 1)
  {
    printf("read from pipe failed... \r\n");
    exit(1);
  }
  //item = cq_pop(me->new_conn_queue); //to deal with item(CQ_ITEM)
  if (me->new_conn_queue.size() != 0){
    conn &con = me->new_conn_queue[0];
    event_set(&con.accept_ev, con.accept_fd, EV_READ|EV_WRITE|EV_PERSIST, accepted_event_handler, me); 
    event_base_set(me->base, &con.accept_ev);
    if (event_add(&con.accept_ev, 0) == -1){
      fprintf(stderr, "[lxp error]event_add failed. \n");
      fflush(stderr);
      abort();
    }
  }
  //event_add(&accept_ev, 0); //??? ???
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
      event_set(&me->notify_event, me->notify_recv_fd, EV_READ|EV_PERSIST, pipe_callback, me);
      event_base_set(me->base, &me->notify_event);
      if (event_add(&me->notify_event, 0) == -1){
        printf("event_add failed \r\n");
        exit(1);
      }
      create_worker(worker_libevent, &threads[i]); 
    }   

    // 1. 初始化EVENT  
    main_base = event_init();  
    if(main_base)  
        printf("init event ok! main thread_id:%lu \n",pthread_self());
  
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
