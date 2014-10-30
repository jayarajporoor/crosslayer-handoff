<?php

$text = "Aum Amma \n";
$usleep_time = 1000000;
$max_timeout = 5;
$timer = 0; 

$socket_port = 9001;
//$src_addr = "192.168.160.15"; 
$src_addr = 0; 
$from_addr = 0;

$time_out_flag = 1; 

$socket = sock_create();
$sock_bind_val = sock_binder($socket, $socket_port, $src_addr);

($argc >= 3) || die("Insufficient parameters\n");

do
{
$timer = 0; 
do 
{
	socket_recvfrom($socket,$buf,65535,0,$from_addr,$socket_port);
	//echo "received $buf from remote machine\n";

	$rcv_data = $buf;

//	echo "received message from remote machine is $rcv_data\n" ; 
	$fields = explode(";",$rcv_data);

	$i=0;
	foreach($fields as $f)
	{
//		echo " $i Msg Recvd is $f\n";
		$msg[$i] = $f;
		$i++;
	}
	if ($msg[0] == "SWITCH-INBOUND-ROUTE") 
	{
//		echo "Message 1 received correctly \n";

		$mn_fl_ip_addr = $msg[1];

		($argc >= 3) || die("Insufficient parameters");
		$gw_ip_addr = $argv[1];
		$dest_port = 9001; 
		$ap_name = $argv[2];
//	        echo "Access Point Name is $ap_name\n";
//		echo "Gateway IP Addr is $gw_ip_addr \n";

		$gw_msg = "SWITCH-INBOUND-ROUTE;$ap_name;$mn_fl_ip_addr";
		$gw_msg_len = strlen($gw_msg);
//		echo "Length of gateway message is $gw_msg_len\n"; 
		socket_sendto($socket,$gw_msg,$gw_msg_len,0,$gw_ip_addr,$dest_port);

		$sock_recv_status = 0;
		while (($sock_recv_status < 1) && ($timer < $max_timeout))
		{
			$sock_recv_status = socket_recvfrom($socket,$buf,65535,0,$gw_ip_addr,$socket_port);
			usleep($usleep_time); 
			$timer += $usleep_time;
//			echo "Socket receive length = $sock_recv_status \n";
//			echo "Elapsed Time is $timer \n";
		}
		
		if ($sock_recv_status < 1) 
		{
			$time_out_flag = 1;
//			echo "Timeout Occured waiting for response from Gateway  : Timer Value is $timer \n"; 
		} else {
			$time_out_flag = 0;
		}
	
	} else 
	{
//		echo "Message 1 not received correctly \n";
	} 
//	echo "Value of TimeOut Flag is $time_out_flag \n"; 
} while ($time_out_flag == 1); 

if ($time_out_flag == 0)
{
	$route_ch_resp = $buf;
//	echo "Received Route change response from gateway is $route_ch_resp \n" ; 
	$msg_len = strlen($route_ch_resp);

//	echo "Sending message to $mn_fl_ip_addr \n";
//	echo "Sending to port $dest_port \n";
//	echo "Sending message $route_ch_resp \n";

	$send_status = socket_sendto($socket,$route_ch_resp,$msg_len,0,$mn_fl_ip_addr,$dest_port);

	if ($send_status < 0)
	{
//		echo "Send Message to MN failed with error \n";
	}
	else
	{
		echo "Send Message to MN success \n"; 
	}

}

//socket_close($socket);

} while (true); 

function add_arp ($ip_addr, $mac_addr)
{
//	echo "Adding new Arp entry to AP\n"; 
//	echo "system(\"arp -s $ip_addr $mac_addr\")\n";
}

function add_route ($ip_addr, $ap_iface,$ap_ip_addr)
{
//	echo "Adding new route to AP\n"; 
//	echo "system(\"route add $ip_addr/32 dev $ap_iface gw $ap_ip_addr\")\n";
}

function sock_create()
{
	$sock = socket_create(AF_INET,SOCK_DGRAM, SOL_UDP);
	if ($sock === false) {
	    $errorcode = socket_last_error();
	    $errormsg = socket_strerror($errorcode);
	    die("Couldn't create socket: [$errorcode] $errormsg");
	}
	return $sock;
}

function sock_binder($socket_loc, $sock_port_loc, $src_addr_loc)
{
	$sock_bind = socket_bind($socket_loc,$src_addr_loc,$sock_port_loc);
	if ($sock_bind === false) {
	    $errorcode = socket_last_error();
	    $errormsg = socket_strerror($errorcode);
	    die("Couldn't bind socket to port $sock_port: [$errorcode] $errormsg");
	}
	return $sock_bind;
}

?>
