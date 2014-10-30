<?

$msg_send = "SWITCH-INBOUND-ROUTE;$mn_floating_ip";
$ip_tosend = $ap_ip;

//This script is called from (included) the main script handoff.php
//Creates a socket and binds to port 9001 for receiving data. Sends the message "SWITCH-INBOUND-ROUTE;mobile-node-floating-ip" to the AP and waits for acknowledgement.
//If ack is okay,it then changes the default route on the ivtu via the AP

$count=0;
$socket_recv_len=0;
$buf="";

while (($count < MAX_BROADCASTS) && (socket_recv_len < 1))
 {	
	echo "msg_send: $msg_send\n";
	echo "ip_tosend: $ip_tosend\n";
	socket_sendmsg_func($sock_handle_switch_route,$msg_send,$PORT_SWITCH_ROUTE,$ip_tosend);
	
	$elapsed_time=0;

	while(($socket_recv_len < 1) && ($elapsed_time < $total_wait_time))
		{
			$socket_recv_len = socket_lookformsg_func($sock_handle_switch_route, $PORT_SWITCH_ROUTE, $buf);


			if ($socket_recv_len > 1) 	//as soon as the socket receives some data, it jumps out of 2 while loops
				{
				break 2;
				}

			usleep($wait_time); 		//usleep takes parameter in microseconds
			$elapsed_time+=$wait_time;
			echo "elapsed_time= $elapsed_time\n";
		}
	$count++;
	echo "count= $count\n";
//	usleep(100);
 }

	echo "socket_recv_len= $socket_recv_len\n";

//if any message was received on the socket, explode it to receive the acknowledgment
if ($socket_recv_len > 1)
{
	echo "buf: $buf\n";
	$ack= trim($buf);
	($ack == "SWITCH-INBOUND-ROUTE-OK") || die("Ack not okay \n");	

//below line to be commented while testing
//	($retval == 0 || die "route del default failed \n");

//	 echo "system(\"route add default gw $ap_ip \")\n";
     //uncomment while running in ivtu
//	($retval == 0 || die "route add default route failed \n");


 //       echo "system(\"route del -host $ap_ip_old/32 dev $ifc_name_old\")\n";	
 //       echo "system(\"route del default gw $ap_ip_old\")\n";	

	 
	
//usleep(1*1000*1000);
//system("route del default gw $ap_ip_old",$retval);

//	 system("route add default gw $ap_ip dev $ifc_name_entered",$retval); 
	system("ip route change default via $ap_ip dev $ifc_name_entered");
	 system("route del -host $ap_ip_old/32 dev $ifc_name_old ",$retval);
//	system("ip route flush cache");
 	system("echo 0 > /proc/sys/net/ipv4/route/flush");
	echo "showing route cache from switch-inbound\n";
        system("ip route show cache");


//usleep(10*1000*1000);
}

$ts_after_default_route_added = microtime(true);
echo "ts_after_AP_route_added: $ts_after_AP_route_added\n";

//close socket
socket_close($sock_handle_switch_route);

?>
