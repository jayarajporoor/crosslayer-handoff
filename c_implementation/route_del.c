/*
 * Copyright (C) 2003 Amrita Innovative Technology Foundation Labs, 
 * Amrita Institutions, Amritapuri, India.
 * Web: http://aitf.amrita.edu.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#if __GLIBC__  >= 2 && __GLIBC_MINOR  >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#endif
#include <netdb.h>
#include <sys/time.h>

/*
 * Add a routing table entry with ip & netmask with dev as the target.
 * Returns 0 if success, non zero for error.
 */
int 
route_del(const char *ip, const char *netmask, char *dev)
{
    struct rtentry rt;
    struct sockaddr_in *sinaddr;
    int err;
    int s;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if(s < 0)
        return -1;
    
    memset(&rt, 0, sizeof(rt));
    sinaddr = (struct sockaddr_in *) (&rt.rt_dst);
    sinaddr->sin_family = AF_INET;
    if(!inet_aton(ip, &(sinaddr->sin_addr)) )
    {
        return -1;
    }
    sinaddr = (struct sockaddr_in *) (&rt.rt_genmask);
    sinaddr->sin_family = AF_INET;
    if(!inet_aton(netmask, &(sinaddr->sin_addr)) )
    {
        return -1;
    }
    rt.rt_dev = dev;
    err = ioctl(s, SIOCDELRT, (void *)&rt);
    close(s);
    if(err < 0)
        return -1;
    return 0;
}

#ifdef UNIT_TEST

void main()
{
    int x;
    long d;
    struct timeval t1, t2;

    gettimeofday(&t1, NULL);
    
    x = route_del("192.168.30.100", "255.255.255.255", "eth0");

    gettimeofday(&t2, NULL);

    d = t2.tv_usec - t1.tv_usec;

    printf("%ld\n", d);
}

#endif
