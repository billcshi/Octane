#include <netinet/ip.h> //For IPhdr
#include <netinet/ip_icmp.h> //For ICMPhdr

#include "Router.h"
#include "sample_tunnel.h"

#define MAX_LENGTH 1024

int Router::UDP_start()
{
    this->socketNumber=socket(AF_INET,SOCK_DGRAM,0);
    if(this->socketNumber<0)
    {
        printf("Open Socket Error\n");
        return -1;
    }
    routeraddr.sin_family = AF_INET;
    routeraddr.sin_addr.s_addr=htonl(INADDR_ANY);
    routeraddr.sin_port=htons(0);

    if(bind(this->socketNumber,(struct sockaddr*) &routeraddr,sizeof(routeraddr))<0)
    {
        printf("Binding Socket Error\n");
        return -1;
    }

    this->isSockOpen=true;

    struct sockaddr_in localaddr;
    socklen_t addrlen;
    addrlen=sizeof(localaddr);
    getsockname(this->socketNumber,(struct sockaddr*) &localaddr,&addrlen);
    this->portNumber=ntohs(localaddr.sin_port);
    return 0;
}

int Router::raw_socket_start()
{
    this->rawSocketNumber=socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(this->rawSocketNumber<0)
    {
        printf("Open RAW_Socket Error\n");
        return -1;
    }
    const char *opt="eth2";
    const int len=strnlen(opt,IFNAMSIZ);
    if(setsockopt(this->rawSocketNumber,SOL_SOCKET,SO_BINDTODEVICE,opt,len)<0)
    {
        printf("Binding Error\n");
        return -1;
    }
    printf("Finish RAW_Socket INIT\n");
    return 0;

}

void Router::tunnel_start(char *tunnelName)
{
    this->tunnelNumber=tun_alloc(tunnelName, IFF_TUN | IFF_NO_PI); 

    if(this->tunnelNumber<0)
    {
        printf("Error in open tunnel\n");
        return;
    }
}

void Router::open_log(const char * filename)
{
    fp_log=fopen(filename,"w");
    if(fp_log==NULL)
    {
        printf("Unable to open the log file");
        return;
    }
    islogOpen=true;
    return;
}

void Router::close_log()
{
    if(islogOpen)
    {
        fclose(fp_log);
        islogOpen=false;
    }
    return;
}

void Router::write_to_log(char *outputStr)
{
    if(!islogOpen) return;
    fprintf(fp_log,"%s",outputStr);
}

void Router::writeln_to_log(char *outputStr)
{
    if(!islogOpen) return;
    fprintf(fp_log,"%s\n",outputStr);
}

void Router::close_router()
{
    shutdown(socketNumber,SHUT_RDWR);
    if(isRawSockOpen)
    {
        close(this->rawSocketNumber);
    }
    char buf[40];
    sprintf(buf,"router %d closed\n",routerNumber);
    write_to_log(buf);
}

void Router::close_tunnel()
{
    close(this->tunnelNumber);
    return;
}

void Router::udp_msg_send(char *msg, struct sockaddr* dst)
{
    socklen_t len=sizeof(*dst);
    //printf("Router %d Sending:%s\n",this->routerNumber,msg);
    sendto(socketNumber, msg, MAX_LENGTH, 0, dst, len);
}

void Router::udp_msg_send_port(char * msg,int portNumber, int length=MAX_LENGTH)
{
    
    struct sockaddr_in dst;
    dst.sin_family=AF_INET;
    dst.sin_addr.s_addr=htonl(INADDR_ANY);
    dst.sin_port=htons(portNumber);
    socklen_t len=sizeof(dst);
    //printf("Router %d Sending:%s\n",this->routerNumber,msg);
    int k=sendto(socketNumber,msg,length,0,(struct sockaddr *) &dst,len);
}

void Router::raw_icmp_send(char *buffer,int length,void* v_iphdr)
{
    struct msghdr msgsent;
    struct sockaddr_in dst_addr;
    struct iovec iov;
    struct iphdr * m_iphdr=(struct iphdr * )v_iphdr;
    dst_addr.sin_family=AF_INET;
    dst_addr.sin_addr.s_addr=m_iphdr->daddr;
    dst_addr.sin_port=htonl(0);
    msgsent.msg_name=&dst_addr;
    msgsent.msg_namelen=sizeof(dst_addr);

    iov.iov_base=buffer;
    iov.iov_len=length;
    msgsent.msg_iov=&iov;
    msgsent.msg_iovlen=1;

    msgsent.msg_control=NULL;
    msgsent.msg_controllen=0;

    msgsent.msg_flags=0;

    int n=sendmsg(this->rawSocketNumber,&msgsent,0);
    if(n<0)
    {
        printf("SendMsgError\n");
    }
    return;
}

void Router::tun_msg_send(char * msg,int length)
{
    write(this->tunnelNumber,msg,length);
    return;
}

int Router::udp_wait_for_msg(char * msgBuf, int &incomePort)
{
    struct sockaddr_in src_addr; //Record the incoming message source
    socklen_t len=sizeof(src_addr);
    int count;
    count = recvfrom(socketNumber,msgBuf,MAX_LENGTH,0,(struct sockaddr*) &src_addr,&len);
    //printf("Router %d Receiving:%s\n",this->routerNumber,msgBuf);
    incomePort=ntohs(src_addr.sin_port);
    return count;
}

int max(int a,int b)
{
    if(a>b) return a;
    else return b;
}

int Router::selUDPorTun(char* Buf,int &from,int &portNum, int &strlength)
{
    timeval myclock;
    myclock.tv_sec=15;  //close the router after 15s idle
    myclock.tv_usec=0;

    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(this->tunnelNumber,&rset);
    FD_SET(this->socketNumber,&rset);
   

    int n=select(max(this->socketNumber,this->tunnelNumber)+1,&rset,NULL,NULL,&myclock);

    if(n<0)
    {
        printf("Error While Select()\n");
        return -1; 
    }
    else if(n==0)
    {
        //printf("Timeout\n");
        return -1;
    }
    else
    {
        if(FD_ISSET(this->socketNumber, &rset))
        {
            strlength= this->udp_wait_for_msg(Buf,portNum);
            from=0;
            //printf("Receive a packet from port: %d\n",portNum);
            return 0;
        }
        if(FD_ISSET(this->tunnelNumber,&rset))
        {
            int length=read(this->tunnelNumber,Buf,MAX_LENGTH);
            if(length<0)
            {
                printf("Receive Error\n");
                return -1;
            }
            from=1;
            strlength=length;
            //printf("Receive a packet from tunnel, the size is %d\n",length);
            return 0;
        }
        else
        {
            //Do nothing here
            return -1;
        }
        
    }
    
}

int Router::selUDPorRaw(char* Buf,int &from,int &portNum, int &strlength)
{
    timeval myclock;
    myclock.tv_sec=15;  //close the router after 15s idle
    myclock.tv_usec=0;

    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(this->rawSocketNumber,&rset);
    FD_SET(this->socketNumber,&rset);
   

    int n=select(max(this->socketNumber,this->rawSocketNumber)+1,&rset,NULL,NULL,&myclock);

    if(n<0)
    {
        printf("Error While Select()\n");
        return -1; 
    }
    else if(n==0)
    {
        //printf("Timeout\n");
        return -1;
    }
    else
    {
        if(FD_ISSET(this->socketNumber, &rset))
        {
            strlength= this->udp_wait_for_msg(Buf,portNum);
            from=0;
            //printf("Receive a packet from port: %d\n",portNum);
            return 0;
        }
        if(FD_ISSET(this->rawSocketNumber,&rset))
        {
            int length=recv(this->rawSocketNumber,Buf,MAX_LENGTH,0);
            if(length<0)
            {
                printf("Receive Error\n");
                return -1;
            }
            from=1;
            strlength=length;
            //printf("Receive a packet from tunnel, the size is %d\n",length);
            return 0;
        }
        else
        {
            //Do nothing here
            return -1;
        }
        
    }
    
}

int Router::selUDP(char * Buf, int &portNum, int &strlength)
{
    timeval myclock;
    myclock.tv_sec=15;  //close the router after 15s idle
    myclock.tv_usec=0;

    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(this->socketNumber,&rset);
   

    int n=select(this->socketNumber+1,&rset,NULL,NULL,&myclock);

    if(n<0)
    {
        printf("Error While Select()\n");
        return -1; 
    }
    else if(n==0)
    {
        //printf("Timeout\n");
        return -1;
    }
    else
    {
        if(FD_ISSET(this->socketNumber, &rset))
        {
            strlength=this->udp_wait_for_msg(Buf,portNum);
            //printf("Receive a packet from port: %d\n",portNum);
            return 0;
        }
        else
        {
            //Do nothing here
            return -1;
        }
        
    }
    
}