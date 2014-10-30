<?php

$text = "Aum Amma \n";


$ap_list = array(
                "EN_LAB1" => array (
                                        "IP" => "192.168.60.1",
                                        "MAC" => "00:0B:6B:0B:4E:C2"),
                "EN_EABC1" => array (
                                        "IP" => "192.168.60.3",
                                        "MAC" => "00:0B:6B:0B:4F:CA"),
                "H1_EAB1" => array (
                                        "IP" => "192.168.60.11",
                                        "MAC" => "00:0B:6B:0B:02:8B"),
                "H1_EABC1" => array (
                                        "IP" => "192.168.60.13",
                                        "MAC" => "00:0B:6B:85:C2:B3")
                        );


($argc > 1) || die("Insufficient parameters\n");

$ap_name = $argv[1]; 

$b_ip_addr = $ap_list[$ap_name]["IP"]; // IP Address of AP - Client interface
$b_mac_addr = $ap_list[$ap_name]["MAC"]; // MAC Address of AP - Client interface


$sock = socket_create(AF_INET,SOCK_DGRAM, SOL_UDP);
if ($sock === false) {
    $errorcode = socket_last_error();
    $errormsg = socket_strerror($errorcode);
    die("Couldn't create socket: [$errorcode] $errormsg");
}

$sock_port = 9000;

$src_addr = "192.168.160.15"; 
$sock_bind = socket_bind($sock,0,$sock_port);
if ($sock_bind === false) {
    $errorcode = socket_last_error();
    $errormsg = socket_strerror($errorcode);
    die("Couldn't bind socket to port $sock_port: [$errorcode] $errormsg");
}


$result = socket_set_option($sock, SOL_SOCKET, SO_REUSEADDR, 1); 
if ($result === false) 
{
    $errorcode = socket_last_error();
    $errormsg = socket_strerror($errorcode);
    die( "Unable to set option on socket:[$errorcode] $errormsg" ); 
}

$from_addr = 0;
$port = 9000;

do 
{
	$sock_recv_len = socket_recvfrom($sock,$buf,65535,0,$from_addr,$port);

	if ($sock_recv_len < 0) 
		echo "Error in receiving from socket \n" ; 
	else
//		echo "received $buf from remote machine\n" ; 

	$rcv_data = $buf;

//	echo "received message from Mobile Node is $rcv_data\n" ; 
	$fields = explode(";",$rcv_data);
	
	$i=0;
	foreach($fields as $f)
	{
//		echo " $i Msg Recvd is $f\n";
		$msg[$i] = $f;
		$i++;
	}
	if ($msg[0] == "REQUEST-ROUTE") {
//		echo "Message 1 $msg[0]\n";
		$req_bandwidth = $msg[1];
		$avl_bandwidth = $req_bandwidth;

		if ($req_bandwidth < $avl_bandwidth) {
//			echo "Request Bandwidth available to client: Bandwidth = $req_bandwidth \n";
		} else {
//			echo "Request Bandwidth not available to client: Available Bandwidth = $avl_bandwidth \n";
		}
	
		$r2_mac_addr = $msg[2];
		$mn_fl_ip_addr = $msg[3];

		$dest_port = 9000; 

//	        echo "Radio2 mac address received is $r2_mac_addr\n";
//	        echo "Floating IP Addr of remote agent is $mn_fl_ip_addr \n";

		$b_ap_interface = "ath0";

		$route_add_call = add_route($mn_fl_ip_addr, $b_ap_interface,$b_ip_addr);
		$arp_add_call = add_arp($mn_fl_ip_addr, $r2_mac_addr);

//		($route_add_call == 0) && ($arp_add_call == 0) || die("System Calls to add route and arp entries failed : Route_Call $route_add_call ARP Call $arp_add_call"); 

		$resp_msg = "OFFER-ROUTE;$avl_bandwidth;$b_ip_addr;$b_mac_addr";
		$resp_len = strlen($resp_msg);
//		echo "Length of Response is $resp_len\n"; 
//		echo "Message Response sent is \n$resp_msg\n"; 


		socket_sendto($sock,$resp_msg,$resp_len,0,$mn_fl_ip_addr,$dest_port);


	} else {
		echo "Message 1 not received correctly \n";
	}

//	socket_close($sock);

} while(true); 

function add_arp ($ip_addr, $mac_addr)
{
//	echo "Adding new Arp entry to AP\n"; 
//	echo "system(\"arp -s $ip_addr $mac_addr\")\n";
	system("arp -s $ip_addr $mac_addr",$ret_val);
	return $ret_val; 
}

function add_route ($ip_addr, $ap_iface,$ap_ip_addr)
{
//	echo "Adding new route to AP\n"; 
//	echo "system(\"route add $ip_addr/32  dev $ap_iface gw $ap_ip_addr\")\n";
	system("route add $ip_addr/32  dev $ap_iface gw $ap_ip_addr",$ret_val);
	 system("echo 0 > /proc/sys/net/ipv4/route/flush");
        system("ip route show cache");

	return $ret_val; 
}

?>
