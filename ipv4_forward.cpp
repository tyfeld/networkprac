#include "sysInclude.h"

extern void fwd_LocalRcv(char *pBuffer, int length);
extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);
extern void fwd_DiscardPkt(char *pBuffer, int type);
extern unsigned int getIpv4Address();
static stud_route_msg routeTable[50];
static int table_size = 0;
vector<stud_route_msg> routeTable;

void stud_Route_Init()
{
    routeTable.clear();
}

void stud_route_add(stud_route_msg *proute)
{
    routeTable.push_back(*proute);
}

int stud_fwd_deal(char *pBuffer, int length)
{
    // get destination address
    unsigned int des_addr = ntohl(((unsigned int *)pBuffer)[4]);

    // check TTL
    if (pBuffer[8] == 0)
    {
        ip_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR);
        return 1;
    }
    // if destination is local host
    if (des_addr == getIpv4Address())
    {
        fwd_LocalRcv(pBuffer, length);
        return 0;
    }

    // not local host, then search route table
    /* 
	 * max_mask_len is the maximum of masklen of route table items
	 * who've been successfully matched, and nexthop is to record
	 */
    unsigned int nexthop, max_mask_len = 0;
    for (int i = 0; i < routeTable.size(); ++i)
    {
        unsigned int masklen = ntohl(routeTable[i].masklen);
        if (ntohl(routeTable[i].dest) >> (32 - masklen) == des_addr >> (32 - masklen) && masklen > max_mask_len)
        //如果匹配目的地址，并且此时的masklen大于所记录的最大masklen，记录此时的masklen和下一跳地址
        {
            max_mask_len = masklen;
            nexthop = ntohl(routeTable[i].nexthop);
        }
    }
    // if no route found
    if (max_mask_len == 0)
    {
        fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE);
        return 1;
    }
    // route found, then prepare for sending
    // set TTL
    --pBuffer[8];
    ((unsigned short *)send_buffer)[5] = 0x0;
    unsigned short cks = htons((unsigned short)checksum((unsigned short int *)send_buffer, 10));
    ((unsigned short *)send_buffer)[5] = ~cks;
    fwd_SendtoLower(pBuffer, length, nexthop);

    return 0;
}