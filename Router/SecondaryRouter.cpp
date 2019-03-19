#include "SecondaryRouter.h"
#include "PrimaryRouter.h"
#include <stdio.h>

#include "../Datagram/IPDatagram.h"
#include "../Datagram/icmp_checksum.h"

#define max(a,b) ((a)>(b)?(a):(b))
#define MAX_LENGTH 1500

void SecondaryRouter::get_my_ip()
{
    printf("Try to get my_ip\n");
    char send_buf[MAX_LENGTH];
    struct icmphdr* m_icmphdr=(struct icmphdr*)&send_buf[0];
    char ip_buf[MAX_LENGTH];
    struct iphdr * m_iphdr=(struct iphdr *)&ip_buf[0];
    m_iphdr->daddr=0x08080808;
    m_icmphdr->type=8;
    m_icmphdr->checksum=0;
    m_icmphdr->code=0;
    m_icmphdr->un.echo.id=0;
    m_icmphdr->un.echo.sequence=0;
    int length=20;
    m_icmphdr->checksum=checksum(send_buf,length);
    raw_icmp_send(send_buf,length,m_iphdr);
    char buf[MAX_LENGTH];
    length=recv(this->rawSocketNumber,buf,MAX_LENGTH,0);
    m_iphdr=(struct iphdr *)&buf[0];
    my_ip=m_iphdr->daddr;
    char s_src[20];
    char newbuf[MAX_LENGTH];
    unsigned int a,b,c,d;
    fromIPto4int(ntohl(my_ip),a,b,c,d);
    sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
    printf("router:%d, My_IP is:%s\n",this->routerNumber,s_src);
    return;
}

void SecondaryRouter::start(int serverport)
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
    get_my_ip();
    while(1)
    {
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
        FD_SET(this->rawSocketNumber,&rset);
        FD_SET(this->socketNumber,&rset);
        FD_SET(this->rawTCPSocketNumber,&rset);
        int n=select(max(max(this->socketNumber,this->rawSocketNumber),this->rawTCPSocketNumber)+1,&rset,NULL,NULL,&tv);
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
            else if(FD_ISSET(this->rawSocketNumber,&rset))
            {
                length=recv(this->rawSocketNumber,buf,MAX_LENGTH,0);
                portNum=0;
                from=1;
            }
            else if(FD_ISSET(this->rawTCPSocketNumber,&rset))
            {
                length=recv(this->rawTCPSocketNumber,buf,MAX_LENGTH,0);
                portNum=0;
                from=1;
            }
            else break; //should not go here
        }
        struct iphdr *m_iphdr;
        m_iphdr=(struct iphdr*) buf;
        if(from ==0)
        {
            if(m_iphdr->protocol==253)
            {
                octane_control * m_control_msg=(octane_control *)&buf[20];
                this->m_OctaneManager->Rule_Install(m_control_msg);
                m_control_msg->octane_flags=1;
                this->udp_msg_send_port(buf,serverport,length);
                continue;
            }
        }

        {
            if(m_iphdr->protocol==1)
            {
                struct icmphdr *m_icmphdr;
                m_icmphdr=(struct icmphdr*) (buf+(m_iphdr->ihl)*4);
                char s_src[20],s_dst[20];
                char newbuf[MAX_LENGTH];
                unsigned int a,b,c,d;
                fromIPto4int(ntohl(m_iphdr->saddr),a,b,c,d);
                sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
                fromIPto4int(ntohl(m_iphdr->daddr),a,b,c,d);
                sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
                if(from ==0) sprintf(newbuf,"ICMP from port: %d, src: %s, dst: %s, type: %d\n",serverport, s_src,s_dst,m_icmphdr->type);
                else sprintf(newbuf,"ICMP from raw sock, src: %s, dst: %s, type: %d\n", s_src,s_dst,m_icmphdr->type);
                this->write_to_log(newbuf);
                printf("%s",newbuf);
            }
            else if(m_iphdr->protocol==6)
            {
                struct tcphdr *m_tcphdr;
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
                if(from==0) sprintf(newbuf,"TCP from port: %d, (%s, %d, %s, %d)\n",portNum, s_src,source_port,s_dst,dest_port);
                else sprintf(newbuf,"TCP from raw sock, (%s, %d, %s, %d)\n",s_src,source_port,s_dst,dest_port);
                this->write_to_log(newbuf);
                printf("%s",newbuf);
            }
            
        }

        uint16_t output_port;
        uint8_t my_action=this->m_OctaneManager->Rule_Check(m_iphdr,output_port);
        if(my_action==NONACTION) continue;

        if(my_action==1)
        {//Forward
            if(output_port==0)
            { //Output Through Raw Socket
                char send_data[MAX_LENGTH];
                if(m_iphdr->protocol==1)
                {//ICMP
                    for(int i=(m_iphdr->ihl)*4;i<length;i++) send_data[i-(m_iphdr->ihl)*4]=buf[i];
                    struct icmphdr * m_send_icmphdr=(struct icmphdr *)&send_data[0];
                    struct icmphdr * m_icmphdr=(struct icmphdr*) (buf+(m_iphdr->ihl)*4);
                    m_send_icmphdr->type=ICMP_ECHO;
                    m_send_icmphdr->code=m_icmphdr->code;
                    m_send_icmphdr->checksum=0;
                    m_send_icmphdr->un.echo.id=m_icmphdr->un.echo.id;
                    m_send_icmphdr->un.echo.sequence=m_icmphdr->un.echo.sequence;
                    m_send_icmphdr->checksum=checksum((char *)m_send_icmphdr,length-(m_iphdr->ihl)*4);
                    ICMP_ID_SEQ_TO_IP.insert(std::make_pair(std::make_pair(m_send_icmphdr->un.echo.id,m_send_icmphdr->un.echo.sequence),m_iphdr->saddr));
                    this->raw_icmp_send(send_data,length-(m_iphdr->ihl)*4,m_iphdr);
                    continue;
                }
                else if(m_iphdr->protocol==6)
                {
                    for(int i=(m_iphdr->ihl)*4;i<length;i++) send_data[i-(m_iphdr->ihl)*4+12]=buf[i];
                    struct psdtcphdr * m_psdtcphdr=(struct psdtcphdr*) &send_data[0];
                    m_psdtcphdr->saddr=this->my_ip;
                    m_psdtcphdr->daddr=m_iphdr->daddr;
                    m_psdtcphdr->protocol=m_iphdr->protocol;
                    m_psdtcphdr->zero=0;
                    m_psdtcphdr->tcpl=ntohs(length-(m_iphdr->ihl)*4);
                    struct tcphdr * m_tcphdr=(struct tcphdr*) &send_data[12];
                    ICMP_ID_SEQ_TO_IP[std::make_pair(m_tcphdr->source,m_tcphdr->dest)]=m_iphdr->saddr;
                    m_tcphdr->check=0;
                    m_tcphdr->check=checksum((char *)m_psdtcphdr,length-(m_iphdr->ihl)*4+12);
                    this->raw_tcp_send(&send_data[12],length-(m_iphdr->ihl)*4,m_iphdr,m_tcphdr);
                }
                else
                {//Other Protocol
                    
                    continue;
                }
            }
            else
            { //Output Through UDP Socket
                char send_data[MAX_LENGTH];
                for(int i=0;i<=length;i++)
                {
                    send_data[i]=buf[i];
                }
                struct iphdr * m_send_iphdr=(struct iphdr *)&send_data[0];
                if(m_send_iphdr->protocol==1)
                {//Need to change the dst_ip in header
                    struct icmphdr * m_send_icmphdr=(struct icmphdr*) (send_data+(m_iphdr->ihl)*4);
                    m_send_iphdr->daddr=ICMP_ID_SEQ_TO_IP[std::make_pair(m_send_icmphdr->un.echo.id,m_send_icmphdr->un.echo.sequence)];
                    m_send_iphdr->check=0;
                    m_send_iphdr->check=checksum((char *)m_send_iphdr,m_send_iphdr->ihl*4);
                }
                else if(m_send_iphdr->protocol==6)
                {
                    struct tcphdr * m_send_tcphdr=(struct tcphdr*) (send_data+(m_iphdr->ihl)*4);
                    char new_data_buf[MAX_LENGTH];
                    for(int i=(m_iphdr->ihl)*4;i<length;i++) new_data_buf[i-(m_iphdr->ihl)*4+12]=buf[i];
                    struct psdtcphdr * m_psdtcphdr=(struct psdtcphdr*) &new_data_buf[0];
                    struct tcphdr * m_new_data_tcphdr=(struct tcphdr*) &new_data_buf[12];
                    m_psdtcphdr->saddr=m_iphdr->saddr;
                    m_psdtcphdr->daddr=ICMP_ID_SEQ_TO_IP[std::make_pair(m_send_tcphdr->dest,m_send_tcphdr->source)];
                    m_psdtcphdr->protocol=m_iphdr->protocol;
                    m_psdtcphdr->zero=0;
                    m_psdtcphdr->tcpl=ntohs(length-(m_iphdr->ihl)*4);
                    m_new_data_tcphdr->check=0;
                    m_new_data_tcphdr->check=checksum(new_data_buf,length-(m_iphdr->ihl)*4+12);
                    m_send_tcphdr->check=m_new_data_tcphdr->check;
                    
                    m_send_iphdr->daddr=ICMP_ID_SEQ_TO_IP[std::make_pair(m_send_tcphdr->dest,m_send_tcphdr->source)];
                    m_send_iphdr->check=0;
                    m_send_iphdr->check=checksum((char *)m_send_iphdr,m_send_iphdr->ihl*4);
                }
                udp_msg_send_port(send_data,output_port,length);
                
            }
            
        }
        else if(my_action==2)
        {//Reply
            char send_data[MAX_LENGTH];
            for(int i=0;i<=length;i++)
            {
                send_data[i]=buf[i];
            }
            struct iphdr * m_send_iphdr=(struct iphdr *)&send_data[0];
            if(m_send_iphdr->protocol==1)
            {//Reply ICMP ECHO
                m_send_iphdr->daddr=m_iphdr->saddr;
                m_send_iphdr->saddr=m_iphdr->daddr;
                m_send_iphdr->check=0;
                m_send_iphdr->check=checksum((char *)m_send_iphdr,(m_send_iphdr->ihl)*4);
                struct icmphdr *m_send_icmphdr=(struct icmphdr*) (send_data+(m_send_iphdr->ihl)*4);
                m_send_icmphdr->type=0;
                m_send_icmphdr->checksum=0;
                m_send_icmphdr->checksum=checksum((char *)m_send_icmphdr,length-(m_send_iphdr->ihl)*4);
                this->udp_msg_send_port(send_data,output_port,length);
            }
        }
        else if(my_action==3)
        {//Drop
            continue;
        }
        else if(my_action==4)
        {//Remove
            continue;
        }
        
    }
}