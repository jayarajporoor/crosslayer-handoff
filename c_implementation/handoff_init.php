<?
$init_done = 0;
system("iptables -t nat -F");
system("echo -n 0 > /proc/sys/net/ipv4/conf/ath0/rp_filter");
system("echo -n 0 > /proc/sys/net/ipv4/conf/ath1/rp_filter");
system("echo -n 0 > /proc/sys/net/ipv4/route/flush");

// Deleting Old Default Route 
system("route del default");

// Adding New Default Route 
system("route add default gw 192.168.1.1");

// Deleting Old Routes to APs
            $old_route = system("route -n | grep 192.168.60"); 
            $old_route_len = 1*strlen($old_route);
            $del_route = substr($old_route,-$old_route_len,16); 
//            echo "Deleting old route $del_route\n";
            while ($del_route != "")
            {
                echo "Deleting old route $del_route\n";
                system("route del $del_route");
                $old_route = system("route -n | grep 192.168.60"); 
                $old_route_len = 1*strlen($old_route);
                $del_route = substr($old_route,-$old_route_len,16); 
            }


$MN_FL_IP=trim(file_get_contents('/etc/telematics/ivtu/mn_fl_ip'));
system("iptables -t nat -F");
system("ip addr add $MN_FL_IP dev lo");
system("iptables -t nat -A POSTROUTING -o ath0 -j SNAT --to-source $MN_FL_IP");
system("iptables -t nat -A POSTROUTING -o ath1 -j SNAT --to-source $MN_FL_IP");
$init_done = 1;

echo"Init Done $init_done\n"
?>

