
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


int main()
{

    int send_sock,listen_sock;
    struct sockaddr_in listen_addr, send_addr, client_addr;
    struct hostent *host;
    char *ip_addr1; 

    char recv_data[256];
    char send_data[256];
    char message[256];
    int addr_len, bytes_read;

    int i,j,k; 
    char recv_msg[256];
    char msg_list[5][128]; 
    char comp_msg[16]; 
    char mn_fl_ip_addr[16];
    char ap_ip_addr[16];
    char ap_name[20];
    char ap_tunnel_ip[20];
    char ap_tunnel_ifc[20];
    char net_mask[20];

    struct pollfd my_sock_fd[2]; 
    int poll_result , optval;

    optval = 1; 

    //###############################
    //# Declaring AP Listen Socket	#
    //###############################

    if ((listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    ip_addr1 = "0"; 
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(9001);
    listen_addr.sin_addr.s_addr = inet_addr(ip_addr1);
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
    send_addr.sin_port = htons(9002);
    //send_addr.sin_addr = *((struct in_addr *)host->h_addr);

    bzero(&(send_addr.sin_zero),8);


    addr_len = sizeof(struct sockaddr);

    #ifdef DEBUG 
    	printf("\nGW Waiting for SWITCH-ROUTE on port 9001\n");
    #endif

    fflush(stdout);
    while (1)
    {
        while ((poll_result = poll(my_sock_fd, 1, -1)) <= 0)
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
              
            strcpy(ap_ip_addr , inet_ntoa(client_addr.sin_addr));

            recv_data[bytes_read] = '\0'; 

	    #ifdef DEBUG 
           	 printf("Received Data is :%s\n", recv_data);
  	    #endif

            fflush(stdout);


            j=0;
            k=0;
            for (i=0;i<=bytes_read;i++)
            {
                if (((recv_data[i] == ';') || (recv_data[i] == '\0')) && (k<4))
                {
                    recv_msg[j] = '\0'; 
                    strcpy(msg_list[k],recv_msg);
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
                while ((k<4) && (strcmp(msg_list[k],"NULL")))
                {
                    printf("Messge %d is %s \n",k, msg_list[k]);
                    k++;
                }
	     #endif

            strcpy(mn_fl_ip_addr,msg_list[1]);
            strcpy(ap_name,msg_list[2]);

//            printf("Adding Route to MN IP Address\n");
            

    //###########################################
    //# Declaring MN IP address as host address
    //# to send message to
    //###########################################
    //
            host= (struct hostent *)gethostbyname((char *)ap_ip_addr);

            send_addr.sin_addr = *((struct in_addr *)host->h_addr);

    //###########################################
    //# Put in function call to get AP IP Addr  #
    //# and AP MAC Addrfrom the AP name given   #
    //# as message                              #
    //###########################################
            strcpy(ap_tunnel_ifc, ap_name); 
            strcpy(ap_tunnel_ip, get_ip_addr(ap_tunnel_ifc, ap_tunnel_ip, sizeof(ap_tunnel_ip)));
            strcpy(net_mask,"255.255.255.255"); 

            #ifdef DEBUG 
                printf("Deleting Old Route to MN \n");
                printf("Adding New Route to MN \n");
	    #endif

            route_del(mn_fl_ip_addr, net_mask, ap_tunnel_ifc); 
            route_add(mn_fl_ip_addr, net_mask, ap_tunnel_ip, ap_tunnel_ifc); 
            #ifdef DEBUG 
                printf("AP Name is %s \n",ap_name);
	    #endif

            sprintf (message, "SWITCH-ROUTE-OK;%s;%s",mn_fl_ip_addr,ap_name);

            strcpy(send_data,message); 

            strcpy(comp_msg ,"SWITCH-ROUTE"); 

            if (strcmp(msg_list[0],comp_msg)== 0)
            {
	       	#ifdef DEBUG 
               	    printf("Sending Message : %s \n",send_data); 
	    	#endif
                sendto(send_sock, send_data, strlen(send_data), 0,
                (struct sockaddr *)&send_addr, sizeof(struct sockaddr));
            }
        }     
    }

}
