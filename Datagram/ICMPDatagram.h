#ifndef ICMPDATAGRAM
#define ICMPDATAGRAM

class ICMPDatagram
{
public:
    unsigned char Type; //8bits
    unsigned char Code=0; //8bits
    unsigned char Checksum[4]; //32bits
    unsigned char id[4]; //32bits
    unsigned char seq[4]; //32bits
    unsigned char Timestamp[8];//8bits
    unsigned char Data[1024]; //remain unchange
    ICMPDatagram(char *inputBuf,int length=84){
        Type=inputBuf[0];
        Code=inputBuf[1];
        Checksum[0]=inputBuf[2];
        Checksum[1]=inputBuf[3];
        Checksum[2]=inputBuf[4];
        Checksum[3]=inputBuf[5];
        id[0]=inputBuf[6];
        id[1]=inputBuf[7];
        id[2]=inputBuf[8];
        id[3]=inputBuf[9];
        seq[0]=inputBuf[10];
        seq[1]=inputBuf[11];
        seq[2]=inputBuf[12];
        seq[3]=inputBuf[13];
        Timestamp[0]=inputBuf[14];
        Timestamp[1]=inputBuf[15];
        Timestamp[2]=inputBuf[16];
        Timestamp[3]=inputBuf[17];
        Timestamp[4]=inputBuf[18];
        Timestamp[5]=inputBuf[19];
        Timestamp[6]=inputBuf[20];
        Timestamp[7]=inputBuf[21];
        for(int i=0;i<length-22;i++)
        {
            Data[i]=inputBuf[22+i];
        }
    }
};

#endif