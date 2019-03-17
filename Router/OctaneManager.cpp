#include "OctaneManager.h"
#include "Router.h"
#include "../Datagram/IPDatagram.h"

void OctaneManager::Rule_Install(octane_control* my_octane_hdr)
{//Input: Octane_Control_Message, Insert into Rulelist
    OctaneRule new_Rule;
    new_Rule.ACTION=my_octane_hdr->octane_action;
    new_Rule.PORT=my_octane_hdr->octane_port;
    new_Rule.SIP=my_octane_hdr->octane_source_ip;
    new_Rule.DIP=my_octane_hdr->octane_dest_ip;
    new_Rule.SPORT=my_octane_hdr->octane_source_port;
    new_Rule.DPORT=my_octane_hdr->octane_dest_port;
    new_Rule.PROTOCOL=my_octane_hdr->octane_protocol;
    if(this->RuleList.size()==drop_after)
    {
        new_Rule.ACTION=3;
    }
    this->RuleList.push_back(new_Rule);
    char s_src[20],s_dst[20];
    unsigned int a,b,c,d;
    unsigned int src_port=new_Rule.SPORT;
    unsigned int dst_port=new_Rule.DPORT;
    char newbuf[1024];
    fromIPto4int(ntohl(new_Rule.SIP),a,b,c,d);
    sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
    fromIPto4int(ntohl(new_Rule.SIP),a,b,c,d);
    sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
    sprintf(newbuf,"router: %d, rule installed (%s, %d, %s, %d, %d) action %d\n",m_Router->getRouterNumber(), s_src,src_port, s_dst, dst_port, new_Rule.PROTOCOL, new_Rule.ACTION);
    m_Router->write_to_log(newbuf);
    printf("%s",newbuf);
    return;
}

uint8_t OctaneManager::Rule_Check(iphdr* my_iphdr)
{//Input: IPPacket, Output: Return Action
    return 1;
}

void OctaneManager::set_m_Router(Router *p_Router)
{
    m_Router=p_Router;
    return;
}

