#include "Router.h"

#define MAX_LENGTH 1024

int Router::UDP_start()
{
    // this part of code is reused from Steven's book
    this->socketNumber=socket(AF_INET,SOCK_DGRAM,0);
    if(this->socketNumber<0)
    {
        printf("Open Socket Error");
        return -1;
    }
    routeraddr.sin_family = AF_INET;
    routeraddr.sin_addr.s_addr=htonl(INADDR_ANY);
    routeraddr.sin_port=htons(0);

    if(bind(this->socketNumber,(struct sockaddr*) &routeraddr,sizeof(routeraddr))<0)
    {
        printf("Binding Socket Error");
        return -1;
    }

    this->isSockOpen=true;

    struct sockaddr_in localaddr;
    socklen_t addrlen;
    addrlen=sizeof(localaddr);
    getsockname(this->socketNumber,(struct sockaddr*) &localaddr,&addrlen);
    // end of reused part
    this->portNumber=ntohs(localaddr.sin_port);
    return 0;
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
    char buf[40];
    sprintf(buf,"router %d closed\n",routerNumber);
    write_to_log(buf);
}

void Router::udp_msg_send(char *msg, struct sockaddr* dst)
{
    socklen_t len=sizeof(*dst);
    printf("Router %d Sending:%s\n",this->routerNumber,msg);
    sendto(socketNumber, msg, MAX_LENGTH, 0, dst, len);
}

void Router::udp_msg_send_port(char * msg,int portNumber)
{
    
    struct sockaddr_in dst;
    dst.sin_family=AF_INET;
    dst.sin_addr.s_addr=htonl(INADDR_ANY);
    dst.sin_port=htons(portNumber);
    socklen_t len=sizeof(dst);
    printf("Router %d Sending:%s\n",this->routerNumber,msg);
    sendto(socketNumber,msg,MAX_LENGTH,0,(struct sockaddr *) &dst,len);
}

void Router::udp_wait_for_msg(char * msgBuf, int &incomePort)
{
    struct sockaddr_in src_addr; //Record the incoming message source
    socklen_t len=sizeof(src_addr);
    int count;
    count = recvfrom(socketNumber,msgBuf,MAX_LENGTH,0,(struct sockaddr*) &src_addr,&len);
    printf("Router %d Receiving:%s\n",this->routerNumber,msgBuf);
    incomePort=ntohs(src_addr.sin_port);
}