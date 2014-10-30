#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>

#include <unistd.h>
#include <stdio.h>
#define MAX_IFS 64

int main (int argc, char **argv)
{
    char ifc_name[16];
    char hw_addr[32]; 
    char *hw_addr_new; 

    char *get_mac_addr(char *iface_name);

    if (argc <2)
    {
        printf("Enter Interface name as argument\n");
        exit(1);
    }
    else
    {
        strcpy(ifc_name,argv[1]); 
//        strcpy(hw_addr,get_mac_addr(ifc_name));
        hw_addr_new = get_mac_addr(ifc_name); 
        printf("The HW Addr of interface %s is %s\n",ifc_name, hw_addr_new); 
    }
    return 0;
}


char* get_mac_addr(char *iface_name)
{
    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];
    struct sockaddr_in *sin; 
    int SockFD;
    char *ip_addr; 
    char mac_addr[32];
    char *hw_addr_loc;
//    char ifc_name[16]; 
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
                    k+=snprintf(mac_addr+k, sizeof(mac_addr)-k-1, j ? ":%02X" : "%02X",
                              (int)(unsigned int)(unsigned char)ifreq.ifr_hwaddr.sa_data[j]);
                }
               
                mac_addr[sizeof(mac_addr)-1]='\0';
               
/*
                if (ioctl (SockFD, SIOCGIFADDR, &ifreq) < 0)
                {
                    perror("ioctl (SockFD, SIOCGIFADDR, &ifreq)");
                }
                sin = ((struct sockaddr_in *)&ifreq.ifr_addr);
                ip_addr = inet_ntoa(sin->sin_addr);
                printf("IPAddr : %s\n",ip_addr); 
*/               
                printf("HWAddr : %s\n",mac_addr); 
            }
        }
    }

    hw_addr_loc = mac_addr;
//    return mac_addr;
    return hw_addr_loc;
}

