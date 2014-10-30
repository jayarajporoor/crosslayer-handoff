#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
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


    int reqrt_send_sock,offrt_listen_sock;
    int swrt_send_sock,swrt_listen_sock;
    struct sockaddr_in offrt_listen_addr, reqrt_send_addr, client_addr0;
    struct sockaddr_in swrt_send_addr, swrt_listen_addr, client_addr1;
    struct hostent *host, *host1;
    char *ip_addr, *ipaddr1; 

    struct timeval tv1,tv2;
    struct timezone tz1,tz2;

    char recv_data[256];
    char send_data[256];
    char message[256];
    int addr_len, bytes_read;

    int i,j,k; 
    char recv_msg[256];
    char msg_list[5][128]; 
    char comp_msg[16]; 
    char mn_fl_ip_addr[16];
    char msg_ip_addr[16];
    char broadcast_addr[16];
    char ap_host_name[16];
    char mn_fl_mac_addr[32]; 
    char ap_mac_addr[32]; 
    char ap_ip_addr[20];
    char net_mask[20];
    char default_ip_addr[16];

    char radio_ifc_name_old[10];
    char radio_ifc_name[10];
    char radio_ip_addr[16];
    char radio_mac_addr[32];
    char req_bandwidth[10]; 
    char avl_bandwidth[10]; 

    struct pollfd my_sock_fd[2],my_sock_fd1[2]; 
    int poll_result , optval;
    int sleep_time,max_retries,offrt_num_retries,swrt_num_retries;
    int offer_rt_flag,sw_rt_flag;

    optval = 1; 
    offrt_num_retries = 0; 
    swrt_num_retries = 0; 
    offer_rt_flag = 0; 
    sw_rt_flag = 0; 
    strcpy(net_mask,"255.255.255.255");

    if (argc < 6)
    {
        printf("Enter Command line option\n"); 
        printf("Usage: fast_handoff <MN new active radio name> <MN current active radio name> <sleep time in ms> <no of retries>\n");
        exit(-1);
    } else
    {
        strcpy(radio_ifc_name,argv[1]);
        strcpy(radio_ifc_name_old,argv[2]);
        sleep_time = atoi(argv[3]); 
        max_retries = atoi(argv[4]);
        strcpy(mn_fl_ip_addr,argv[5]);

        #ifdef DEBUG
          printf("Max Retries = %d ; Sleep Time = %d",max_retries, sleep_time); 
        #endif
    }

    gettimeofday(&tv1,&tz1);


    //##############################################
    //# Finding IP and MAC address of active Radio #
    //##############################################

    strcpy(radio_ip_addr , get_ip_addr(radio_ifc_name, radio_ip_addr, 
              sizeof(radio_ip_addr))); 
//    strcpy(mn_fl_ip_addr, "192.168.30.100"); 
    strcpy(radio_mac_addr , get_mac_addr(radio_ifc_name, radio_mac_addr, 
              sizeof(radio_mac_addr))); 

    #ifdef DEBUG
        printf("MN %s IP ADDR: %s\nAP %s MAC ADDR: %s\n",radio_ifc_name,
                   mn_fl_ip_addr,radio_ifc_name,radio_mac_addr); 
    #endif
    
    //########################################
    //# Declaring Request Route Send Socket  #
    //########################################

    if ((reqrt_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {
        perror("SocketError");
        exit(1);
    }

    reqrt_send_addr.sin_family = AF_INET;
    reqrt_send_addr.sin_port = htons(9000);
    reqrt_send_addr.sin_addr.s_addr = inet_addr(radio_ip_addr);
    //reqrt_send_addr.sin_addr = *((struct in_addr *)host->h_addr);

    bzero(&(reqrt_send_addr.sin_zero),8);


    addr_len = sizeof(struct sockaddr);

    fflush(stdout);


    if((setsockopt(reqrt_send_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
    {
        perror("Socket setopt Error");
        exit(1);
    }

    if (bind(reqrt_send_sock,(struct sockaddr *)&reqrt_send_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("BindError");
        exit(1);
    }

    if((setsockopt(reqrt_send_sock, SOL_SOCKET, SO_BROADCAST, 
            &optval, sizeof optval)) == -1)
    {
        perror("Socket setopt Broadcast Error");
        exit(1);
    }

    //###########################################
    //# Declaring Request Route Listen Socket	#
    //###########################################

    if ((offrt_listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    ip_addr = "0"; 
    offrt_listen_addr.sin_family = AF_INET;
    offrt_listen_addr.sin_port = htons(9000);
    offrt_listen_addr.sin_addr.s_addr = inet_addr(ip_addr);
    bzero(&(offrt_listen_addr.sin_zero),8);

    if((setsockopt(offrt_listen_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
    {
        perror("Socket setopt Error");
        exit(1);
    }

    if (bind(offrt_listen_sock,(struct sockaddr *)&offrt_listen_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("BindError");
        exit(1);
    }

    //#######################################
    //# Declaring Switch Route Send Socket  #
    //#######################################

        if ((swrt_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        {
            perror("Swith-Route-Send-SocketError");
            exit(1);
        }

        if((setsockopt(swrt_send_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
        {
            perror("SWITCH-ROUTE Socket setopt Error");
            exit(1);
        }

        swrt_send_addr.sin_family = AF_INET;
        swrt_send_addr.sin_port = htons(9001);
//        swrt_send_addr.sin_addr = *((struct in_addr *)host1->h_addr);

        bzero(&(swrt_send_addr.sin_zero),8);


    //#########################################
    //# Declaring Switch Route Listen Socket  #
    //#########################################

        if ((swrt_listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
            perror("MN-Listen-Socket-Error");
            exit(1);
        }

        if((setsockopt(swrt_listen_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
        {
            perror("Socket setopt Error");
            exit(1);
        }

        ip_addr = "0"; 
        swrt_listen_addr.sin_family = AF_INET;
        swrt_listen_addr.sin_port = htons(9001);
        swrt_listen_addr.sin_addr.s_addr = inet_addr(ip_addr);
        bzero(&(swrt_listen_addr.sin_zero),8);


        if (bind(swrt_listen_sock,(struct sockaddr *)&swrt_listen_addr, 
        sizeof(struct sockaddr)) == -1)
        {
        perror("Switch Route Listen-Socket BindError");
        exit(1);
        }


    my_sock_fd[0].fd = offrt_listen_sock; 
    my_sock_fd[0].events = POLLIN;
    my_sock_fd1[0].fd = swrt_listen_sock; 
    my_sock_fd1[0].events = POLLIN;

    //###########################################
    //# Function call to get AP IP Addr  #
    //# and AP MAC Addrfrom the AP name given   #
    //# as cmd line argument                    #
    //###########################################
//    while(1)
    while ((offrt_num_retries <= max_retries) && (swrt_num_retries <= max_retries)
            && ((offer_rt_flag == 0) || (sw_rt_flag == 0)))
    {

    //###########################################
    //# Declaring Broadcast IP address as host address
    //# to send message to
    //###########################################
    //
        if (offer_rt_flag == 0)
//        while((offrt_num_retries <= max_retries) && (offer_rt_flag == 0))
        {
            strcpy(broadcast_addr,"255.255.255.255"); 
            host= (struct hostent *)gethostbyname((char *)broadcast_addr);

            reqrt_send_addr.sin_addr = *((struct in_addr *)host->h_addr);


            strcpy(req_bandwidth,"1000"); 
            sprintf (message, "REQUEST-ROUTE;%s;%s;%s",req_bandwidth,mn_fl_ip_addr,radio_mac_addr);

            strcpy(send_data,message); 

            #ifdef DEBUG
                printf("Sending Message : %s \n",send_data); 
            #endif 
            sendto(reqrt_send_sock, send_data, strlen(send_data), 0,
                  (struct sockaddr *)&reqrt_send_addr, sizeof(struct sockaddr));

            while (((poll_result = poll(my_sock_fd, 1, 200)) <= 0)
                    &&(offrt_num_retries <= max_retries))
            {
                if (poll_result < 0)
                {
                    perror("Poll Error");
                    exit(1);
                }
                else
                {
                    sendto(reqrt_send_sock, send_data, strlen(send_data), 0,
                             (struct sockaddr *)&reqrt_send_addr, sizeof(struct sockaddr));
                    offrt_num_retries += 1; 
                }
            }


            if (my_sock_fd[0].revents & POLLIN == 1)
            {
                bytes_read = recvfrom(offrt_listen_sock,recv_data,256,0,
                    (struct sockaddr *)&client_addr0, &addr_len);
                  

                recv_data[bytes_read] = '\0';

                if (offrt_num_retries > max_retries)
                {
                    printf("Exceeded maximum no of retries \n");
                    return (-1);
                }
                else if ( strcmp((inet_ntoa(client_addr0.sin_addr)),mn_fl_ip_addr) == 0)
                {
                    printf("Receiving our own broadcast\n"); 
                    printf("Receive Addrs is %s\n",inet_ntoa(client_addr0.sin_addr)); 
                    usleep(1000*sleep_time); 
                    offrt_num_retries += 1; 
                }
                else
                {

//                    printf("\n(%s , %d) said : \n",inet_ntoa(client_addr0.sin_addr),
//                                                  ntohs(client_addr0.sin_port));
                    #ifdef DEBUG
                        printf("%s\n", recv_data);
                    #endif 
                    fflush(stdout);


                    j=0;
                    k=0;
                    for (i=0;i<=bytes_read;i++)
                    {
                        if (((recv_data[i] == ';') || (recv_data[i] == '\0')) && (k<4))
                        {
                            recv_msg[j] = '\0'; 
            //              printf("Received Msg is %s \n",recv_msg); 
                            strcpy(msg_list[k],recv_msg);
            //              printf("Messge %d is %s \n",k, msg_list[k]);
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
                    #ifdef DEBUG
                        while ((k<4) && (msg_list[k] != "NULL"))
                        {
//                          printf("Messge %d is %s \n",k, msg_list[k]);
                            k++;
                        }
                    #endif

                    if (strcmp(msg_list[0], "OFFER-ROUTE") == 0)
                    {
                        offrt_num_retries = 0; 
                        offer_rt_flag = 1; 
                        strcpy(avl_bandwidth,msg_list[1]);
                        strcpy(ap_ip_addr,msg_list[2]);
                        strcpy(ap_mac_addr,msg_list[3]);

                        #ifdef DEBUG
                            printf("Deleting Old Route to AP \n");
                            printf("Adding New Route to AP \n");
                        #endif

                        route_del(ap_ip_addr, net_mask, radio_ifc_name); 
                        route_add(ap_ip_addr, net_mask, radio_ip_addr, radio_ifc_name); 
                        #ifdef DEBUG
                            printf("Deleting Old ARP entry for AP \n");
                            printf("Adding New ARP entry for AP \n");
                        #endif
                        arp_del(ap_ip_addr, ap_mac_addr, net_mask, radio_ifc_name,1); 
                        arp_add(ap_ip_addr, ap_mac_addr, net_mask, radio_ifc_name,1); 

                        gettimeofday(&tv2,&tz2);
                        printf("Total Time taken for OFFER-ROUTE is %d microseconds\n",
                        ((tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec)));
//                        exit(0);
                    }
                    else
                    {
                        #ifdef DEBUG
                           printf("BAD MESSAGE\n"); 
                        #endif
                        offrt_num_retries += 1; 
                        usleep(1000*sleep_time); 
                    }
                    
                }     
            }     
        }

        if (offrt_num_retries > max_retries)
        {
            printf("Exceeded maximum no of OFFER-ROUTE retries\n");
            return(-1); 
        }


        while ((swrt_num_retries <= max_retries) && (sw_rt_flag == 0)
                && (offer_rt_flag == 1))
        {

        //###########################################
        //# Declaring AP IP address as host address
        //# to send message 
        //###########################################
        //
            host1= (struct hostent *)gethostbyname((char *)ap_ip_addr);
            printf("AP IP Addr received is %s\n",ap_ip_addr); 

            swrt_send_addr.sin_addr = *((struct in_addr *)host1->h_addr);

            sprintf (message, "SWITCH-ROUTE;%s",mn_fl_ip_addr);

            strcpy(send_data,message); 

            #ifdef DEBUG
                printf("Sending Message : %s \n",send_data); 
            #endif
            sendto(swrt_send_sock, send_data, strlen(send_data), 0,
                  (struct sockaddr *)&swrt_send_addr, sizeof(struct sockaddr));

            strcpy(default_ip_addr,"0.0.0.0"); 

            while (((poll_result = poll(my_sock_fd1, 1, 200)) <= 0)
                    &&(swrt_num_retries <= max_retries))
            {
                if (poll_result < 0)
                {
                    perror("Poll Error");
                    exit(1);
                }
                else
                {
                    swrt_num_retries += 1; 
                    sendto(swrt_send_sock, send_data, strlen(send_data), 0,
                          (struct sockaddr *)&swrt_send_addr, sizeof(struct sockaddr));
                    printf("Num of SWITCH-ROUTE retries is %d\n",swrt_num_retries); 
                }
            }

//            printf("Num of SWITCH-ROUTE retries is %d\n",swrt_num_retries); 

            if (my_sock_fd1[0].revents & POLLIN == 1)
            {
                bytes_read = recvfrom(swrt_listen_sock,recv_data,256,0,
                    (struct sockaddr *)&client_addr1, &addr_len);
                  

                recv_data[bytes_read] = '\0';


//                printf("\n(%s , %d) said : \n",inet_ntoa(client_addr1.sin_addr),
//                                              ntohs(client_addr1.sin_port));
                printf("%s\n", recv_data);
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
                while ((k<4) && (strcmp(msg_list[k],"NULL")))
                {
        //                printf("Messge %d is %s \n",k, msg_list[k]);
                    k++;
                }

                if (strcmp(msg_list[0], "SWITCH-ROUTE-OK") == 0)
                {
                    swrt_num_retries = 0; 
                    sw_rt_flag = 1; 
                    strcpy(msg_ip_addr,msg_list[1]);
                    strcpy(ap_host_name,msg_list[2]);
//		    strcpy(net_mask, "0.0.0.0");

                    printf("Deleting Old Default Route \n");
                    route_del("NULL", "NULL", "NULL"); 

                        printf("Adding New Default Route to AP \n");
                    route_add("NULL", "NULL", ap_ip_addr, radio_ifc_name); 
                    sw_rt_flag = 1; 

                    gettimeofday(&tv2,&tz2);
                    printf("Total Time taken is %d microseconds\n",
                    ((tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec)));
                    exit(0);
                }
                else
                {
                    printf("BAD MESSAGE\n"); 
                    swrt_num_retries += 1; 
                    usleep(1000*sleep_time); 
                }
                
            }
        }
    }
    if (swrt_num_retries > max_retries)
    {
        printf("Exceeded maximum no of SWITCH-ROUTE retries\n");
        return(-2); 
    }

}

