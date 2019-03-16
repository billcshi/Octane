#ifndef IPDATAGRAM
#define IPDATAGRAM
#include "ICMPDatagram.h"

class IPDatagram
{
public:
    unsigned char VerAndHead; //8bits
    unsigned char ser_field=0; //8bits
    unsigned char total_length[2]; //16bits
    unsigned char id[2]; //16bits
    unsigned char Flag[2]; //16bits
    unsigned char TTL;//8bits
    unsigned char Protocol;//8bits
    unsigned char HeaderChecksum[2]; //16bits
    unsigned char src[4]; //32bits
    unsigned char dst[4]; //32bits
    class ICMPDatagram * ICMP;
    IPDatagram(char *inputBuf){
        VerAndHead=inputBuf[0];
        ser_field=inputBuf[1];
        total_length[0]=inputBuf[2];
        total_length[1]=inputBuf[3];
        id[0]=inputBuf[4];
        id[1]=inputBuf[5];
        Flag[0]=inputBuf[6];
        Flag[1]=inputBuf[7];
        TTL=inputBuf[8];
        Protocol=inputBuf[9];
        HeaderChecksum[0]=inputBuf[10];
        HeaderChecksum[1]=inputBuf[11];
        src[0]=inputBuf[12];
        src[1]=inputBuf[13];
        src[2]=inputBuf[14];
        src[3]=inputBuf[15];
        dst[0]=inputBuf[16];
        dst[1]=inputBuf[17];
        dst[2]=inputBuf[18];
        dst[3]=inputBuf[19];
    }
};

void fromIPto4int(unsigned int IP,unsigned int &a,unsigned int &b,unsigned int &c,unsigned int &d);



#endif