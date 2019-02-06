#ifndef ROUTER
#define ROUTER

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

class Router{
public:
    int portNumber;
    int socketNumber;
    struct sockaddr_in routeraddr;
    Router()
    {
        UDP_start();
    }
    ~Router() 
    {
            if (islogOpen)
            {
                close_log();
            }
            if (isSockOpen)
            {
                shutdown(socketNumber,SHUT_RDWR);
            }
    }
    void write_to_log(char *);
    void writeln_to_log(char *);
    void open_log(const char *);
    void close_log();
    int UDP_start();
private:
    FILE *fp_log;
    bool islogOpen=false;
    bool isSockOpen=false;
};

#endif