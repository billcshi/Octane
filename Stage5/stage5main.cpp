#include <iostream>
#include <unistd.h>

#include <netinet/ip.h> //For IPhdr
#include <netinet/ip_icmp.h> //For ICMPhdr

#include "stage5main.h"
#include "../Router/Router.h"
#include "../Router/PrimaryRouter.h"
#include "../Router/SecondaryRouter.h"
#include "../Datagram/IPDatagram.h"
#include "../Datagram/ICMPDatagram.h"
#include "../Datagram/icmp_checksum.h"
#include "../timers/timers.hh"

void primaryRouterMainS5(PrimaryRouter *primary_router)
{
    primary_router->open_log("stage5.r0.out");
    char buf[1024];
    sprintf(buf,"primary port: %d\n",primary_router_port);
    primary_router->write_to_log(buf);
    int clientport;
    primary_router->udp_wait_for_msg(buf,clientport);
    char newbuf[1024];
    sprintf(newbuf,"router: %d, pid: %s, port:%d\n",1,buf,clientport);
    primary_router->write_to_log(newbuf);
    //After Init
    char TunnelName[16]="tun1";
    primary_router->tunnel_start(TunnelName);
    primary_router->start(clientport);

    printf("Sever Close\n");

    primary_router->close_router();
    primary_router->close_tunnel();
    primary_router->close_log();
}

void secondaryRouterMainS5(int number, int pid, int drop_after)
{
    SecondaryRouter my_router(number);
    char buf[1024],newbuf[1024];
    sprintf(buf,"stage5.r%d.out",number);
    my_router.open_log(buf);
    int my_port=my_router.getPort();
    sprintf(buf,"router: %d, pid: %d,port: %d\n",number,pid,my_port);
    my_router.write_to_log(buf);
    sprintf(buf,"%d",pid);
    my_router.udp_msg_send_port(buf,primary_router_port,1024);
    my_router.raw_socket_start("eth1");
    my_router.start(primary_router_port);

    printf("Client %d Close\n",number);

    my_router.close_router();
    my_router.close_log();
}

int stage5main(task_config *taskConfig)
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
        primaryRouterMainS5(&primary_router);
    }
    else
    {
        secondaryRouterMainS5(my_number,now_pid,taskConfig->drop_after);
    }

    return 0;
}
