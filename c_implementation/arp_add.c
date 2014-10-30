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
#include <net/if_arp.h>
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
 * Add a arp entry with ip & netmask with dev as the target.
 * Returns 0 if success, non zero for error.
 */
int 
arp_add(const char *ip, const char *mac, const char *netmask, char *dev, int flag)
{
    struct arpreq ar;
    struct sockaddr_in *sinaddr;
    int err;
    int s;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if(s < 0)
        return -1;
    
    memset(&ar, 0, sizeof(ar));
    sinaddr = (struct sockaddr_in *) (&ar.arp_pa);
    sinaddr->sin_family = AF_INET;
    if(!inet_aton(ip, &(sinaddr->sin_addr)) )
    {
        return -1;
    }
    sinaddr = (struct sockaddr_in *) (&ar.arp_ha);
    sinaddr->sin_family = AF_INET;
    if(!inet_aton(mac, &(sinaddr->sin_addr)) )
    {
        return -1;
    }
    sinaddr = (struct sockaddr_in *) (&ar.arp_netmask);
    sinaddr->sin_family = AF_INET;
    if(!inet_aton(netmask, &(sinaddr->sin_addr)) )
    {
        return -1;
    }
    strcpy(ar.arp_dev,dev);
    ar.arp_flags = flag;
    err = ioctl(s, SIOCSARP, (void *)&ar);
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
    
    x = arp_add("192.168.30.100", "00:1d:09:1d:41:b8", "255.255.255.0", "eth0",ATF_PERM);

    if (x <1)
    {
         perror("arp_add");
    }
    gettimeofday(&t2, NULL);

    d = t2.tv_usec - t1.tv_usec;

    printf("%ld\n", d);
}

#endif
