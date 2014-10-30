//###################################################################
//# Header file containing functions to get MAC Addr and IP Addr    #
//# for a interface given as a command line argument                #
//###################################################################

char* get_ip_addr(char *iface_name, char ip_addr[], int ip_addr_len)
{
    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];
    struct sockaddr_in *sin; 
    int SockFD;
    int j,k; 


    SockFD = socket(AF_INET, SOCK_DGRAM, 0);


    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;
    if (ioctl(SockFD, SIOCGIFCONF, &ifc) < 0)
    {
        printf("ioctl(SIOCGIFCONF): %m\n");
        return 0;
    }


    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req; ifr < ifend; ifr++)
    {
        if (ifr->ifr_addr.sa_family == AF_INET)
        {
            strncpy(ifreq.ifr_name, ifr->ifr_name,sizeof(ifreq.ifr_name));
            if ((strcmp(ifreq.ifr_name, iface_name)) == 0)
            {
               
                if (ioctl (SockFD, SIOCGIFADDR, &ifreq) < 0)
                {
                    perror("ioctl (SockFD, SIOCGIFADDR, &ifreq)");
                }
                sin = ((struct sockaddr_in *)&ifreq.ifr_addr);
                ip_addr = inet_ntoa(sin->sin_addr);
//                printf("IPAddr : %s\n",ip_addr_loc); 

            }
        }
    }

    return ip_addr;
}


char* get_mac_addr(char *iface_name, char mac_addr[], const int mac_addr_size)
{
    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];
    struct sockaddr_in *sin; 
    char mac_addr_loc[32]; 
    int SockFD;
    int j,k; 


    SockFD = socket(AF_INET, SOCK_DGRAM, 0);


    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;
    if (ioctl(SockFD, SIOCGIFCONF, &ifc) < 0)
    {
        printf("ioctl(SIOCGIFCONF): %m\n");
        return 0;
    }


    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req; ifr < ifend; ifr++)
    {
        if (ifr->ifr_addr.sa_family == AF_INET)
        {
            strncpy(ifreq.ifr_name, ifr->ifr_name,sizeof(ifreq.ifr_name));
            if ((strcmp(ifreq.ifr_name, iface_name)) == 0)
            {
                if (ioctl (SockFD, SIOCGIFHWADDR, &ifreq) < 0)
                {
                    printf("SIOCGIFHWADDR(%s): %m\n", ifreq.ifr_name);
                    return 0;
                }
               
               
                for (j=0, k=0; j<6; j++) 
                {
                    k+=snprintf(mac_addr_loc+k, sizeof(mac_addr_loc)-k-1, j ? ":%02X" : "%02X",
                              (int)(unsigned int)(unsigned char)ifreq.ifr_hwaddr.sa_data[j]);
                }
               
                mac_addr_loc[sizeof(mac_addr_loc)-1]='\0';
               
//                printf("HW_ADDR %s\n",mac_addr_loc); 
            }
        }
    }
    strcpy(mac_addr, mac_addr_loc); 

    return mac_addr;
}

/*
 * Add a routing table entry with ip & netmask with dev as the target.
 * Returns 0 if success, non zero for error.
 */
int 
route_add(const char *ip, const char *netmask, const char *gateway, char *dev)
{
    struct rtentry rt;
    struct sockaddr_in *sinaddr;
    int err;
    int s;
    unsigned short int flags;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if(s < 0)
        return -1;
    
    memset(&rt, 0, sizeof(rt));
    sinaddr = (struct sockaddr_in *) (&rt.rt_dst);
    sinaddr->sin_family = AF_INET;
    if(strcmp(ip, "NULL") != 0)
    {
        if(!inet_aton(ip, &(sinaddr->sin_addr)) )
        {
            return -1;
        }
    }
    else
    {
        #ifdef DEBUG
            printf("IP Addr is NULL\n");
        #endif
    }
    sinaddr = (struct sockaddr_in *) (&rt.rt_genmask);
    sinaddr->sin_family = AF_INET;
    if(strcmp(netmask, "NULL") != 0)
    {
        if(!inet_aton(netmask, &(sinaddr->sin_addr)) )
        {
            return -1;
        }
    }
    else
    {
        #ifdef DEBUG
           printf("Netmask is NULL\n");
        #endif
//        if(!inet_aton("0", &(sinaddr->sin_addr)) )
//        {
//            return -1;
//        }
    }

    sinaddr = (struct sockaddr_in *) (&rt.rt_gateway);
    sinaddr->sin_family = AF_INET;
    if(!inet_aton(gateway, &(sinaddr->sin_addr)) )
    {
        return -1;
    }

    rt.rt_dev = dev;
    flags = RTF_GATEWAY;
    rt.rt_flags = flags;
    err = ioctl(s, SIOCADDRT, &rt);
    close(s);
    if(err < 0)
    {
        return -1;
    }
    return 0;
}



/*
 * Deleting a routing table entry with ip & netmask with dev as the target.
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
    if(strcmp(ip, "NULL") != 0)
    {
        if(!inet_aton(ip, &(sinaddr->sin_addr)) )
        {
            return -1;
        }
    }
    sinaddr = (struct sockaddr_in *) (&rt.rt_genmask);
    sinaddr->sin_family = AF_INET;
    if(strcmp(netmask, "NULL") != 0)
    {
        if(!inet_aton(netmask, &(sinaddr->sin_addr)) )
        {
            return -1;
        }
    }
    if(strcmp(netmask, "NULL") != 0)
    {
      rt.rt_dev = dev;
    }
    err = ioctl(s, SIOCDELRT, (void *)&rt);
    close(s);
    if(err < 0)
        return -1;
    return 0;
}



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


/*
 * Add a arp entry with ip & netmask with dev as the target.
 * Returns 0 if success, non zero for error.
 */
int 
arp_del(const char *ip, const char *mac, const char *netmask, char *dev, int flag)
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
    err = ioctl(s, SIOCDARP, (void *)&ar);
    close(s);
    if(err < 0)
        return -1;
    return 0;
}


