<?php

$text = "Aum Amma \n";

// Port addr in Gateway
$socket_port = 9001;

// Gateway IP Addr
$src_addr = 0; 

#$ap_tunnel_ip_list = array("EN_LAB1" => "192.168.70.2","EN_EABC1" => "192.168.160.15","H1_EAB1" => "192.168.70.6", "H1_EABC1" => "192.168.70.8");

$ap_tunnel_ip_list = array("EN_LAB1" => "192.168.70.2","EN_EABC1" => "192.168.70.3","H1_EAB1" => "192.168.70.6", "H1_EABC1" => "192.168.70.8");

$ap_tunnel_ifc_list = array("EN_LAB1" => "ec1","EN_EABC1" => "ec2","H1_EAB1" => "ec3", "H1_EABC1" => "ec4");
$from_addr = 0;

$socket = sock_create();
$sock_bind_val = sock_binder($socket, $socket_port, $src_addr);
if ($sock_bind_val === false)
	    die("Couldn't bind socket to port $socket_port");

do
{
	socket_recvfrom($socket,$buf,65535,0,$from_addr,$socket_port);


	$rcv_data = $buf;
	echo "received message from remote machine is $rcv_data\n";
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

		$ap_name = $msg[1];
		$mn_fl_ip_addr = $msg[2];
		
		$ap_ip_addr = $ap_tunnel_ip_list[$ap_name];

		$dest_port = 9001; 
//	        echo "Access Point Name is $ap_name\n";
//		echo "Mobile Node IP Addr is $mn_fl_ip_addr \n";
		change_route_gw($ap_name,$mn_fl_ip_addr);
//		($ret_val == 0) || die ("System call route del failed in change_route route_add ");

		$gw_resp_msg = "SWITCH-INBOUND-ROUTE-OK";
		$gw_msg_len = strlen($gw_resp_msg);
//		echo "Length of gateway message is $gw_msg_len\n"; 
		socket_sendto($socket,$gw_resp_msg,$gw_msg_len,0,$ap_ip_addr,$dest_port);
		echo "Gateway Switch Operation Done \n";
	
	} else 
	{
		echo "Message 1 not received correctly \n";
	} 

//socket_close($socket);
} while(true);



function change_route_gw ($ap_name_loc, $mn_fl_ip_addr_loc)
{

global $ap_tunnel_ifc_list;
global $ap_tunnel_ip_list;
	$ap_tunnel_ifc = $ap_tunnel_ifc_list[$ap_name_loc];
	$ap_tunnel_ip = $ap_tunnel_ip_list[$ap_name_loc];
//	echo "AP Tunnel IFC for AP $ap_name_loc is $ap_tunnel_ifc\n";
//	echo "AP Tunnel IP for AP $ap_name_loc is $ap_tunnel_ip\n";
//	echo "Deleting curr route entry for mobile node $mn_fl_ip_addr_loc\n"; 
//	echo "system(\"route del $mn_fl_ip_addr_loc/32\")\n";
//	system("route del $mn_fl_ip_addr_loc/32",$ret_val);
//	($ret_val == 0) || die ("System call route del failed in change_route");

//        echo "Adding new route entry for mobile node ip $mn_fl_ip_addr_loc\n";
//        echo "system(\"route add $mn_fl_ip_addr_loc/32 dev $ap_tunnel_ifc  gw $ap_tunnel_ip\")\n"; 
//        system("route add $mn_fl_ip_addr_loc/32 dev $ap_tunnel_ifc  gw $ap_tunnel_ip"); 

	system("ip route change $mn_fl_ip_addr_loc/32 via $ap_tunnel_ip dev $ap_tunnel_ifc");
 	system("echo 0 > /proc/sys/net/ipv4/route/flush");
	echo "showing cache in gateway\n";
        system("ip route show cache");

	return $ret_val; 
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
