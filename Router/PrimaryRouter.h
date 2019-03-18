#ifndef PRIMARYROUTER
#define PRIMARYROUTER
#include "Router.h"
#include "../timers/timers.hh"
#include "OctaneManager.h"
#include <map>

class ReliableTransTimer: public TimerCallback{
public:
    ReliableTransTimer(Router* p_Router, int seqno,int targetport,char *buf,int length):m_Router(p_Router),m_seqno(seqno),target_port(targetport){
        for(int i=0;i<length;i++)
        {
            m_buf[i]=buf[i];
        }
    };
    ~ReliableTransTimer(){};
    int m_seqno;
    char m_buf[1024];
    int length;
    int target_port;
    Router * m_Router;
    int Expire();
};

class IdleTimeMonitor:public TimerCallback{
public:
    IdleTimeMonitor(Router * p_Router):m_Router(p_Router){};
    ~IdleTimeMonitor(){};
    int Expire();
private:
    Router* m_Router;
};

class PrimaryRouter:public Router
{
public:
    PrimaryRouter(int routernumber):Router(routernumber){
        m_OctaneManager=new OctaneManager;
        m_OctaneManager->set_m_Router(this);
    };
    void start(int);
    void startv2();
    int clientPort[5]={};
private:
    Timers *m_timerManager;
    OctaneManager *m_OctaneManager;
    int m_seqno=1;
    std::map<int,handle> seqno_to_handle;

};

#endif