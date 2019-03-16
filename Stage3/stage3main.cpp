#include <iostream>
#include <unistd.h>

#include <netinet/ip.h> //For IPhdr
#include <netinet/ip_icmp.h> //For ICMPhdr

#include "stage3main.h"
#include "../Router/Router.h"
#include "../Datagram/IPDatagram.h"
#include "../Datagram/ICMPDatagram.h"
#include "../Datagram/icmp_checksum.h"

void fromIPto4int(unsigned int IP,unsigned int &a,unsigned int &b,unsigned int &c,unsigned int &d)
{
    d=IP%0x100;
    IP/=0x100;
    c=IP%0x100;
    IP/=0x100;
    b=IP%0x100;
    IP/=0x100;
    a=IP;
    return;
}

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
            struct icmphdr *m_icmphdr;
            struct iphdr *m_iphdr;
            m_iphdr=(struct iphdr*) buf;
            m_icmphdr=(struct icmphdr*) (buf+(m_iphdr->ihl)*4);
            char s_src[20],s_dst[20];
            unsigned int a,b,c,d;
            fromIPto4int(ntohl(m_iphdr->saddr),a,b,c,d);
            sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
            fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
            sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
            sprintf(newbuf,"ICMP from port: %d, src: %s, dst: %s, type: %d\n",portNum, s_src,s_dst,m_icmphdr->type);
            primary_router->write_to_log(newbuf);
            printf("%s",newbuf);
            //primary_router->udp_msg_send_port(buf,clientport,length);
            primary_router->tun_msg_send(buf,length);
        }
        else
        {
            struct icmphdr *m_icmphdr;
            struct iphdr *m_iphdr;
            m_iphdr=(struct iphdr*) buf;
            m_icmphdr=(struct icmphdr*) (buf+(m_iphdr->ihl)*4);
            char s_src[20],s_dst[20];
            unsigned int a,b,c,d;
            fromIPto4int(ntohl(m_iphdr->saddr),a,b,c,d);
            sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
            fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
            sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);

            sprintf(newbuf,"ICMP from tunnel, src: %s, dst: %s, type: %d\n",s_src,s_dst,m_icmphdr->type);
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
    my_router.raw_socket_start();
    //After Init
    uint32_t source_ip;
    int source_port;
    while(1)
    {
        int Serverport;
        int length;
        int from;
        int state=my_router.selUDPorRaw(buf,from,Serverport,length);
        if(state == -1) break;

        if(from==0)
        {//From UDP socket
            struct icmphdr *m_icmphdr;
            struct iphdr *m_iphdr;
            m_iphdr=(struct iphdr*) buf;
            m_icmphdr=(struct icmphdr*) (buf+(m_iphdr->ihl)*4);
            char s_src[20],s_dst[20];
            unsigned int a,b,c,d;
            source_port=Serverport;
            source_ip=m_iphdr->saddr;
	        int is_dst=false;
            fromIPto4int(ntohl(m_iphdr->saddr),a,b,c,d);
            sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
            fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
            sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
            if(a==10 && b==5 && c==51)
	        {
		        is_dst=true;
	        }
            sprintf(newbuf,"ICMP from port: %d, src: %s, dst: %s, type: %d\n",Serverport, s_src,s_dst,m_icmphdr->type);
            my_router.write_to_log(newbuf);
            printf("%s",newbuf);
	        if(is_dst)
	        {
                char send_data[1024];
                for(int i=0;i<=length;i++)
                {
                    send_data[i]=buf[i];
                }
                struct iphdr *send_iphdr=(struct iphdr *)&send_data[0];
                send_iphdr->daddr=m_iphdr->saddr;
                send_iphdr->saddr=m_iphdr->daddr;
                struct icmphdr *send_icmphdr=(struct icmphdr*) (send_data+(send_iphdr->ihl)*4);
                send_icmphdr->type=0;
                send_icmphdr->checksum=0;
                send_icmphdr->checksum=checksum((char *)send_icmphdr,length-(m_iphdr->ihl)*4);
                send_iphdr->check=0;
                send_iphdr->check=checksum((char *)send_iphdr,(send_iphdr->ihl)*4);
                my_router.udp_msg_send_port(send_data,source_port,length);
		        continue;
	        }
            //Send ICMP packet to outside Network
            char send_data[1024];
            for(int i=(m_iphdr->ihl)*4;i<length;i++) send_data[i-(m_iphdr->ihl)*4]=buf[i];
            struct icmphdr * m_send_icmphdr=(struct icmphdr *)&send_data[0];
            m_send_icmphdr->type=ICMP_ECHO;
            m_send_icmphdr->code=m_icmphdr->code;
            m_send_icmphdr->checksum=0;
            m_send_icmphdr->un.echo.id=m_icmphdr->un.echo.id;
            m_send_icmphdr->un.echo.sequence=m_icmphdr->un.echo.sequence;
            m_send_icmphdr->checksum=checksum((char *)m_send_icmphdr,length-(m_iphdr->ihl)*4);
            my_router.raw_icmp_send(send_data,length-(m_iphdr->ihl)*4,m_iphdr);
        }
        else if(from==1)
        {//From Raw socket
            //printf("MSG FROM RAW\n");
            struct icmphdr *m_icmphdr;
            struct iphdr *m_iphdr;
            m_iphdr=(struct iphdr*) buf;
            m_icmphdr=(struct icmphdr*) (buf+(m_iphdr->ihl)*4);
            char s_src[20],s_dst[20];
            unsigned int a,b,c,d;
            fromIPto4int(ntohl(m_iphdr->saddr),a,b,c,d);
            sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
            fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
            sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
            sprintf(newbuf,"ICMP from raw sock, src: %s, dst: %s, type: %d\n",s_src,s_dst,m_icmphdr->type);
            my_router.write_to_log(newbuf);
            printf("%s",newbuf);
            
            char send_data[1024];
            for(int i=0;i<=length;i++)
            {
                send_data[i]=buf[i];
            }
            struct iphdr *send_iphdr=(struct iphdr *)&send_data[0];
            send_iphdr->daddr=source_ip;
            send_iphdr->check=0;
            send_iphdr->check=checksum((char *)send_iphdr,(m_iphdr->ihl)*4);
            my_router.udp_msg_send_port(send_data,source_port,length);
        }
        else
        {
            break;
        }
    }

    printf("Client %d Close\n",number);

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
