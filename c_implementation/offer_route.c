#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <string.h>
#include <net/route.h>
#include <net/if_arp.h>

#define MAX_IFS 64

#include "get_addr.h" 

int main(int argc, char *argv[])
{


    int send_sock,listen_sock;
    struct sockaddr_in listen_addr, send_addr, client_addr;
    struct hostent *host;
    char *ip_addr, *ipaddr1; 

    char recv_data[256];
    char send_data[256];
    char message[256];
    int addr_len, bytes_read;

    int i,j,k; 
    char recv_msg[256];
    char msg_list[5][128]; 
    char comp_msg[16]; 
    char mn_fl_ip_addr[16];
    char ap_name[16];
    char ap_ifc_name[10];
    char ap_mac_addr[32]; 
    char ap_ip_addr[20];
    char net_mask[20];

    char radio_mac_addr[32];
    char req_bandwidth[10]; 
    char avl_bandwidth[10]; 

    struct pollfd my_sock_fd[2]; 
    int poll_result , optval;

    optval = 1; 
    strcpy(net_mask,"255.255.255.255");

    if (argc < 2)
    {
        printf("Enter Command line option\n"); 
        printf("Usage: offer_route <MN connection interface_name>\n");
        exit(1);
    } else
    {
        strcpy(ap_ifc_name,argv[1]);
    }

    //###############################
    //# Declaring AP Listen Socket	#
    //###############################

    if ((listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    ip_addr = "0"; 
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(9000);
    listen_addr.sin_addr.s_addr = inet_addr(ip_addr);
    bzero(&(listen_addr.sin_zero),8);

    if((setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
    {
        perror("Socket setopt Error");
        exit(1);
    }

    if (bind(listen_sock,(struct sockaddr *)&listen_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("BindError");
        exit(1);
    }

    my_sock_fd[0].fd = listen_sock; 
    my_sock_fd[0].events = POLLIN;

    //##########################
    //# Declaring Send Socket  #
    //##########################

    if ((send_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {
        perror("SocketError");
        exit(1);
    }

    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(9000);
    //send_addr.sin_addr = *((struct in_addr *)host->h_addr);

    bzero(&(send_addr.sin_zero),8);


    addr_len = sizeof(struct sockaddr);

    fflush(stdout);


    //###########################################
    //# Function call to get AP IP Addr  #
    //# and AP MAC Addrfrom the AP name given   #
    //# as cmd line argument                    #
    //###########################################

    strcpy(ap_ip_addr , get_ip_addr(ap_ifc_name, ap_ip_addr, sizeof(ap_ip_addr))); 
    strcpy(ap_mac_addr , get_mac_addr(ap_ifc_name, ap_mac_addr, sizeof(ap_mac_addr))); 

    printf("AP eth0 IP ADDR: %s\nAP eth0 MAC ADDR: %s\n",ap_ip_addr, ap_mac_addr); 
    
    printf("\nAP Waiting for REQUEST-ROUTE on port 9000\n");
    while (1)
    {
        while ((poll_result = poll(my_sock_fd, 1, 0)) <= 0)
        {
            if (poll_result < 0)
            {
                perror("Poll Error");
                exit(1);
            }
        }


        if (my_sock_fd[0].revents & POLLIN == 1)
        {
            bytes_read = recvfrom(listen_sock,recv_data,256,0,
                (struct sockaddr *)&client_addr, &addr_len);
              

            recv_data[bytes_read] = '\0';

                printf("\n(%s , %d) said : ",inet_ntoa(client_addr.sin_addr),
                                               ntohs(client_addr.sin_port));
            printf("%s", recv_data);
            fflush(stdout);


            j=0;
            k=0;
            for (i=0;i<=bytes_read;i++)
            {
                if (((recv_data[i] == ';') || (recv_data[i] == '\0')) && (k<4))
                {
                    recv_msg[j] = '\0'; 
        //          printf("Received Msg is %s \n",recv_msg); 
                    strcpy(msg_list[k],recv_msg);
        //          printf("Messge %d is %s \n",k, msg_list[k]);
                    k++; 
                    j = 0; 
                }
                else 
                {
                    recv_msg[j] = recv_data[i];
                    j++;
                }
            }

            strcpy(msg_list[k],"NULL");
            k=0;
            while ((k<4) && (msg_list[k] != "NULL"))
            {
                printf("Messge %d is %s \n",k, msg_list[k]);
                k++;
            }

            strcpy(req_bandwidth,msg_list[1]);
            strcpy(mn_fl_ip_addr,msg_list[2]);
            strcpy(radio_mac_addr,msg_list[3]);

            printf("Deleting Old Route to MN \n");
            route_del(mn_fl_ip_addr, net_mask, ap_ifc_name); 
            printf("Adding New Route to MN \n");
            route_add(mn_fl_ip_addr, net_mask, ap_ip_addr, ap_ifc_name); 
            printf("Deleting Old ARP entry for MN \n");
            arp_del(mn_fl_ip_addr, radio_mac_addr, net_mask, ap_ifc_name,1); 
            printf("Adding New ARP entry for MN \n");
            arp_add(mn_fl_ip_addr, radio_mac_addr, net_mask, ap_ifc_name,1); 
            

    //###########################################
    //# Declaring MN IP address as host address
    //# to send message to
    //###########################################
    //
            host= (struct hostent *)gethostbyname((char *)mn_fl_ip_addr);

            send_addr.sin_addr = *((struct in_addr *)host->h_addr);


//            sprintf (message, "OFFER-ROUTE;%s;%s;%s",req_bandwidth,AP_IP_ADDR,AP_MAC_ADDR);
            sprintf (message, "OFFER-ROUTE;%s;%s;%s",req_bandwidth,ap_ip_addr,ap_mac_addr);

            strcpy(send_data,message); 

            strcpy(comp_msg ,"REQUEST-ROUTE"); 

            if (strcmp(msg_list[0],comp_msg)== 0)
            {
                printf("Sending Message : %s \n",send_data); 
                sendto(send_sock, send_data, strlen(send_data), 0,
                (struct sockaddr *)&send_addr, sizeof(struct sockaddr));
            }
        }     
    }

}

