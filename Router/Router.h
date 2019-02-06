#ifndef ROUTER
#define ROUTER

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

class Router{
public:
    
    Router(int routerNumber)
    {
        this->routerNumber=routerNumber;
        UDP_start();
    }
    ~Router() 
    {
            if (isSockOpen)
            {
                close_router();
            }
            if (islogOpen)
            {
                close_log();
            }
    }
    void write_to_log(char *);
    void writeln_to_log(char *);
    void open_log(const char *);
    void close_log();
    void close_router();
    int UDP_start();

    void udp_msg_send(char *,struct sockaddr*);
    void udp_msg_send_port(char *,int);
    void udp_wait_for_msg(char *,int&);

    int getPort() {return portNumber;}
    int getRouterNumber() {return routerNumber;}
private:
    FILE *fp_log;
    int socketNumber;
    int portNumber;
    int routerNumber;
    struct sockaddr_in routeraddr;
    bool islogOpen=false;
    bool isSockOpen=false;
};

#endif