#include "sysInclude.h"

using namespace std;

extern void ip_DiscardPkt(char *pBuffer, int type);

extern void ip_SendtoLower(char *pBuffer, int length);

extern void ip_SendtoUp(char *pBuffer, int length);

extern unsigned int getIpv4Address();

unsigned short int checksum(unsigned short int *pBuffer, int length)
{
    //计算校验和
    unsigned int sum = 0;
    for (int i = 0; i < length; i++)
    {
        sum += ntohs(pBuffer[i]);           // calculate the sum
        sum = (sum >> 16) + (sum & 0xffff); // wrap around when overflow }
    }
    return sum;
}

int stud_ip_recv(char *pBuffer, unsigned short length)
{
    //check version
    unsigned int version = (unsigned int)pBuffer[0] >> 4;
    if (version != 4)
    {
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
        return 1;
    }

    //check head length, require 20-60 byte
    unsigned int ihl = ((unsigned int)pBuffer[0]) & 0x0f;
    ihl *= 4;
    if (ihl < 20 || ihl > 60)
    {
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
        return 1;
    }

    //check time to live
    unsigned int ttl = (unsigned int)pBuffer[8];
    if (!ttl)
    {
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
        return 1;
    }

    //checksum
    unsigned int cks = checksum((unsigned short int *)pBuffer, ihl / 2);
    if (cks != 0xffff)
    {
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
        return 1;
    }

    //check this ip
    unsigned int desAddr = ntohl(*(unsigned int *)(&pBuffer[16]));
    if (desAddr != getIpv4Address() && desAddr != 0xffffffff)
    {
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR);
        return 1;
    }

    ip_SendtoUp(pBuffer + ihl, length - ihl);
}

int stud_ip_Upsend(char *pBuffer, unsigned short len, unsigned int srcAddr, unsigned int dstAddr, byte protocol, byte ttl)
{
    char *send_buffer = (char *)malloc(len + 20);
    memcpy(send_buffer + 20, pBuffer, len);
    send_buffer[0] = 0x45;
    send_buffer[1] = 0x0;
    ((unsigned short *)send_buffer)[1] = htons(len + 20);
    ((unsigned int *)send_buffer)[1] = 0x0;
    send_buffer[8] = ttl;
    send_buffer[9] = protocol;
    ((unsigned int *)send_buffer)[3] = htonl(srcAddr);
    ((unsigned int *)send_buffer)[4] = htonl(dstAddr);
    ((unsigned short *)send_buffer)[5] = 0x0;
    unsigned short cks = htons((unsigned short)checksum((unsigned short int *)send_buffer, 10));
    ((unsigned short *)send_buffer)[5] = ~cks;
    ip_SendtoLower(send_buffer, len + 20);
    return 0;
}