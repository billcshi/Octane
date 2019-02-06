#include "Router.h"

int Router::UDP_start()
{
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
    fprintf(fp_log,"%s",outputStr);
}

void Router::writeln_to_log(char *outputStr)
{
    fprintf(fp_log,"%s\n",outputStr);
}