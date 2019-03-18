#include "PrimaryRouter.h"
#include <stdio.h>

#include "../Datagram/IPDatagram.h"
#include "../Datagram/icmp_checksum.h"

#define max(a,b) ((a)>(b)?(a):(b))
#define MAX_LENGTH 1024

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

void PrimaryRouter::start(int clientport)
{
    TimerCallback * tcb;
    struct timeval tv;
    char buf[1024];
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
                char newbuf[1024];
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
            char newbuf[1024];
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
            char newbuf[1024];
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
            char newbuf[1024];
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