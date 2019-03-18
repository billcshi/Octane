#include <iostream>
#include <unistd.h>

#include <netinet/ip.h> //For IPhdr
#include <netinet/ip_icmp.h> //For ICMPhdr

#include "stage6main.h"
#include "../Router/Router.h"
#include "../Router/PrimaryRouter.h"
#include "../Router/SecondaryRouter.h"
#include "../Datagram/IPDatagram.h"
#include "../Datagram/ICMPDatagram.h"
#include "../Datagram/icmp_checksum.h"
#include "../timers/timers.hh"

void primaryRouterMainS6(int number, PrimaryRouter *primary_router)
{
    primary_router->open_log("stage6.r0.out");
    char buf[1024];
    sprintf(buf,"primary port: %d\n",primary_router_port);
    primary_router->write_to_log(buf);
    for(int i=0;i<number;i++)
    {
        int routernumber,clientport,routerpid;
        primary_router->udp_wait_for_msg(buf,clientport);
        sscanf(buf,"%d%d",&routernumber,&routerpid);
        char newbuf[1024];
        sprintf(newbuf,"router: %d, pid: %d, port:%d\n",routernumber,routerpid,clientport);
        primary_router->clientPort[routernumber]=clientport;
    }
    //After Init
    char TunnelName[16]="tun1";
    primary_router->tunnel_start(TunnelName);
    //primary_router->start(clientport);
    primary_router->startv2();
    printf("Sever Close\n");

    primary_router->close_router();
    primary_router->close_tunnel();
    primary_router->close_log();
}

void secondaryRouterMainS6(int number, int pid, int drop_after)
{
    SecondaryRouter my_router(number);
    char buf[1024],newbuf[1024];
    sprintf(buf,"stage6.r%d.out",number);
    my_router.open_log(buf);
    int my_port=my_router.getPort();
    sprintf(buf,"router: %d, pid: %d,port: %d\n",number,pid,my_port);
    my_router.write_to_log(buf);
    sprintf(buf,"%d %d",number,pid);
    my_router.udp_msg_send_port(buf,primary_router_port,1024);
    char eth[5];
    sprintf(eth,"eth%d",number);
    my_router.raw_socket_start(eth);
    my_router.serverPort=primary_router_port;
    my_router.start(primary_router_port);

    printf("Client %d Close\n",number);

    my_router.close_router();
    my_router.close_log();
}

int stage6main(task_config *taskConfig)
{
    int now_pid,my_number;
    PrimaryRouter primary_router(0);
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

    if(now_pid==primary_pid)
    {
        primaryRouterMainS6(taskConfig->num_routers,&primary_router);
    }
    else
    {
        secondaryRouterMainS6(my_number,now_pid,taskConfig->drop_after);
    }

    return 0;
}
