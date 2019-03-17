#ifndef OCTANEMANAGER
#define OCTANEMANAGER
#include <stdint.h>
#include "../Datagram/OctaneMessage.h"
#include <vector>

#include <netinet/ip.h> //For IPhdr
#include <netinet/ip_icmp.h> //For ICMPhdr

class Router;

struct OctaneRule{
    uint32_t SIP;
    uint16_t SPORT;
    uint32_t DIP;
    uint16_t DPORT;
    uint16_t PROTOCOL;
    uint8_t ACTION;
    uint16_t PORT;
};

class OctaneManager{
public:
    OctaneManager(){}
    ~OctaneManager(){}
    void Rule_Install(octane_control *);
    uint8_t Rule_Check(iphdr *);
    void set_m_Router(Router *p_Router);
    int drop_after=-1;
private:
    std::vector<OctaneRule> RuleList;
    Router * m_Router;
};
#endif