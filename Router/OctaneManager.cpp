#include "OctaneManager.h"
#include "Router.h"
#include "../Datagram/IPDatagram.h"

#include <iostream>

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
    char newbuf[1500];
    fromIPto4int(ntohl(new_Rule.SIP),a,b,c,d);
    sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
    fromIPto4int(ntohl(new_Rule.DIP),a,b,c,d);
    sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
    uint16_t source_port=htons(new_Rule.SPORT);
    uint16_t dest_port=htons(new_Rule.DPORT);
    sprintf(newbuf,"router: %d, rule installed (%s, %d, %s, %d, %d) action %d\n",m_Router->getRouterNumber(), s_src,source_port, s_dst, dest_port, new_Rule.PROTOCOL, new_Rule.ACTION);
    m_Router->write_to_log(newbuf);
    printf("%s",newbuf);
    return;
}

uint8_t OctaneManager::Rule_Check(iphdr* my_iphdr,uint16_t &port)
{//Input: IPPacket, Output: Return Action
    for(auto i=RuleList.begin();i!=RuleList.end();i++)
    {
        if(my_iphdr->saddr==i->SIP || i->SIP==ANY_IP)
        {
            if(my_iphdr->daddr==i->DIP || i->DIP==ANY_IP)
            {
                if(my_iphdr->protocol==i->PROTOCOL)
                {
                    if(my_iphdr->protocol==1 || true)
                    {//ICMP without port
                        char s_src[20],s_dst[20];
                        char newbuf[1500];
                        unsigned int a,b,c,d;
                        fromIPto4int(ntohl(i->SIP),a,b,c,d);
                        sprintf(s_src,"%d.%d.%d.%d",a,b,c,d);
                        fromIPto4int(ntohl(i->DIP),a,b,c,d);
                        sprintf(s_dst,"%d.%d.%d.%d",a,b,c,d);
                        uint16_t source_port=htons(i->SPORT);
                        uint16_t dest_port=htons(i->DPORT);
                        sprintf(newbuf,"router: %d, rule hit (%s, %d, %s, %d, %d) action %d port %d\n",this->m_Router->getRouterNumber(),s_src,source_port,s_dst,dest_port,i->PROTOCOL,i->ACTION,i->PORT);
                        this->m_Router->write_to_log(newbuf);
                        printf("%s",newbuf);
                        port=i->PORT;
                        return i->ACTION;
                    }
                }
            }
        }
    }
    port=0;
    return NONACTION;
}

void OctaneManager::set_m_Router(Router *p_Router)
{
    m_Router=p_Router;
    return;
}

