#include "PrimaryRouter.h"
#include <stdio.h>
#include <iostream>
#include "../Datagram/IPDatagram.h"
#include "../Datagram/icmp_checksum.h"

#define max(a,b) ((a)>(b)?(a):(b))
#define MAX_LENGTH 1500

int ReliableTransTimer::Expire()
{
    printf("Reliabe Trans Time OUT, Retry\n");
    m_Router->udp_msg_send_port(m_buf,target_port,length);
    return TimerCallback::RESCHEDULE_SAME;
}

int IdleTimeMonitor::Expire()
{
    if(m_Router->isIdle==true)
    {
        m_Router->isGoingtoStop=true;
        printf("Idle Too Long Time, Router Stop");
    }
    m_Router->isIdle=true;
    return TimerCallback::RESCHEDULE_SAME;
}

void PrimaryRouter::start(int clientport) //For Stage4 Stage5
{
    TimerCallback * tcb;
    struct timeval tv;
    char buf[MAX_LENGTH];
    int length;
    int portNum;
    int from;
    m_timerManager=new Timers;
    tcb=new IdleTimeMonitor(this);
    m_timerManager->AddTimer(15000,tcb); //IF Idle more than 15 s than stop the router
    while(1)
    {
        int act=1;
        m_timerManager->NextTimerTime(&tv);
        if (tv.tv_sec == 0 && tv.tv_usec == 0) {
            m_timerManager->ExecuteNextTimer();
            if(this->isGoingtoStop==true) break;
			continue;
		}
        if (tv.tv_sec==MAXVALUE && tv.tv_sec==0){
            break;//None any timers
        }
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(this->tunnelNumber,&rset);
        FD_SET(this->socketNumber,&rset);
        int n=select(max(this->socketNumber,this->tunnelNumber)+1,&rset,NULL,NULL,&tv);
        if(n<0)
        {
            printf("Error While Select()\n");
            break; 
        }
        else if(n==0)
        {
            //Nothing input
            continue;
        }
        else
        {
            if(FD_ISSET(this->socketNumber, &rset))
            {
                length= this->udp_wait_for_msg(buf,portNum);
                from=0;
            }
            else if(FD_ISSET(this->tunnelNumber,&rset))
            {
                length=read(this->tunnelNumber,buf,MAX_LENGTH);
                portNum=0;
                from=1;
                if(length<0)
                {
                    printf("Receive Error\n");
                    break;
                }
            }
            else break; //should not go here
        }
        struct icmphdr *m_icmphdr;
        struct iphdr *m_iphdr;
        if(from ==0)
        {
            m_iphdr=(struct iphdr*) buf;
            if(m_iphdr->protocol==253)
            {
                octane_control *m_control_msg=(octane_control *)&buf[20];
                if(m_control_msg->octane_flags==0) continue;
                handle h=seqno_to_handle[m_control_msg->octane_seqno];
                m_timerManager->RemoveTimer(h);
                continue;
            }
            else if(m_iphdr->protocol==1)
            {
                m_icmphdr=(struct icmphdr*) (buf+(m_iphdr->ihl)*4);
                char s_src[20],s_dst[20];
                unsigned int a,b,c,d;
                char newbuf[MAX_LENGTH];
                fromIPto4int(ntohl(m_iphdr->saddr),a,b,c,d);
                sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
                fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
                sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
                sprintf(newbuf,"ICMP from port: %d, src: %s, dst: %s, type: %d\n",portNum, s_src,s_dst,m_icmphdr->type);
                this->write_to_log(newbuf);
                printf("%s",newbuf);
            }
            else
            {
                //Nothing here
                continue;
            }
        }
        else
        {
            m_iphdr=(struct iphdr*) buf;
            m_icmphdr=(struct icmphdr*) (buf+(m_iphdr->ihl)*4);
            char s_src[20],s_dst[20];
            char newbuf[MAX_LENGTH];
            unsigned int a,b,c,d;
            
            fromIPto4int(ntohl(m_iphdr->saddr),a,b,c,d);
            sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
            fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
            sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
            if(a==10 && b==5 && c==51)
	        {
		        act=2;
	        }
            sprintf(newbuf,"ICMP from tunnel, src: %s, dst: %s, type: %d\n",s_src,s_dst,m_icmphdr->type);
            this->write_to_log(newbuf);
            printf("%s",newbuf);
        }

        uint16_t output_port;
        uint8_t my_action=this->m_OctaneManager->Rule_Check(m_iphdr,output_port);
        if(my_action==NONACTION)
        {
            //Add control Msg to own table
            char newbuf[MAX_LENGTH];
            {
                octane_control* new_control_packet=(octane_control *)&newbuf[0];
                //From tun1 to secondary router
                new_control_packet->octane_action=1;
                new_control_packet->octane_port=clientport;
                new_control_packet->octane_flags=0;
                new_control_packet->octane_seqno=m_seqno;
                m_seqno++;
                new_control_packet->octane_source_ip=m_iphdr->saddr;
                new_control_packet->octane_source_port=ANY_PORT;
                new_control_packet->octane_dest_ip=m_iphdr->daddr;
                new_control_packet->octane_dest_port=ANY_PORT;
                new_control_packet->octane_protocol=m_iphdr->protocol;
                m_OctaneManager->Rule_Install(new_control_packet);
                //From secondary router to tun1
                new_control_packet->octane_action=1;
                new_control_packet->octane_port=0;
                new_control_packet->octane_flags=0;
                new_control_packet->octane_seqno=m_seqno;
                m_seqno++;
                new_control_packet->octane_source_ip=m_iphdr->daddr;
                new_control_packet->octane_source_port=ANY_PORT;
                new_control_packet->octane_dest_ip=m_iphdr->saddr;
                new_control_packet->octane_dest_port=ANY_PORT;
                new_control_packet->octane_protocol=m_iphdr->protocol;
                m_OctaneManager->Rule_Install(new_control_packet);
            }
            
            //Add control Msg to secondary router Here
            if(act==1)
            {
                iphdr * new_iphdr=(iphdr *)&newbuf[0];
                new_iphdr->version=m_iphdr->version;
                new_iphdr->ihl=5;
                new_iphdr->tos=0;
                new_iphdr->tot_len=20+sizeof(octane_control);
                new_iphdr->id=0;
                new_iphdr->frag_off=0;
                new_iphdr->ttl=64;
                new_iphdr->protocol=253;
                new_iphdr->check=0;
                new_iphdr->saddr=m_iphdr->saddr;
                new_iphdr->daddr=m_iphdr->daddr;
                new_iphdr->check=checksum((char *)new_iphdr,20);
                octane_control* new_control_packet=(octane_control*)&newbuf[20];
                {
                    new_control_packet->octane_action=1;
                    new_control_packet->octane_port=0;
                    new_control_packet->octane_flags=0;
                    new_control_packet->octane_seqno=m_seqno;
                    m_seqno++;
                    new_control_packet->octane_source_ip=m_iphdr->saddr;
                    new_control_packet->octane_source_port=ANY_PORT;
                    new_control_packet->octane_dest_ip=m_iphdr->daddr;
                    new_control_packet->octane_dest_port=ANY_PORT;
                    new_control_packet->octane_protocol=m_iphdr->protocol;
                    ReliableTransTimer *new_timer=new ReliableTransTimer(this,new_control_packet->octane_seqno,clientport,newbuf,20+sizeof(octane_control));
                    handle h=m_timerManager->AddTimer(2000,new_timer);
                    seqno_to_handle[new_control_packet->octane_seqno]=h;
                    this->udp_msg_send_port((char*) new_iphdr,clientport,20+sizeof(octane_control));
                }
                {
                    new_control_packet->octane_action=1;
                    new_control_packet->octane_port=this->getPort();
                    new_control_packet->octane_flags=0;
                    new_control_packet->octane_seqno=m_seqno;
                    m_seqno++;
                    new_control_packet->octane_source_ip=m_iphdr->daddr;
                    new_control_packet->octane_source_port=ANY_PORT;
                    new_control_packet->octane_dest_ip=ANY_IP;//m_iphdr->saddr;
                    new_control_packet->octane_dest_port=ANY_PORT;
                    new_control_packet->octane_protocol=m_iphdr->protocol;
                    ReliableTransTimer *new_timer=new ReliableTransTimer(this,new_control_packet->octane_seqno,clientport,newbuf,20+sizeof(octane_control));
                    handle h=m_timerManager->AddTimer(2000,new_timer);
                    seqno_to_handle[new_control_packet->octane_seqno]=h;
                    this->udp_msg_send_port((char*) new_iphdr,clientport,20+sizeof(octane_control));
                }

            }
            else if(act==2)
            {
                iphdr * new_iphdr=(iphdr *)&newbuf[0];
                new_iphdr->version=m_iphdr->version;
                new_iphdr->ihl=5;
                new_iphdr->tos=0;
                new_iphdr->tot_len=20+sizeof(octane_control);
                new_iphdr->id=0;
                new_iphdr->frag_off=0;
                new_iphdr->ttl=64;
                new_iphdr->protocol=253;
                new_iphdr->check=0;
                new_iphdr->saddr=m_iphdr->saddr;
                new_iphdr->daddr=m_iphdr->daddr;
                new_iphdr->check=checksum((char *)new_iphdr,20);
                octane_control* new_control_packet=(octane_control*)&newbuf[20];
                new_control_packet->octane_action=2;
                new_control_packet->octane_port=this->getPort();
                new_control_packet->octane_flags=0;
                new_control_packet->octane_seqno=m_seqno;
                m_seqno++;
                new_control_packet->octane_source_ip=m_iphdr->saddr;
                new_control_packet->octane_source_port=ANY_PORT;
                new_control_packet->octane_dest_ip=m_iphdr->daddr;
                new_control_packet->octane_dest_port=ANY_PORT;
                new_control_packet->octane_protocol=m_iphdr->protocol;
                ReliableTransTimer *new_timer=new ReliableTransTimer(this,new_control_packet->octane_seqno,clientport,newbuf,20+sizeof(octane_control));
                handle h=m_timerManager->AddTimer(2000,new_timer);
                seqno_to_handle[new_control_packet->octane_seqno]=h;
                this->udp_msg_send_port((char*) new_iphdr,clientport,20+sizeof(octane_control));
            }
            my_action=this->m_OctaneManager->Rule_Check(m_iphdr,output_port);
        }
        if(my_action==1)
        {//Forward
            char newbuf[MAX_LENGTH];
            for(int i=0;i<length;i++)
            {
                newbuf[i]=buf[i];
            }
            if(output_port==0)
            {//Output to tun
                this->tun_msg_send((char *)newbuf,length);
            }
            else
            {
                this->udp_msg_send_port((char*)newbuf,output_port,length);
            }
            

        }
        else if(my_action==2)
        {//Reply
            continue;
        }
        else if(my_action==3)
        {//Drop
            continue;
        }
        else if(my_action==4)
        {//Remove
            continue;
        }
        else
        {// Should not go here
            continue;
        }
        
    }
}

void PrimaryRouter::startv2(int Stage) //For Stage6 Stage7
{
    TimerCallback * tcb;
    struct timeval tv;
    char buf[MAX_LENGTH];
    int length;
    int portNum;
    int from;
    m_timerManager=new Timers;
    tcb=new IdleTimeMonitor(this);
    m_timerManager->AddTimer(15000,tcb); //IF Idle more than 15 s than stop the router
    if(Stage==7)
    {
        char buf[MAX_LENGTH];
        struct iphdr* m_iphdr=(struct iphdr*)&buf[0];
        struct octane_control *m_octane_control=(struct octane_control *)&buf[20];
        m_octane_control->octane_action=1;  //To primarity router: (ANY,ANY,ANY,443,6) action 1 port secondary router 1
        m_octane_control->octane_flags=0;
        m_octane_control->octane_port=this->clientPort[1];
        m_octane_control->octane_seqno=m_seqno;
        m_seqno++;
        m_octane_control->octane_source_ip=ANY_IP;
        m_octane_control->octane_source_port=ANY_PORT;
        m_octane_control->octane_dest_ip=ANY_IP;
        m_octane_control->octane_dest_port=ntohs(443);
        m_octane_control->octane_protocol=6;
        m_OctaneManager->Rule_Install(m_octane_control);
        m_octane_control->octane_action=1;  //To primary router: (ANY,443,ANY,ANY,6) action 1 port 0
        m_octane_control->octane_flags=0;
        m_octane_control->octane_port=0;
        m_octane_control->octane_seqno=m_seqno;
        m_seqno++;
        m_octane_control->octane_source_ip=ANY_IP;
        m_octane_control->octane_source_port=ntohs(443);
        m_octane_control->octane_dest_ip=ANY_IP;
        m_octane_control->octane_dest_port=ANY_PORT;
        m_octane_control->octane_protocol=6;
        m_OctaneManager->Rule_Install(m_octane_control);
        {m_iphdr->protocol=253;              //To secondary router 1: (ANY, ANY,ANY,443,6) action 1 port secondary router 2
        m_octane_control->octane_action=1;
        m_octane_control->octane_flags=0;
        m_octane_control->octane_port=this->clientPort[2];
        m_octane_control->octane_seqno=m_seqno;
        m_seqno++;
        m_octane_control->octane_source_ip=ANY_IP;
        m_octane_control->octane_source_port=ANY_PORT;
        m_octane_control->octane_dest_ip=ANY_IP;
        m_octane_control->octane_dest_port=ntohs(443);
        m_octane_control->octane_protocol=6;
        ReliableTransTimer *new_timer=new ReliableTransTimer(this,m_octane_control->octane_seqno,this->clientPort[1],buf,20+sizeof(octane_control));
        handle h=m_timerManager->AddTimer(2000,new_timer);
        seqno_to_handle[m_octane_control->octane_seqno]=h;
        this->udp_msg_send_port((char*) buf,this->clientPort[1],20+sizeof(octane_control));}
        {m_iphdr->protocol=253;              //To secondary router 1: (ANY, 443,ANY,ANY,6) action 1 port primary router
        m_octane_control->octane_action=1;
        m_octane_control->octane_flags=0;
        m_octane_control->octane_port=this->portNumber;
        m_octane_control->octane_seqno=m_seqno;
        m_seqno++;
        m_octane_control->octane_source_ip=ANY_IP;
        m_octane_control->octane_source_port=ntohs(443);
        m_octane_control->octane_dest_ip=ANY_IP;
        m_octane_control->octane_dest_port=ANY_PORT;
        m_octane_control->octane_protocol=6;
        ReliableTransTimer *new_timer=new ReliableTransTimer(this,m_octane_control->octane_seqno,this->clientPort[1],buf,20+sizeof(octane_control));
        handle h=m_timerManager->AddTimer(2000,new_timer);
        seqno_to_handle[m_octane_control->octane_seqno]=h;
        this->udp_msg_send_port((char*) buf,this->clientPort[1],20+sizeof(octane_control));}
        {m_iphdr->protocol=253;              //To secondary router 2: (ANY, ANY,ANY,443,6) action 1 port 0
        m_octane_control->octane_action=1;
        m_octane_control->octane_flags=0;
        m_octane_control->octane_port=0;
        m_octane_control->octane_seqno=m_seqno;
        m_seqno++;
        m_octane_control->octane_source_ip=ANY_IP;
        m_octane_control->octane_source_port=ANY_PORT;
        m_octane_control->octane_dest_ip=ANY_IP;
        m_octane_control->octane_dest_port=ntohs(443);
        m_octane_control->octane_protocol=6;
        ReliableTransTimer *new_timer=new ReliableTransTimer(this,m_octane_control->octane_seqno,this->clientPort[2],buf,20+sizeof(octane_control));
        handle h=m_timerManager->AddTimer(2000,new_timer);
        seqno_to_handle[m_octane_control->octane_seqno]=h;
        this->udp_msg_send_port((char*) buf,this->clientPort[2],20+sizeof(octane_control));}
        {m_iphdr->protocol=253;              //To secondary router 2: (ANY, 443,ANY,ANY,6) action 1 port secondary router 1
        m_octane_control->octane_action=1;
        m_octane_control->octane_flags=0;
        m_octane_control->octane_port=this->clientPort[1];
        m_octane_control->octane_seqno=m_seqno;
        m_seqno++;
        m_octane_control->octane_source_ip=ANY_IP;
        m_octane_control->octane_source_port=ntohs(443);
        m_octane_control->octane_dest_ip=ANY_IP;
        m_octane_control->octane_dest_port=ANY_PORT;
        m_octane_control->octane_protocol=6;
        ReliableTransTimer *new_timer=new ReliableTransTimer(this,m_octane_control->octane_seqno,this->clientPort[2],buf,20+sizeof(octane_control));
        handle h=m_timerManager->AddTimer(2000,new_timer);
        seqno_to_handle[m_octane_control->octane_seqno]=h;
        this->udp_msg_send_port((char*) buf,this->clientPort[2],20+sizeof(octane_control));}
    }
    while(1)
    {
        int act=1;
        m_timerManager->NextTimerTime(&tv);
        if (tv.tv_sec == 0 && tv.tv_usec == 0) {
            m_timerManager->ExecuteNextTimer();
            if(this->isGoingtoStop==true) break;
			continue;
		}
        if (tv.tv_sec==MAXVALUE && tv.tv_sec==0){
            break;//None any timers
        }
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(this->tunnelNumber,&rset);
        FD_SET(this->socketNumber,&rset);
        int n=select(max(this->socketNumber,this->tunnelNumber)+1,&rset,NULL,NULL,&tv);
        if(n<0)
        {
            printf("Error While Select()\n");
            break; 
        }
        else if(n==0)
        {
            //Nothing input
            continue;
        }
        else
        {
            if(FD_ISSET(this->socketNumber, &rset))
            {
                length= this->udp_wait_for_msg(buf,portNum);
                from=0;
            }
            else if(FD_ISSET(this->tunnelNumber,&rset))
            {
                length=read(this->tunnelNumber,buf,MAX_LENGTH);
                portNum=0;
                from=1;
                if(length<0)
                {
                    printf("Receive Error\n");
                    break;
                }
            }
            else break; //should not go here
        }
        struct icmphdr *m_icmphdr;
        struct iphdr *m_iphdr;
        struct tcphdr * m_tcphdr;
        if(from ==0)
        {
            m_iphdr=(struct iphdr*) buf;
            if(m_iphdr->protocol==253)
            {
                octane_control *m_control_msg=(octane_control *)&buf[20];
                if(m_control_msg->octane_flags==0) continue;
                handle h=seqno_to_handle[m_control_msg->octane_seqno];
                m_timerManager->RemoveTimer(h);
                continue;
            }
            else if(m_iphdr->protocol==1)
            {
                m_icmphdr=(struct icmphdr*) (buf+(m_iphdr->ihl)*4);
                char s_src[20],s_dst[20];
                unsigned int a,b,c,d;
                char newbuf[MAX_LENGTH];
                fromIPto4int(ntohl(m_iphdr->saddr),a,b,c,d);
                sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
                fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
                sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
                sprintf(newbuf,"ICMP from port: %d, src: %s, dst: %s, type: %d\n",portNum, s_src,s_dst,m_icmphdr->type);
                this->write_to_log(newbuf);
                printf("%s",newbuf);
            }
            else if(m_iphdr->protocol==6)
            {
                m_tcphdr=(struct tcphdr*) (buf+(m_iphdr->ihl)*4);
                char s_src[20],s_dst[20];
                unsigned int a,b,c,d;
                char newbuf[MAX_LENGTH];
                fromIPto4int(ntohl(m_iphdr->saddr),a,b,c,d);
                sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
                fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
                sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
                uint16_t source_port=htons(m_tcphdr->source);
                uint16_t dest_port=htons(m_tcphdr->dest);
                sprintf(newbuf,"TCP from port: %d, (%s, %d, %s, %d)\n",portNum, s_src,source_port,s_dst,dest_port);
                this->write_to_log(newbuf);
                printf("%s",newbuf);
            }
            else
            {
                //Nothing here
                continue;
            }
        }
        else
        {
            m_iphdr=(struct iphdr*) buf;
            if(m_iphdr->protocol==1)
            {
                m_icmphdr=(struct icmphdr*) (buf+(m_iphdr->ihl)*4);
                char s_src[20],s_dst[20];
                char newbuf[MAX_LENGTH];
                unsigned int a,b,c,d;
            
                fromIPto4int(ntohl(m_iphdr->saddr),a,b,c,d);
                sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
                fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
                sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
                if(a==10 && b==5 && c==51)
	            {
		            act=2;
	            }
                sprintf(newbuf,"ICMP from tunnel, src: %s, dst: %s, type: %d\n",s_src,s_dst,m_icmphdr->type);
                this->write_to_log(newbuf);
                printf("%s",newbuf);
            }
            else if(m_iphdr->protocol==6)
            {
                m_tcphdr=(struct tcphdr*) (buf+(m_iphdr->ihl)*4);
                char s_src[20],s_dst[20];
                char newbuf[MAX_LENGTH];
                unsigned int a,b,c,d;
            
                fromIPto4int(ntohl(m_iphdr->saddr),a,b,c,d);
                sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
                fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
                sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
                uint16_t source_port=htons(m_tcphdr->source);
                uint16_t dest_port=htons(m_tcphdr->dest);
                sprintf(newbuf,"TCP from tunnel, (%s, %d, %s, %d)\n",s_src,source_port,s_dst,dest_port);
                this->write_to_log(newbuf);
                printf("%s",newbuf);
            }
            
        }

        uint16_t output_port;
        uint8_t my_action=this->m_OctaneManager->Rule_Check(m_iphdr,output_port);
        if(my_action==NONACTION)
        {
            //Add control Msg to own table
            int target_client_number;
            char newbuf[MAX_LENGTH];
            {
                octane_control* new_control_packet=(octane_control *)&newbuf[0];
                //From tun1 to secondary router
                new_control_packet->octane_action=1;
                if(m_iphdr->protocol==1)
                {
                    if(act==2) // ip=10.5.51.x
                    {
                        unsigned int a,b,c,d;
                        fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
                        switch (d)
                        {
                            case 11:
                                new_control_packet->octane_port=this->clientPort[1];
                                target_client_number=1;
                                break;
                            case 12:
                                new_control_packet->octane_port=this->clientPort[2];
                                target_client_number=2;
                                break;
                            case 13:
                                new_control_packet->octane_port=this->clientPort[3];
                                target_client_number=3;
                                break;
                            case 14:
                                new_control_packet->octane_port=this->clientPort[4];
                                target_client_number=4;
                                break;
                            default:
                                new_control_packet->octane_port=this->clientPort[1];
                                target_client_number=1;
                                break;
                        }
                    }
                    else //Other go to router 1
                    {
                        new_control_packet->octane_port=this->clientPort[1];
                        target_client_number=1;
                    }
                    new_control_packet->octane_source_port=ANY_PORT;
                    new_control_packet->octane_dest_port=ANY_PORT;
                }
                else if(m_iphdr->protocol==6)
                {
                    if(m_tcphdr->dest==ntohs(80))
                    {
                        new_control_packet->octane_port=this->clientPort[1];
                        target_client_number=1;
                    }
                    else
                    {
                        new_control_packet->octane_port=this->clientPort[2];
                        target_client_number=2;
                    }
                    new_control_packet->octane_source_port=m_tcphdr->source;
                    new_control_packet->octane_dest_port=m_tcphdr->dest;
                }
                else
                {
                    new_control_packet->octane_port=0; //Default=0
                }                
                new_control_packet->octane_flags=0;
                new_control_packet->octane_seqno=m_seqno;
                m_seqno++;
                new_control_packet->octane_source_ip=m_iphdr->saddr;
                new_control_packet->octane_dest_ip=m_iphdr->daddr;
                new_control_packet->octane_protocol=m_iphdr->protocol;
                m_OctaneManager->Rule_Install(new_control_packet);
                //From secondary router to tun1
                new_control_packet->octane_action=1;
                new_control_packet->octane_port=0;
                new_control_packet->octane_flags=0;
                new_control_packet->octane_seqno=m_seqno;
                m_seqno++;
                new_control_packet->octane_source_ip=m_iphdr->daddr;
                new_control_packet->octane_dest_ip=m_iphdr->saddr;
                if(m_iphdr->protocol==1)
                {
                    new_control_packet->octane_source_port=ANY_PORT;
                    new_control_packet->octane_dest_port=ANY_PORT;
                }
                else if(m_iphdr->protocol==6)
                {
                    new_control_packet->octane_source_port=m_tcphdr->dest;
                    new_control_packet->octane_dest_port=m_tcphdr->source;
                }
                new_control_packet->octane_protocol=m_iphdr->protocol;
                m_OctaneManager->Rule_Install(new_control_packet);
            }
            //Add control Msg to secondary router Here
            if(act==1)
            {
                iphdr * new_iphdr=(iphdr *)&newbuf[0];
                new_iphdr->version=m_iphdr->version;
                new_iphdr->ihl=5;
                new_iphdr->tos=0;
                new_iphdr->tot_len=20+sizeof(octane_control);
                new_iphdr->id=0;
                new_iphdr->frag_off=0;
                new_iphdr->ttl=64;
                new_iphdr->protocol=253;
                new_iphdr->check=0;
                new_iphdr->saddr=m_iphdr->saddr;
                new_iphdr->daddr=m_iphdr->daddr;
                new_iphdr->check=checksum((char *)new_iphdr,20);
                octane_control* new_control_packet=(octane_control*)&newbuf[20];
                {
                    new_control_packet->octane_action=1;
                    new_control_packet->octane_port=0;
                    new_control_packet->octane_flags=0;
                    new_control_packet->octane_seqno=m_seqno;
                    m_seqno++;
                    new_control_packet->octane_source_ip=m_iphdr->saddr;
                    if(m_iphdr->protocol==1) 
                    {
                        new_control_packet->octane_source_port=ANY_PORT;
                        new_control_packet->octane_dest_port=ANY_PORT;
                    }
                    else if(m_iphdr->protocol==6)
                    {
                        new_control_packet->octane_source_port=m_tcphdr->source;
                        new_control_packet->octane_dest_port=m_tcphdr->dest;
                    }
                    new_control_packet->octane_dest_ip=m_iphdr->daddr;
                    new_control_packet->octane_protocol=m_iphdr->protocol;
                    ReliableTransTimer *new_timer=new ReliableTransTimer(this,new_control_packet->octane_seqno,this->clientPort[target_client_number],newbuf,20+sizeof(octane_control));
                    handle h=m_timerManager->AddTimer(2000,new_timer);
                    seqno_to_handle[new_control_packet->octane_seqno]=h;
                    this->udp_msg_send_port((char*) new_iphdr,this->clientPort[target_client_number],20+sizeof(octane_control));
                }
                {
                    new_control_packet->octane_action=1;
                    new_control_packet->octane_port=this->getPort();
                    new_control_packet->octane_flags=0;
                    new_control_packet->octane_seqno=m_seqno;
                    m_seqno++;
                    new_control_packet->octane_source_ip=m_iphdr->daddr;
                    if(m_iphdr->protocol==1) 
                    {
                        new_control_packet->octane_source_port=ANY_PORT;
                        new_control_packet->octane_dest_port=ANY_PORT;
                    }
                    else if(m_iphdr->protocol==6)
                    {
                        new_control_packet->octane_source_port=m_tcphdr->dest;
                        new_control_packet->octane_dest_port=m_tcphdr->source;
                    }
                    new_control_packet->octane_dest_ip=ANY_IP;//m_iphdr->saddr;
                    new_control_packet->octane_protocol=m_iphdr->protocol;
                    ReliableTransTimer *new_timer=new ReliableTransTimer(this,new_control_packet->octane_seqno,this->clientPort[target_client_number],newbuf,20+sizeof(octane_control));
                    handle h=m_timerManager->AddTimer(2000,new_timer);
                    seqno_to_handle[new_control_packet->octane_seqno]=h;
                    this->udp_msg_send_port((char*) new_iphdr,this->clientPort[target_client_number],20+sizeof(octane_control));
                }

            }
            else if(act==2)
            {
                iphdr * new_iphdr=(iphdr *)&newbuf[0];
                new_iphdr->version=m_iphdr->version;
                new_iphdr->ihl=5;
                new_iphdr->tos=0;
                new_iphdr->tot_len=20+sizeof(octane_control);
                new_iphdr->id=0;
                new_iphdr->frag_off=0;
                new_iphdr->ttl=64;
                new_iphdr->protocol=253;
                new_iphdr->check=0;
                new_iphdr->saddr=m_iphdr->saddr;
                new_iphdr->daddr=m_iphdr->daddr;
                new_iphdr->check=checksum((char *)new_iphdr,20);
                octane_control* new_control_packet=(octane_control*)&newbuf[20];
                new_control_packet->octane_action=2;
                new_control_packet->octane_port=this->getPort();
                new_control_packet->octane_flags=0;
                new_control_packet->octane_seqno=m_seqno;
                m_seqno++;
                new_control_packet->octane_source_ip=m_iphdr->saddr;
                new_control_packet->octane_source_port=ANY_PORT;
                new_control_packet->octane_dest_ip=m_iphdr->daddr;
                new_control_packet->octane_dest_port=ANY_PORT;
                new_control_packet->octane_protocol=m_iphdr->protocol;
                ReliableTransTimer *new_timer=new ReliableTransTimer(this,new_control_packet->octane_seqno,this->clientPort[target_client_number],newbuf,20+sizeof(octane_control));
                handle h=m_timerManager->AddTimer(2000,new_timer);
                seqno_to_handle[new_control_packet->octane_seqno]=h;
                this->udp_msg_send_port((char*) new_iphdr,this->clientPort[target_client_number],20+sizeof(octane_control));
            }
            my_action=this->m_OctaneManager->Rule_Check(m_iphdr,output_port);
        }
        if(my_action==1)
        {//Forward
            char newbuf[MAX_LENGTH];
            for(int i=0;i<length;i++)
            {
                newbuf[i]=buf[i];
            }
            if(output_port==0)
            {//Output to tun
                this->tun_msg_send((char *)newbuf,length);
            }
            else
            {
                this->udp_msg_send_port((char*)newbuf,output_port,length);
            }
            

        }
        else if(my_action==2)
        {//Reply
            continue;
        }
        else if(my_action==3)
        {//Drop
            continue;
        }
        else if(my_action==4)
        {//Remove
            continue;
        }
        else
        {// Should not go here
            continue;
        }
        
    }
}