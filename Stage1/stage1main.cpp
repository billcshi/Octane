#include <iostream>
#include <unistd.h>

#include "stage1main.h"
#include "../Router/Router.h"

int primary_router_port;
int primary_pid;

void primaryRouterMain(Router *primary_router)
{
    primary_router->open_log("stage1.r0.out");
    char buf[1024];
    sprintf(buf,"primary port: %d\n",primary_router_port);
    primary_router->write_to_log(buf);
    int port;
    primary_router->udp_wait_for_msg(buf,port);
    char newbuf[60];
    sprintf(newbuf,"router: %d, pid: %s, port:%d\n",1,buf,port);
    primary_router->write_to_log(newbuf);
}

void secondaryRouterMain(int number, int pid)
{
    Router my_router(number);
    char buf[64];
    sprintf(buf,"stage1.r%d.out",number);
    my_router.open_log(buf);
    int my_port=my_router.getPort();
    sprintf(buf,"router: %d, pid: %d,port: %d\n",number,pid,my_port);
    my_router.write_to_log(buf);
    sprintf(buf,"%d",pid);
    my_router.udp_msg_send_port(buf,primary_router_port);
}

int stage1main(task_config *taskConfig)
{
    int now_pid,my_number;
    Router primary_router(0);
    primary_router_port=primary_router.getPort();
    primary_pid=getpid();
    now_pid=getpid();
    my_number=0;
    for(int i=0;i<taskConfig->num_routers;i++)
    {//create several secondary routers
        int my_pid=fork();
        if(my_pid>0)
        {
            now_pid=my_pid;
            my_number=i+1;
            break;
        }
        if(my_pid<0)
        {
            printf("Cannot fork a new process\n");
            return -1;
        }
    }

    //printf("my_number:%d my_pid:%d\n",my_number,now_pid);

    if(now_pid==primary_pid)
    {
        primaryRouterMain(&primary_router);
    }
    else
    {
        secondaryRouterMain(my_number,now_pid);
    }
    
    return 0;
}