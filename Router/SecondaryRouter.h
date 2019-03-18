#ifndef SECONDARYROUTER
#define SECONDARYROUTER
#include "Router.h"
#include "../timers/timers.hh"
#include "OctaneManager.h"
#include <map>

class SecondaryRouter:public Router
{
public:
    SecondaryRouter(int routernumber):Router(routernumber){
        m_OctaneManager=new OctaneManager;
        m_OctaneManager->set_m_Router(this);
    };
    void start(int);
    int serverPort;
private:
    Timers *m_timerManager;
    std::map<std::pair<uint16_t,uint16_t>,uint32_t> ICMP_ID_SEQ_TO_IP;
};

#endif