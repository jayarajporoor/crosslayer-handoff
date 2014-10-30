<?

//TIME STAMP
echo "Script Starting\n";
//echo "ts_script_start: $ts_script_start\n";
//$ts_script_start= microtime(true);

//Enter the correct AP names, interface names and the bssid in the associative array below before running in ivtu
$ifc_ip_list = array("eth0" => "192.168.160.11", "eth1" => "192.168.1.125","ath0" => "192.168.45.101", "ath1" => "192.168.46.101");
$ifc_mac_list = array ("eth0" => "00:1D:09:1D:41:D8", "eth1" => "xx:xx:xx:xx:xx:xx", "ath0" => "00:0B:6B:0A:80:CA", "ath1" => "00:0B:6B:0B:4E:96");
$ap_mac_list = array ("EN_LAB1" => "00:0B:6B:0B:4E:C2", "EN_EABC1" => "00:0B:6B:0B:4F:CA", "SUDHAN" => "00:1D:09:21:67:53", "HARI" => "00:1D:09:1D:41:B8", "H1_EABC1" => "00:0B:6B:85:C2:B3", "H1_EAB1" => "00:0B:6B:0B:02:8B");

$ap_ip_list = array ("EN_LAB1" => "192.168.60.1", "EN_EABC1" => "192.168.60.3", "H1_EABC1" => "192.168.60.13", "H1_EAB1" => "192.168.60.11");

//Provide the interface name such as "ath0" as the first argument, AP_name such as "EN_LAB1" etc as the second command line argument after the php file, 3rd argument is again the interface eg. ath1 which is already associated, 4th argument is the AP name already associated with the existing interface eg.EN_LAB2 
($argc > 4) || die ("Provide 4 commandline arguments after php file.Look in to code for commment to find which ones");
//Provide the interface name such as "ath0" as the first argument and the AP_name such as "EN_LAB1" etc as the second command line argument after the php file
($argc > 4) || die ("Provide 4 commandline arguments after php file");
{
	$ifc_name_entered = $argv[1];
	$ap_name_entered = $argv[2];
	$ifc_name_old = $argv[3];
	$ap_name_old = $argv[4];
	$ap_ip_old = $ap_ip_list[$ap_name_old];
	$ifc_ip = $ifc_ip_list[$ifc_name_entered];
	$ifc_mac = $ifc_mac_list[$ifc_name_entered];
	$ap_bssid_entered = $ap_mac_list[$ap_name_entered];

	//echo "ifc_ip: $ifc_ip\n";
	//echo "ifc_mac: $ifc_mac\n";
	//echo "ap_bssid_entered: $ap_bssid_entered\n";
}

$mn_floating_ip="192.168.30.100";
$req_bandwidth=1000; //in kbps
$msg_send = "REQUEST-ROUTE;$req_bandwidth;$ifc_mac;$mn_floating_ip";

$PORT_REQ_ROUTE_BCAST=8998;
$PORT_REQ_ROUTE_RECV=9000;
$PORT_SWITCH_ROUTE=9001;

$IP_BCAST = '255.255.255.255';
$wait_time = 1000;
$total_wait_time = 1000*1000*3; //in seconds

define("MAX_BROADCASTS", "3");

set_time_limit(0); //dont timeout the script

//include "./ap_scan.php";
//****************************************Coped ap_scan.php here - start ***********************************

$ts_before_AP_assoc = microtime(true);
echo "ts_before_AP_assoc: $ts_before_AP_assoc\n";

//Associating with the AP

//$ap_info = switch_ap($ifc_name_entered,$ap_bssid_entered);
switch_ap($ifc_name_entered,$ap_bssid_entered);

do
{
        $ap_info = system("iwconfig $ifc_name_entered | grep \"Access Point\"");
	$ap_info = trim($ap_info);
	$bssid = substr($ap_info, -17);

	if ($bssid == "")
	{
		 $associated = "FALSE";
	}
	else
	{	 
		$associated = "TRUE";
	}


	$ts_AP_assoc_in_loop = microtime(true);
	//echo "ts_AP_assoc_in_loop: $ts_AP_assoc_in_loop\n";
	
	$time_in_assoc_loop = ($ts_AP_assoc_in_loop - $ts_before_AP_assoc);
	//echo "time_in_assoc_loop: $time_in_assoc_loop\n";

} while (($time_in_assoc_loop < 10) && ($associated == "FALSE"));

($bssid == $ap_bssid_entered) || die("Association is not correct\n");

$ts_assoc_completed= microtime(true);
echo "ts_assoc_completed: $ts_assoc_completed\n";

$time_for_ap_assoc = ($ts_assoc_completed - $ts_before_AP_assoc);
echo "time for ap association: $time_for_ap_assoc\n";


//****************************************Copied ap_scan.php here - end ***********************************


//create socket
$sock_handle_req_route_bcast=socket_create(AF_INET,SOCK_DGRAM,0) or die("Could not create socket\n");
socket_set_nonblock($sock_handle_req_route_bcast);

$sock_handle_req_route_recv=socket_create(AF_INET,SOCK_DGRAM,0) or die("Could not create socket\n");
socket_set_nonblock($sock_handle_req_route_recv);

$sock_handle_switch_route=socket_create(AF_INET,SOCK_DGRAM,0) or die("Could not create socket for switch-route\n");
socket_set_nonblock($sock_handle_switch_route);

//to reuse address and port and to get rid of address in use error
if (!socket_set_option($sock_handle_req_route_bcast, SOL_SOCKET, SO_REUSEADDR, 1)) {
    echo socket_strerror(socket_last_error($sock_handle_req_route_bcast));
    exit;
} 

if (!socket_set_option($sock_handle_req_route_recv, SOL_SOCKET, SO_REUSEADDR, 1)) {
    echo socket_strerror(socket_last_error($sock_handle_req_route_recv));
    exit;
} 

if (!socket_set_option($sock_handle_switch_route, SOL_SOCKET, SO_REUSEADDR, 1)) {
    echo socket_strerror(socket_last_error($sock_handle_switch_route));
    exit;
} 

//set option for broadcast
if (!socket_set_option($sock_handle_req_route_bcast, SOL_SOCKET, SO_BROADCAST, 1)) {
    echo socket_strerror(socket_last_error($sock_handle_req_route_bcast));
    exit;
} 

//bind sockets
socket_bind($sock_handle_req_route_bcast,$ifc_ip,$PORT_REQ_ROUTE_BCAST) or die("Could not bind to socket with $PORT_REQ_ROUTE_BCAST\n");
socket_bind($sock_handle_req_route_recv,0,$PORT_REQ_ROUTE_RECV) or die("Could not bind to socket with $PORT_REQ_ROUTE_RECV\n");
socket_bind($sock_handle_switch_route,0,$PORT_SWITCH_ROUTE) or die("Could not bind to socket with $PORT_SWITCH_ROUTE\n");

$count=0;
$socket_recv_len=0;
$buf="";
//echo "buf= $buf\n";


$ts_before_req_route = microtime(true);
echo "ts_before_req_route: $ts_before_req_route\n";


//Request-route
while (($count < MAX_BROADCASTS) && (socket_recv_len < 1))
 {	
	socket_sendmsg_func($sock_handle_req_route_bcast,$msg_send,$PORT_REQ_ROUTE_RECV,$IP_BCAST);	
	$elapsed_time=0;

	while(($socket_recv_len < 1) && ($elapsed_time < $total_wait_time))
		{
			$socket_recv_len = socket_lookformsg_func($sock_handle_req_route_recv, $PORT_REQ_ROUTE_RECV, $buf);


			if ($socket_recv_len > 1) 	//as soon as the socket receives some data, it jumps out of 2 while loops
			{
				break 2;
			}

			usleep($wait_time); 		//usleep takes parameter in microseconds
			$elapsed_time+=$wait_time;
			//echo "elapsed_time= $elapsed_time\n";
			//echo "socket_recv_len= $socket_recv_len\n";
		}
	$count++;
	//echo "count= $count\n";
	usleep(100);
 }

//if any message was received on the socket, explode them to get the bandwidth,mac and ip for AP with the delimiter as semi-colon
if ($socket_recv_len > 1)
{
	$ts_after_req_route_recd = microtime(true);
	echo "ts_after_req_route_recd: $ts_after_req_route_recd\n";

	$time_to_receive_offer_route = ($ts_after_req_route_recd - $ts_before_req_route);
	echo "time to receive offer route = $time_to_receive_offer_route\n";

	//echo "buf: $buf\n";
	list($passphrase, $recd_bandwidth, $ap_ip, $ap_bssid) = explode(";",$buf);

	//echo "passphrase= $passphrase\n";
	//echo "recd bandwidth= $recd_bandwidth\n";
	//echo "ap_ip= $ap_ip\n";
	//echo "ap_bssid= $ap_bssid\n";
	
}


//if the received bandwidth from AP is less than the requested bandwidth, the script dies
($recd_bandwidth >= $req_bandwidth) || die("Received bandwidth is not enough\n");

//add route for AP..while testing in ivtu uncomment the actual syntax in function
switch_route($ifc_name_entered);

//add arp entry for AP..uncomment actual syntax in ivtu
create_arp_entry($ap_ip,$ap_bssid);

//$ts_after_AP_route_added = microtime(true);
//echo "ts_after_AP_route_added: $ts_after_AP_route_added\n";

//close socket
socket_close($sock_handle_req_route_bcast);
socket_close($sock_handle_req_route_recv);

//include "./switch-inbound-route.php";

//****************************************Coped switch-inbound-route.php here - start ***********************************

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
	//echo "msg_send: $msg_send\n";
	//echo "ip_tosend: $ip_tosend\n";
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
			//echo "elapsed_time= $elapsed_time\n";
		}
	$count++;
	//echo "count= $count\n";
//	usleep(100);
 }

	//echo "socket_recv_len= $socket_recv_len\n";

//if any message was received on the socket, explode it to receive the acknowledgment
if ($socket_recv_len > 1)
{
	//echo "buf: $buf\n";
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

	 system("route add default gw $ap_ip dev $ifc_name_entered",$retval); 
	 system("route del -host $ap_ip_old/32 dev $ifc_name_old ",$retval);
	system("route del default gw $ap_ip_old dev $ifc_name_old");
//	system("ip route change default via $ap_ip dev $ifc_name_entered");
//	system("ip route flush cache");
 	system("echo 0 > /proc/sys/net/ipv4/route/flush");
	echo "showing route cache from switch-inbound\n";
        system("ip route show cache");


}

$ts_after_default_route_added = microtime(true);

echo "ts_after_default_route_added: $ts_after_default_route_added\n"; 

$time_for_L3_handoff = ($ts_after_default_route_added - $ts_before_req_route);
echo "time for L3 handoff: $time_for_L3_handoff\n";

//close socket
socket_close($sock_handle_switch_route);
//echo "Program going to sleep \n";
//usleep(120*1000*1000);
echo "Script End\n";


//****************************************Coped switch-inbound-route.php here - End ***********************************




/*****************************FUNCTIONS*********************************************************/

function socket_sendmsg_func($socket,$message,$port_send,$ip_to_send)
{
$len=strlen($message);
socket_sendto($socket,$message,$len,0,$ip_to_send,$port_send);
}

function socket_lookformsg_func($socket_handle, $port_recv, &$buf)
{
global $ifc_ip;
global $mn_floating_ip;
$buf = "";

$socket_recv_len=socket_recvfrom($socket_handle,$buf,65535,0,$ip_recv,$port_recv);
if($socket_recv_len <= 1)
{
//echo "zero recv len\n";
return $socket_recv_len;
}
        $fields = explode(";",$buf);

        $i=0;
        foreach($fields as $f)
        {
                //echo " $i Msg Recvd is $f\n";
                $msg[$i] = $f;
                $i++;
        }
        if (($msg[0] == "OFFER-ROUTE") || ($msg[0] == "SWITCH-INBOUND-ROUTE-OK"))
	{
		//echo "ip_recv: $ip_recv\n";
		//echo "mnfloating_ip: $mn_floating_ip\n";
		//echo "buf: $buf\n";
		return $socket_recv_len;
	}
	else
	{
	       //echo "receving our own xmit: $ip_recv, $port_recv. ifc-ip: $ifc_ip\n";
		//echo "received msg: $buf\n";
		$buf = "BAD Message\n";
		return 0;//we're receiving our own broadcast - so ignore
	}
//return $socket_recv_len;

}

function switch_route($ifc)
{
	global $ap_ip;

        //echo "system(\"route add -host $ap_ip/32  dev $ifc\")\n";
	//uncomment the following line while running in ivtu
	  system("route add -host $ap_ip/32 dev $ifc",$retval);

	//below line to be commented while testing
	//echo "retval in add route: $retval\n";
//	($retval == 0) || die("route add failed in handoff.php\n");
}

function create_arp_entry($ip,$mac)
{
	//uncomment the following line while running script in ivtu
	system("arp -s $ip $mac",$retval); //actual syntax
	echo "system(\"arp -s $ip $mac\")\n";

	//below line to be commented while testing
	//echo "retval in arp entry: $retval\n";
//	($retval == 0) || die("create arp entry failed in handoff.php\n");

}

function switch_ap($ifc, $bssid)
{
	//below line is actual syntax..uncomment in ivtu
         system("switch_ap $ifc $bssid",$retval);

        //echo "system(\"switch_ap $ifc $bssid\")\n";
	
	//below line to be commented while testing
	//($retval == 0) || die ("AP assoc failed \n");


   // return $ap_info;
}

?>






