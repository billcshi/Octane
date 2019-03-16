#include <iostream>
#include <unistd.h>

#include "stage3main.h"
#include "../Router/Router.h"
#include "../Datagram/IPDatagram.h"
#include "../Datagram/ICMPDatagram.h"
#include "../Datagram/icmp_checksum.h"

void primaryRouterMainS3(Router *primary_router)
{
    primary_router->open_log("stage3.r0.out");
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
    while (1)
    {
        int from;
        int portNum;
        int length;
        int states=primary_router->selUDPorTun(buf,from,portNum,length);
        if(states==-1) break;

        if(from==0)
        {
            //from client
            IPDatagram newIP(buf);
            ICMPDatagram newICMP(&buf[20]);
            char s_src[20],s_dst[20];
            sprintf(s_src,"%d.%d.%d.%d",newIP.src[0],newIP.src[1],newIP.src[2],newIP.src[3]);
            sprintf(s_dst,"%d.%d.%d.%d",newIP.dst[0],newIP.dst[1],newIP.dst[2],newIP.dst[3]);

            sprintf(newbuf,"ICMP from port: %d, src: %s, dst: %s, type: %d\n",portNum, s_src,s_dst,newICMP.Type);
            primary_router->write_to_log(newbuf);
            printf("%s",newbuf);
            //primary_router->udp_msg_send_port(buf,clientport,length);
            primary_router->tun_msg_send(buf,length);
        }
        else
        {
            IPDatagram newIP(buf);
            ICMPDatagram newICMP(&buf[20]);
            char s_src[20],s_dst[20];
            sprintf(s_src,"%d.%d.%d.%d",newIP.src[0],newIP.src[1],newIP.src[2],newIP.src[3]);
            sprintf(s_dst,"%d.%d.%d.%d",newIP.dst[0],newIP.dst[1],newIP.dst[2],newIP.dst[3]);

            sprintf(newbuf,"ICMP from tunnel, src: %s, dst: %s, type: %d\n",s_src,s_dst,newICMP.Type);
            primary_router->write_to_log(newbuf);
            printf("%s",newbuf);
            primary_router->udp_msg_send_port(buf,clientport,length);
        }
    }

    printf("Sever Close\n");

    primary_router->close_router();
    primary_router->close_tunnel();
    primary_router->close_log();
}

void secondaryRouterMainS3(int number, int pid)
{
    Router my_router(number);
    char buf[1024],newbuf[1024];
    sprintf(buf,"stage3.r%d.out",number);
    my_router.open_log(buf);
    int my_port=my_router.getPort();
    sprintf(buf,"router: %d, pid: %d,port: %d\n",number,pid,my_port);
    my_router.write_to_log(buf);
    sprintf(buf,"%d",pid);
    my_router.udp_msg_send_port(buf,primary_router_port,1024);

    //After Init
    while(1)
    {
        int Serverport;
        int length;
        int state=my_router.selUDP(buf,Serverport,length);
        if(state == -1) break;

        IPDatagram newIP(buf);
        ICMPDatagram newICMP(&buf[20]);
        char s_src[20],s_dst[20];
        sprintf(s_src,"%d.%d.%d.%d",newIP.src[0],newIP.src[1],newIP.src[2],newIP.src[3]);
        sprintf(s_dst,"%d.%d.%d.%d",newIP.dst[0],newIP.dst[1],newIP.dst[2],newIP.dst[3]);

        sprintf(newbuf,"ICMP from port: %d, src: %s, dst: %s, type: %d\n",Serverport, s_src,s_dst,newICMP.Type);
        my_router.write_to_log(newbuf);
        printf("%s",newbuf);
        //printf("Receive Package Size:%d Byte",length);
        
        char replyDatagram[1024];
        for(int i=0;i<length;i++) replyDatagram[i]=buf[i];
        replyDatagram[length]='\0';
        //set TTL=64
        replyDatagram[8]=64;
        //set checksum=0 in IP
        replyDatagram[10]=0;
        replyDatagram[11]=0;
        //revert src, dst
        replyDatagram[12]=buf[16]; replyDatagram[16]=buf[12];
        replyDatagram[13]=buf[17]; replyDatagram[17]=buf[13];
        replyDatagram[14]=buf[18]; replyDatagram[18]=buf[14];
        replyDatagram[15]=buf[19]; replyDatagram[19]=buf[15];
        unsigned short IP_checksum=checksum(&replyDatagram[20],20);
        replyDatagram[10]=IP_checksum/256;
        replyDatagram[11]=IP_checksum%256;

        // ICMP type=0
        replyDatagram[20]=0;
        // set ICMP_checksum=0
        replyDatagram[22]=0;
        replyDatagram[23]=0;

        unsigned short ICMP_checksum=checksum(&replyDatagram[20],length-20);
        replyDatagram[22]=ICMP_checksum%256;
        replyDatagram[23]=ICMP_checksum/256;
        //printf("ICMP_checksum=%d %d %d\n",ICMP_checksum,replyDatagram[22],replyDatagram[23]);

        my_router.udp_msg_send_port(replyDatagram,Serverport,length);
    }

    printf("Client Close\n");

    my_router.close_router();
    my_router.close_log();
}

int stage3main(task_config *taskConfig)
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

    if(now_pid==primary_pid)
    {
        primaryRouterMainS3(&primary_router);
    }
    else
    {
        secondaryRouterMainS3(my_number,now_pid);
    }

    return 0;
}