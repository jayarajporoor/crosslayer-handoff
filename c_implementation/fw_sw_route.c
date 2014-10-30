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

int main(int argc,char **argv)
{
    #define AP_IP_ADDR "192.168.60.11"
    #define GW_IP_ADDR "192.168.60.11"
    #define AP_MAC_ADDR "00:0B:6B:0B:02:8B"

    int mn_send_sock,mn_listen_sock,gw_send_sock,gw_listen_sock;
    struct sockaddr_in mn_listen_addr,mn_send_addr,client_addr;
    struct sockaddr_in gw_listen_addr,gw_send_addr;
    struct hostent *host_gw, *host_mn;
    char *ip_addr, *ipaddr1; 

    char recv_data[256];
    char recv_data1[256];
    char send_data[256];
    char send_data1[256];
    char message[256];
    int addr_len, bytes_read1, bytes_read;

    int i,j,k; 
    char recv_msg[256];
    char recv_msg1[256];
    char msg_list[5][128]; 
    char msg_list1[4][128]; 
    char comp_msg[16]; 
    char comp_msg1[16]; 
    char mn_fl_ip_addr[16];
    char radio_mac_addr[20];
    char req_bandwidth[10]; 
    char avl_bandwidth[10]; 
    char *ap_name; 
    char *gw_ip_addr; 

    struct pollfd my_sock_fd[2]; 
    int poll_result , optval;

    if (argc < 3)
    {
        printf("Insufficient Command line parameters\n");
        return -1;
    }
    else 
    {
        gw_ip_addr = argv[1];
        ap_name = argv[2]; 
        optval = 1; 


    //####################################
    //# Declaring MN Listen Socket       #
    //####################################

        if ((mn_listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
            perror("MN-Listen-Socket-Error");
            exit(1);
        }

        if((setsockopt(mn_listen_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
        {
            perror("Socket setopt Error");
            exit(1);
        }

        ip_addr = "0"; 
        mn_listen_addr.sin_family = AF_INET;
        mn_listen_addr.sin_port = htons(9001);
        mn_listen_addr.sin_addr.s_addr = inet_addr(ip_addr);
        bzero(&(mn_listen_addr.sin_zero),8);


        if (bind(mn_listen_sock,(struct sockaddr *)&mn_listen_addr, 
        sizeof(struct sockaddr)) == -1)
        {
        perror("MN-Listen-Socket-BindError");
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

        if ((gw_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        {
            perror("GW-Send-SocketError");
            exit(1);
        }

        if((setsockopt(gw_send_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
        {
            perror("Socket setopt Error");
            exit(1);
        }

        gw_send_addr.sin_family = AF_INET;
        gw_send_addr.sin_port = htons(9001);
        gw_send_addr.sin_addr = *((struct in_addr *)host_gw->h_addr);

        bzero(&(gw_send_addr.sin_zero),8);



    //###################################
    //# Declaring GW Listen Socket 	    #
    //###################################

        if ((gw_listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        {
            perror("GW-Listen-SocketError");
            exit(1);
        }

        if((setsockopt(gw_listen_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
        {
            perror("Socket setopt Error");
            exit(1);
        }

        gw_listen_addr.sin_family = AF_INET;
        gw_listen_addr.sin_port = htons(9002);
        gw_listen_addr.sin_addr.s_addr = inet_addr(ip_addr);

        bzero(&(gw_listen_addr.sin_zero),8);
        

        if (bind(gw_listen_sock,(struct sockaddr *)&gw_listen_addr, 
            sizeof(struct sockaddr)) == -1)
        {
            perror("GW-Listen-Socket-BindError");
            exit(1);
        }


    //#############################
    //# Declaring MN Send Socket  #
    //#############################

        if ((mn_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        {
        perror("MN-Send-SocketError");
        exit(1);
        }

        if((setsockopt(mn_send_sock, SOL_SOCKET, SO_REUSEADDR, 
            &optval, sizeof optval)) == -1)
        {
            perror("Socket setopt Error");
            exit(1);
        }

        mn_send_addr.sin_family = AF_INET;
        mn_send_addr.sin_port = htons(9001);
        bzero(&(mn_send_addr.sin_zero),8);

        addr_len = sizeof(struct sockaddr);

        printf("\n Waiting for Receive on port 9001\n");
        fflush(stdout);
        
        my_sock_fd[0].fd = mn_listen_sock; 
        my_sock_fd[0].events = POLLIN;

        my_sock_fd[1].fd = gw_listen_sock; 
        my_sock_fd[1].events = POLLIN;


        while (1)
        {
            while ((poll_result = poll(my_sock_fd, 2, 0)) <= 0)
            {
                if (poll_result < 0)
                {
                    perror("Poll Error");
                    exit(1);
                }
            }

            if (my_sock_fd[0].revents & POLLIN == 1)
            {

                bytes_read = recvfrom(mn_listen_sock,recv_data,256,0,
                (struct sockaddr *)&client_addr, &addr_len);
                  

                recv_data[bytes_read] = '\0';

    //          printf("\n(%s , %d) said : ",inet_ntoa(client_addr.sin_addr),
    //                         ntohs(client_addr.sin_port));
                printf("%s\n", recv_data);
                fflush(stdout);


                j=0;
                k=0;
                printf("Received no of bytes is %d \n",bytes_read); 
                for (i=0;i<=bytes_read;i++)
                {
                    if (((recv_data[i] == ';') || (recv_data[i] == '\0')) && (k<3))
                    {
                        recv_msg[j] = '\0'; 
                        strcpy(msg_list[k],recv_msg);
                        k++; 
                        j = 0; 
                    }
                    else
                    {
                        if (recv_data[i] == '\n')
                        {
                            recv_msg[j] = '\0'; 
                        } else
                        {
                            recv_msg[j] = recv_data[i];
                            j++;
                        }
                    }
                }

                strcpy(msg_list[k],"NULL");
                k=0;
                while ((k<4) && (strcmp(msg_list[k],"NULL")))
                {
                    printf("Messge %d is %s \n",k, msg_list[k]);
                    k++;
                }

                strcpy(mn_fl_ip_addr,msg_list[1]);

                sprintf (message, "%s;%s;%s",msg_list[0],mn_fl_ip_addr,ap_name);

                strcpy(send_data,message); 

                strcpy(comp_msg ,"SWITCH-ROUTE"); 

                if (strcmp(msg_list[0],comp_msg)== 0)
                {
                    printf("Sending Message : %s \n",send_data); 
                    sendto(gw_send_sock, send_data, strlen(send_data), 0,
                    (struct sockaddr *)&gw_send_addr, sizeof(struct sockaddr));
                } else
                    printf("Msg is not SWITCH-ROUTE \n"); 
            }

            if (my_sock_fd[1].revents & POLLIN == 1)
            {
                
            
    //######################################
    // Receiving SWITCH-ROUTE-OK from GW   #
    //######################################

                bytes_read1 = recvfrom(gw_listen_sock,recv_data1,256,0,
                (struct sockaddr *)&client_addr, &addr_len);
                      

                recv_data1[bytes_read1] = '\0';

                printf("%s\n", recv_data1);
                fflush(stdout);


                j=0;
                k=0;
                for (i=0;i<=bytes_read1;i++)
                {
                    if (((recv_data1[i] == ';') || (recv_data1[i] == '\0'))
                         && (k<4))
                    {
                        recv_msg1[j] = '\0'; 
                        strcpy(msg_list1[k],recv_msg1);
                        k++; 
                        j = 0; 
                    }
                    else {
                        recv_msg1[j] = recv_data1[i];
                        j++;
                    }
                }

                strcpy(msg_list1[k],"NULL");
                k=0;
                while ((k<4) && (strcmp(msg_list1[k],"NULL")!= 0))
                {
                    printf("Messge %d is %s \n",k, msg_list1[k]);
                    k++;
                }

                strcpy(mn_fl_ip_addr,msg_list1[1]);

    //#########################################	 
    //# Set Mobile Node IP addr as host addr  #
    //# to send SWITCH-ROUTE-OK response	  #
    //#########################################	 

                host_mn = (struct hostent *) gethostbyname( (char *)mn_fl_ip_addr); 

                mn_send_addr.sin_addr = *((struct in_addr *)host_mn->h_addr);

                strcpy(send_data1,recv_data1); 

                strcpy(comp_msg1 ,"SWITCH-ROUTE-OK"); 

    //#################################
    // Sending SWITCH-ROUTE-OK to MN
    //#################################
                if (strcmp(msg_list1[0],comp_msg1)== 0)
                {
                    printf("Sending Message : %s \n",send_data1); 
                    sendto(mn_send_sock, send_data1, strlen(send_data1), 0,
                    (struct sockaddr *)&mn_send_addr, 
                            sizeof(struct sockaddr));
                } else
                    printf("Bad Message\n");

            }
//            poll_result = poll(my_sock_fd, 2, 0);
        }

    }
    return 0; 
}
