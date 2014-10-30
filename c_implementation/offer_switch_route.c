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


int main(int argc,char **argv)
{

    int mn_offrt_send_sock,mn_offrt_listen_sock; 
    int mn_swrt_send_sock,mn_swrt_listen_sock,gw_swrt_send_sock,gw_swrt_listen_sock;
    struct sockaddr_in mn_swrt_listen_addr,mn_swrt_send_addr,client_addr;
    struct sockaddr_in mn_offrt_listen_addr,mn_offrt_send_addr,client_addr1;
    struct sockaddr_in gw_swrt_listen_addr,gw_swrt_send_addr;
    struct hostent *host_gw, *host_mn, *host_mn_offrt;
    char *ip_addr, *ipaddr1; 

    char recv_data0[256];
    char recv_data1[256];
    char recv_data2[256];
    char send_data0[256];
    char send_data1[256];
    char send_data2[256];
    char message[256];
    int addr_len, bytes_read, bytes_read1,bytes_read2;

    int i,j,k; 
    char recv_msg0[256];
    char recv_msg1[256];
    char recv_msg2[256];
    char msg_list0[5][128]; 
    char msg_list1[5][128]; 
    char msg_list2[4][128]; 
    char comp_msg[16]; 
    char comp_msg1[16]; 
    char mn_fl_ip_addr[16];
    char radio_mac_addr[20];
    char req_bandwidth[10]; 
    char avl_bandwidth[10]; 
    char ap_name[16]; 
    char gw_ip_addr[20]; 

    char ap_ifc_name[10];
    char ap_ip_addr[20];
    char ap_mac_addr[32];
    char net_mask[20];

    struct pollfd my_sock_fd[3]; 
    int poll_result , optval;

    optval = 1; 
    strcpy(net_mask,"255.255.255.255");

    if (argc < 4)
    {
        printf("Insufficient Command line parameters\n");
        printf("USAGE: offer_switch_route <AP interface name> <AP Name> <Gateway IP Addr>\n"); 
        return -1;
    }
    else 
    {
        strcpy(ap_ifc_name,argv[1]); 
        strcpy(ap_name,argv[2]); 
        strcpy(gw_ip_addr,argv[3]);


    //###############################
    //# Declaring AP Listen Socket	#
    //###############################

        if ((mn_offrt_listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
            perror("Socket ERROR");
            exit(1);
        }

        ip_addr = "0"; 
        mn_offrt_listen_addr.sin_family = AF_INET;
        mn_offrt_listen_addr.sin_port = htons(9000);
        mn_offrt_listen_addr.sin_addr.s_addr = inet_addr(ip_addr);
        bzero(&(mn_offrt_listen_addr.sin_zero),8);

        if((setsockopt(mn_offrt_listen_sock, SOL_SOCKET, SO_REUSEADDR, 
                &optval, sizeof optval)) == -1)
        {
            perror("Socket setopt Error");
            exit(1);
        }

        if (bind(mn_offrt_listen_sock,(struct sockaddr *)&mn_offrt_listen_addr, sizeof(struct sockaddr)) == -1)
        {
            perror("BindError");
            exit(1);
        }

        my_sock_fd[0].fd = mn_offrt_listen_sock; 
        my_sock_fd[0].events = POLLIN;

        //##########################
        //# Declaring Send Socket  #
        //##########################

        if ((mn_offrt_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        {
            perror("SocketError");
            exit(1);
        }

        mn_offrt_send_addr.sin_family = AF_INET;
        mn_offrt_send_addr.sin_port = htons(9000);
        //mn_offrt_send_addr.sin_addr = *((struct in_addr *)host->h_addr);

        bzero(&(mn_offrt_send_addr.sin_zero),8);


        addr_len = sizeof(struct sockaddr);

        fflush(stdout);


        //###########################################
        //# Function call to get AP IP Addr  #
        //# and AP MAC Addrfrom the AP name given   #
        //# as cmd line argument                    #
        //###########################################

        strcpy(ap_ip_addr , get_ip_addr(ap_ifc_name, ap_ip_addr, sizeof(ap_ip_addr))); 
        strcpy(ap_mac_addr , get_mac_addr(ap_ifc_name, ap_mac_addr, sizeof(ap_mac_addr))); 

#ifdef DEBUG
        printf("AP %s IP ADDR: %s\nAP eth0 MAC ADDR: %s\n",ap_ifc_name,ap_ip_addr, ap_mac_addr); 
        
        printf("\nAP Waiting for REQUEST-ROUTE on port 9000\n");
#endif
    //####################################
    //# Declaring MN Listen Socket       #
    //####################################

        if ((mn_swrt_listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
            perror("MN-SwithRoute-Listen-Socket-Error");
            exit(1);
        }

        if((setsockopt(mn_swrt_listen_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
        {
            perror("Socket setopt Error");
            exit(1);
        }

        ip_addr = "0"; 
        mn_swrt_listen_addr.sin_family = AF_INET;
        mn_swrt_listen_addr.sin_port = htons(9001);
        mn_swrt_listen_addr.sin_addr.s_addr = inet_addr(ip_addr);
        bzero(&(mn_swrt_listen_addr.sin_zero),8);


        if (bind(mn_swrt_listen_sock,(struct sockaddr *)&mn_swrt_listen_addr, 
        sizeof(struct sockaddr)) == -1)
        {
        perror("MN-SwithRoute-Listen-Socket-BindError");
        exit(1);
        }

    //####################################
    //# Set Gateway IP addr as host addr #
    //# to send switch route request     #
    //####################################

        host_gw= (struct hostent *) gethostbyname((char *)gw_ip_addr); 

    //#######################################
    //# Declaring Gateway Send Socket 	#
    //#######################################

        if ((gw_swrt_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        {
            perror("GW-Send-SocketError");
            exit(1);
        }

        if((setsockopt(gw_swrt_send_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
        {
            perror("Socket setopt Error");
            exit(1);
        }

        gw_swrt_send_addr.sin_family = AF_INET;
        gw_swrt_send_addr.sin_port = htons(9001);
        gw_swrt_send_addr.sin_addr = *((struct in_addr *)host_gw->h_addr);

        bzero(&(gw_swrt_send_addr.sin_zero),8);



    //###################################
    //# Declaring GW Listen Socket 	    #
    //###################################

        if ((gw_swrt_listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        {
            perror("GW-Listen-SocketError");
            exit(1);
        }

        if((setsockopt(gw_swrt_listen_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
        {
            perror("Socket setopt Error");
            exit(1);
        }

        gw_swrt_listen_addr.sin_family = AF_INET;
        gw_swrt_listen_addr.sin_port = htons(9002);
        gw_swrt_listen_addr.sin_addr.s_addr = inet_addr(ip_addr);

        bzero(&(gw_swrt_listen_addr.sin_zero),8);
        

        if (bind(gw_swrt_listen_sock,(struct sockaddr *)&gw_swrt_listen_addr, 
            sizeof(struct sockaddr)) == -1)
        {
            perror("GW-Listen-Socket-BindError");
            exit(1);
        }


    //#############################
    //# Declaring MN Send Socket  #
    //#############################

        if ((mn_swrt_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        {
        perror("MN-Send-SocketError");
        exit(1);
        }

        if((setsockopt(mn_swrt_send_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
        {
            perror("Socket setopt Error");
            exit(1);
        }

        mn_swrt_send_addr.sin_family = AF_INET;
        mn_swrt_send_addr.sin_port = htons(9001);
        bzero(&(mn_swrt_send_addr.sin_zero),8);

        addr_len = sizeof(struct sockaddr);

#ifdef DEBUG
        printf("\n Waiting for Receive on port 9001\n");
#endif
        fflush(stdout);
        
        my_sock_fd[1].fd = mn_swrt_listen_sock; 
        my_sock_fd[1].events = POLLIN;

        my_sock_fd[2].fd = gw_swrt_listen_sock; 
        my_sock_fd[2].events = POLLIN;


        while (1)
        {
            while ((poll_result = poll(my_sock_fd, 3, -1)) <= 0)
            {
                if (poll_result < 0)
                {
                    perror("Poll Error");
                    exit(1);
                }
            }

            if (my_sock_fd[0].revents & POLLIN == 1)
            {
                bytes_read = recvfrom(mn_offrt_listen_sock,recv_data0,256,0,
                    (struct sockaddr *)&client_addr, &addr_len);
                  

                recv_data0[bytes_read] = '\0';

#ifdef DEBUG
                    printf("\n(%s , %d) said : ",inet_ntoa(client_addr.sin_addr),
                                                   ntohs(client_addr.sin_port));
#endif
                printf("%s", recv_data0);
                fflush(stdout);


                j=0;
                k=0;
                for (i=0;i<=bytes_read;i++)
                {
                    if (((recv_data0[i] == ';') || (recv_data0[i] == '\0')) && (k<4))
                    {
                        recv_msg0[j] = '\0'; 
            //          printf("Received Msg is %s \n",recv_msg0); 
                        strcpy(msg_list0[k],recv_msg0);
            //          printf("Messge %d is %s \n",k, msg_list0[k]);
                        k++; 
                        j = 0; 
                    }
                    else 
                    {
                        recv_msg0[j] = recv_data0[i];
                        j++;
                    }
                }

                strcpy(msg_list0[k],"NULL");
                k=0;
#ifdef DEBUG
                while ((k<4) && (msg_list0[k] != "NULL"))
                {
                    printf("Messge %d is %s \n",k, msg_list0[k]);
                    k++;
                }
#endif

                strcpy(req_bandwidth,msg_list0[1]);
                strcpy(mn_fl_ip_addr,msg_list0[2]);
                strcpy(radio_mac_addr,msg_list0[3]);

#ifdef DEBUG
                printf("Deleting Old Route to MN \n");
                printf("Adding New Route to MN \n");
#endif
                route_del(mn_fl_ip_addr, net_mask, ap_ifc_name); 
                route_add(mn_fl_ip_addr, net_mask, ap_ip_addr, ap_ifc_name); 
#ifdef DEBUG
                printf("Deleting Old ARP entry for MN \n");
                printf("Adding New ARP entry for MN \n");
#endif
                arp_del(mn_fl_ip_addr, radio_mac_addr, net_mask, ap_ifc_name,1); 
                arp_add(mn_fl_ip_addr, radio_mac_addr, net_mask, ap_ifc_name,1); 
                

        //###########################################
        //# Declaring MN IP address as host address
        //# to send message to
        //###########################################
        //
                host_mn_offrt= (struct hostent *)gethostbyname((char *)mn_fl_ip_addr);

                mn_offrt_send_addr.sin_addr = *((struct in_addr *)host_mn_offrt->h_addr);


    //            sprintf (message, "OFFER-ROUTE;%s;%s;%s",req_bandwidth,AP_IP_ADDR,AP_MAC_ADDR);
                sprintf (message, "OFFER-ROUTE;%s;%s;%s",req_bandwidth,ap_ip_addr,ap_mac_addr);

                strcpy(send_data0,message); 

                strcpy(comp_msg ,"REQUEST-ROUTE"); 

                if (strcmp(msg_list0[0],comp_msg)== 0)
                {
                    printf("Sending Message : %s \n",send_data0); 
                    sendto(mn_offrt_send_sock, send_data0, strlen(send_data0), 0,
                    (struct sockaddr *)&mn_offrt_send_addr, sizeof(struct sockaddr));
                }
            }     
    
            if (my_sock_fd[1].revents & POLLIN == 1)
            {

                bytes_read1 = recvfrom(mn_swrt_listen_sock,recv_data1,256,0,
                (struct sockaddr *)&client_addr1, &addr_len);
                  

                recv_data1[bytes_read1] = '\0';

    //          printf("\n(%s , %d) said : ",inet_ntoa(client_addr.sin_addr),
    //                         ntohs(client_addr.sin_port));
                printf("%s\n", recv_data1);
                fflush(stdout);


                j=0;
                k=0;
#ifdef DEBUG
                printf("Received no of bytes is %d \n",bytes_read1); 
#endif
                for (i=0;i<=bytes_read1;i++)
                {
                    if (((recv_data1[i] == ';') || (recv_data1[i] == '\0')) && (k<3))
                    {
                        recv_msg1[j] = '\0'; 
                        strcpy(msg_list1[k],recv_msg1);
                        k++; 
                        j = 0; 
                    }
                    else
                    {
                        if (recv_data1[i] == '\n')
                        {
                            recv_msg1[j] = '\0'; 
                        } else
                        {
                            recv_msg1[j] = recv_data1[i];
                            j++;
                        }
                    }
                }

                strcpy(msg_list1[k],"NULL");
                k=0;
#ifdef DEBUG
                while ((k<4) && (strcmp(msg_list1[k],"NULL")))
                {
                    printf("Messge %d is %s \n",k, msg_list1[k]);
                    k++;
                }
#endif

                strcpy(mn_fl_ip_addr,msg_list1[1]);

                sprintf (message, "%s;%s;%s",msg_list1[0],mn_fl_ip_addr,ap_name);

                strcpy(send_data1,message); 

                strcpy(comp_msg ,"SWITCH-ROUTE"); 

                if (strcmp(msg_list1[0],comp_msg)== 0)
                {
                    printf("Sending Message : %s \n",send_data1); 
                    sendto(gw_swrt_send_sock, send_data1, strlen(send_data1), 0,
                    (struct sockaddr *)&gw_swrt_send_addr, sizeof(struct sockaddr));
                } else
                    printf("Msg is not SWITCH-ROUTE \n"); 
            }

            if (my_sock_fd[2].revents & POLLIN == 1)
            {
                
            
    //######################################
    // Receiving SWITCH-ROUTE-OK from GW   #
    //######################################

                bytes_read2 = recvfrom(gw_swrt_listen_sock,recv_data1,256,0,
                (struct sockaddr *)&client_addr1, &addr_len);
                      

                recv_data1[bytes_read2] = '\0';

                printf("%s\n", recv_data1);
                fflush(stdout);


                j=0;
                k=0;
                for (i=0;i<=bytes_read2;i++)
                {
                    if (((recv_data1[i] == ';') || (recv_data1[i] == '\0'))
                         && (k<4))
                    {
                        recv_msg2[j] = '\0'; 
                        strcpy(msg_list2[k],recv_msg2);
                        k++; 
                        j = 0; 
                    }
                    else {
                        recv_msg2[j] = recv_data1[i];
                        j++;
                    }
                }

                strcpy(msg_list2[k],"NULL");
                k=0;
#ifdef DEBUG
                while ((k<4) && (strcmp(msg_list2[k],"NULL")!= 0))
                {
                    printf("Messge %d is %s \n",k, msg_list2[k]);
                    k++;
                }
#endif

                strcpy(mn_fl_ip_addr,msg_list2[1]);

    //#########################################	 
    //# Set Mobile Node IP addr as host addr  #
    //# to send SWITCH-ROUTE-OK response	  #
    //#########################################	 

                host_mn = (struct hostent *) gethostbyname( (char *)mn_fl_ip_addr); 

                mn_swrt_send_addr.sin_addr = *((struct in_addr *)host_mn->h_addr);

                strcpy(send_data2,recv_data1); 

                strcpy(comp_msg1 ,"SWITCH-ROUTE-OK"); 

    //#################################
    // Sending SWITCH-ROUTE-OK to MN
    //#################################
                if (strcmp(msg_list2[0],comp_msg1)== 0)
                {
                    printf("Sending Message : %s \n",send_data2); 
                    sendto(mn_swrt_send_sock, send_data2, strlen(send_data2), 0,
                    (struct sockaddr *)&mn_swrt_send_addr, 
                            sizeof(struct sockaddr));
                } else
                    printf("Bad Message\n");

            }
//            poll_result = poll(my_sock_fd, 2, 0);
        }

    }
    return 0; 
}
