#ifndef ROUTER
#define ROUTER

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
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
    }
    void write_to_log(char *);
    void writeln_to_log(char *);
    void open_log(const char *);
    void close_log();
    void close_router();
    void close_tunnel();
    int UDP_start();
    void tunnel_start(char *);

    void udp_msg_send(char *,struct sockaddr*);
    void udp_msg_send_port(char *,int,int);
    int udp_wait_for_msg(char *,int&);
    void tun_msg_send(char *,int);

    int selUDPorTun(char* Buf,int &from,int &portNum, int &strlength);
    int selUDP(char * Buf, int &portNum, int &strlength);

    int getPort() {return portNumber;}
    int getRouterNumber() {return routerNumber;}
private:
    FILE *fp_log;
    int socketNumber;
    int portNumber;
    int routerNumber;
    int tunnelNumber;
    struct sockaddr_in routeraddr;
    bool islogOpen=false;
    bool isSockOpen=false;
};

#endif