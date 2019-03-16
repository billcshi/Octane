#include "IPDatagram.h"

void fromIPto4int(unsigned int IP,unsigned int &a,unsigned int &b,unsigned int &c,unsigned int &d)
{
    d=IP%0x100;
    IP/=0x100;
    c=IP%0x100;
    IP/=0x100;
    b=IP%0x100;
    IP/=0x100;
    a=IP;
    return;
}