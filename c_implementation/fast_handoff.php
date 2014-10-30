#!/bin/php
<?php
set_time_limit(0);//allow to run forever
$ESSID=trim(file_get_contents('/etc/telematics/ivtu/essid'));
$SIGNAL_LEVEL_LOW_THRESHOLD = intval(file_get_contents('/etc/telematics/ivtu/signal_low_threshold'));
$SIGNAL_LEVEL_DELTA_THRESHOLD = intval(file_get_contents('/etc/telematics/ivtu/signal_delta_threshold'));
$SIGNAL_LEVEL_DEVIATION_MAX = intval(file_get_contents('/etc/telematics/ivtu/signal_deviation_max'));
$SLEEP_TIME=trim(file_get_contents('/etc/telematics/ivtu/sleep_time'));
$MAX_RETRIES=trim(file_get_contents('/etc/telematics/ivtu/max_retries'));
$SCAN_HISTORY_LIST_MAX=5;
$MISSED_BEACON_MAX=$SCAN_HISTORY_LIST_MAX;
$BLACK_LIST_EXPIRY_SECS=2;
$SLEEP_USECS=1000*100;

$REALLY_LOW_SIGNAL_LEVEL=-500;
$NO_SIGNAL_LEVEL=-100;
$REALLY_HIGH_SIGNAL_LEVEL=0;

$debug = false;
$testing = false;
$test_data_file = "test.dat";

system("php /sbin/handoff_init.php");

if($argc > 1) 
{
    if($argv[1] == "debug")
    {
        $debug = true;
    }
    if( ($argv[1] == "test") && ($argc > 2) )
    {
        $debug = true;
        $testing = true;
        $test_data_file = $argv[2];
        $SLEEP_USECS = 1000*500;
        echo "Running in Test Mode\n";
        echo "Test data file is $test_data_file\n";
    }
}

class scan_line
{
    function scan_line($essid, $bssid, $signal_level,$flags)
    {
        $this->essid = $essid;
        $this->bssid = $bssid;
        $this->signal_level = intval($signal_level);
        $this->flags = $flags;
    }
 
//ran-start
	 function printinfo()
 	   {
 		echo $this->essid . "\n";
 		echo $this->bssid . "\n";
 		echo $this->signal_level . "\n";
		echo $this->flags . "\n";
 	   }
//ran-end

}

class ap_summary_info
{
    function ap_summary_info()
    {
        $this->missed_beacon_count = 0;
        $this->signal_level_sum = 0;
        $this->count = 0;
        $this->signal_level = 0;
        $this->signal_level_min = $REALLY_HIGH_SIGNAL_LEVEL;
        $this->signal_level_max = $REALLY_LOW_SIGNAL_LEVEL;
    }
}

$iwlist = "iwlist";
#$iwlist = "./scan.sh";
$IFC0 = "ath0";
$IFC1 = "ath1";
$MN_FL_IP=trim(file_get_contents('/etc/telematics/ivtu/mn_fl_ip'));

$active_ifc = $IFC0;
$idle_ifc = $IFC1;
$current_bssid = null;

$init_done = false;
$scan_history = array();
$ap_summaries = array();
$bssid_black_list = array();

//reset_route();

$initial_iter = 0;
$initializing = true;

while(true)
{
    $p = iwlist_open($idle_ifc);

    iwlist_parse_init();
    while($line=fgets($p))
    {
        $line = trim($line);

        if($line == "")
        {
            break ;//breaking on empty line allows us to feed test data easily
        }

        if(!iwlist_parse_line($line)) 
        {
	    //we haven't parsed all fields of the cell. so go back and parse more
            continue;
        }
	//now we have all the fields of the cell
        if( ($essid == $ESSID) && ($flags == "wpa" ) )
        {
            $scan_line = new scan_line($essid, $bssid, $signal_level, $flags);
            debug_print("essid=$essid,bssid=$bssid,signal-level:$signal_level,flags=$flags\n");
            $current_scan_line_list[$bssid] = $scan_line;
            
	    $scan_history[$bssid][] = $scan_line;

            $scan_line->printinfo($essid, $bssid, $signal_level, $flags); //ran
		
            if(!isset($ap_summaries[$bssid]))
            {
                $ap_summaries[$bssid] = new ap_summary_info();
	    }
            
	    $ap_summary = &$ap_summaries[$bssid];

	    debug_print("AP Summary BSSID=$bssid\n");
            
	    debug_print("AP Summary Curr. Signal Level Sum =$ap_summary->signal_level_sum\n");
	    $ap_summary->signal_level_sum += $scan_line->signal_level;
	    debug_print("AP Summary New Signal Level Sum =  $ap_summary->signal_level_sum \n");

	    debug_print("AP Summary Count $ap_summary->count \n");
            $ap_summary->count++;
	    debug_print("AP Summary New Count $ap_summary->count \n");

	    debug_print("AP Summary Curr. Signal Level = $ap_summary->signal_level\n");
            $ap_summary->signal_level = $ap_summary->signal_level_sum/$ap_summary->count;
	    debug_print("AP Summary New Signal Level = $ap_summary->signal_level\n");
            echo "Average AP signal Level is $ap_summary->signal_level\n"; 

            if($scan_line->signal_level > $ap_summary->signal_level_max)
            {
                $ap_summary->signal_level_max = $scan_line->signal_level;
            }

            if($scan_line->signal_level < $ap_summary->signal_level_min)
            {
                $ap_summary->signal_level_min = $scan_line->signal_level;
            }
            $ap_summary->missed_beacon_count = 0;
            //echo scan_line_to_str($scan_line);
        }
    }

    foreach($scan_history as $bssid => &$scan_line_list)
    {
        if(!isset($current_scan_line_list[$bssid]))
        {
            $scan_line = new scan_line($ESSID, $bssid, $NO_SIGNAL_LEVEL, "");
            $ap_summaries[$bssid]->missed_beacon_count++;
	    debug_print("AP Summary Missed Beacon Count = $ap_summary->missed_beacon_count\n");
            $scan_line_list[] = $scan_line;
        }
       
        $ap_summary = &$ap_summaries[$bssid];
        if(sizeof($scan_line_list) > $SCAN_HISTORY_LIST_MAX)
        {
	    debug_print("Scan history AP Summary BSSID=$bssid\n");
            $oldest_scan_line = array_shift($scan_line_list);

	    debug_print("Scan history AP Summary Curr. Signal Level Sum =$ap_summary->signal_level_sum\n");
            $ap_summary->signal_level_sum -= $oldest_scan_line->signal_level;
	    debug_print("Scan history AP Summary Signal Level Sum New is $ap_summary->signal_level_sum \n");

	    debug_print("Scan history AP Summary Curr. Count is $ap_summary->count \n");
            $ap_summary->count--;
	    debug_print("Scan history AP Summary Count New is $ap_summary->count \n");
            
	    debug_print("Scan history AP Summary Curr Signal Level is $ap_summary->signal_level \n");
            $ap_summary->signal_level = $ap_summary->signal_level_sum/$ap_summary->count;
	    debug_print("Scan history AP Summary Signal Level New is $ap_summary->signal_level \n");

            if($oldest_scan_line->signal_level >= ($ap_summary->signal_level_max + $SIGNAL_LEVEL_DELTA_THRESHOLD))
            {
                $ap_summary->signal_level_max = $REALLY_LOW_SIGNAL_LEVEL;
                foreach($scan_line_list as &$scan_line)
                {
                    if($scan_line->signal_level > $ap_summary->signal_level_max)
                    {
                        $ap_summary->signal_level_max= $scan_line->signal_level;
                    }
                }
            }
            
            if($oldest_scan_line->signal_level <= $ap_summary->signal_level_min)
            {
                $ap_summary->signal_level_min = $REALLY_HIGH_SIGNAL_LEVEL;
                foreach($scan_line_list as &$scan_line)
                {
                    if($scan_line->signal_level < $ap_summary->signal_level_min)
                    {
                        $ap_summary->signal_level_min = $scan_line->signal_level;
                    }
                }
            }

            if($ap_summary->missed_beacon_count >=  $MISSED_BEACON_MAX)
            {
                /*
                we haven't received beacon (or probe responses) recently - so free the memory objects
                */
                unset($ap_summaries[$bssid]);
                unset($scan_history[$bssid]);
                $is_current_bssid = "";
                if($bssid == $current_bssid)
                {
                    $current_bssid=null;
                    $is_current_bssid = "(is current bssid)";
                }
                trigger_error("Removing $bssid $is_current_bssid from scan_line list.  No recent frames received", E_USER_NOTICE);
            }
        }

    }

    $new_bssid = false;

    if(!$initializing)
    {
        if(!$current_bssid)
        {
            echo "Calling get_best_bssid";
            $new_bssid = get_best_bssid($ap_summaries);
        }else
        {
            echo "Calling get_next_bssid";
            $new_bssid = get_next_bssid($ap_summaries);
        }
    }else
    {
        $initial_iter++;
        if($initial_iter >= $SCAN_HISTORY_LIST_MAX)
        {
            $initializing=false;
        }
    }

    if($new_bssid)
    {
	debug_print("New BSSID=$new_bssid\n");
        $signal_level = $ap_summaries[$new_bssid]->signal_level;
	debug_print("New BSSID Signal Level =$signal_level\n");

        $current_signal_level = $ap_summaries[$current_bssid]->signal_level;
	debug_print("Current_Signal_Level =$current_signal_level\n");
        $current_signal_level_max = $scan_line_sumaries[$current_bssid]->signal_level_max;
	debug_print("Current_Signal_Level_Max =$current_signal_level_max\n");
        $current_signal_level_min = $scan_line_sumaries[$current_bssid]->signal_level_min;
	debug_print("Current_Signal_Level_Min =$current_signal_level_min\n");
                                        
        trigger_error("AP Switching - ifc: $idle_ifc, new_bssid: $new_bssid, signal-level: $signal_level, ".
                      "current_bssid: $current_bssid, signal-level: $current_signal_level, " .
                      "signal_level_max: $current_signal_level_max, signal_level_min: $current_signal_level_min"
                      , E_USER_NOTICE);
        $ap_info = switch_ap($idle_ifc, $new_bssid);

        //line format:
        //Mode: Managed Frequency: 2.462 GHz   Access Point: 00:0B:6B:0B:4E:C2
        $ap_info = trim($ap_info);
        $bssid = substr($ap_info, -17);
        $switch_ok = false;
        $switch_fail_reason = "";
	debug_print("Associated BSSID is =$bssid\n");

        if($bssid == $new_bssid)
        {
//            $def_route = switch_route($idle_ifc);
            echo "system(\"fast_handoff $idle_ifc $active_ifc $SLEEP_TIME $MAX_RETRIES $MN_FL_IP\")";
            $change_route = system("fast_handoff $idle_ifc $active_ifc $SLEEP_TIME $MAX_RETRIES $MN_FL_IP");
            $def_route = system("route -n"); 
            //last line will show default route which will use
            //the idle interface if the switch_route is successful
            //default route format:
            //0.0.0.0   192.168.60.1    0.0.0.0     UG  0   0   0 athX
            $ifc = substr($def_route, -1*strlen($idle_ifc));
	        debug_print("IFC=$ifc\n");
            if($ifc == $idle_ifc)
            {
                $switch_ok = true;
            }else
            {
                $switch_fail_reason = "switch_route failed";
            }
        }else
        {
            $switch_fail_reason = "switch_ap failed";
        }

        if($switch_ok)
        {
            $tmp = $idle_ifc;
            $idle_ifc = $active_ifc;
            $active_ifc = $tmp;
            $current_bssid = $new_bssid;
            $old_route = system("route -n | grep $idle_ifc | grep 192.168"); 
            $old_route_len = 1*strlen($old_route);
            $del_route = substr($old_route,-$old_route_len,16); 
            echo "Deleting old route $del_route\n";
            if ($del_route != "")
              system("route del $del_route");
         
        }else
        {
            trigger_error("AP Switch failed - ifc: $idle_ifc, new_bssid: $new_bssid, signal-level: $signal_level, ".
                        "Reason: $switch_fail_reason. Blacklisting $new_bssid for $BLACK_LIST_EXPIRY_SECS, ".
                        "current_bssid: $current_bssid, signal-level: $current_signal_level", E_USER_WARNING);
            //for some reason we were not able to switch to the
            //new AP fully - therefore we blacklist the AP for some time
            //NOTE: here we're assuming that our interface itself is
            //working fine
            $bssid_black_list[$new_bssid] = time();
        }
    }

    usleep($SLEEP_USECS);
    
    $curr_time = time();
    foreach($bssid_black_list as $bssid => $time)
    {
        $age = $curr_time - $time;
        if($age >= $BLACK_LIST_EXPIRY_SECS)
        {
            trigger_error("Removing $bssid from blacklist", E_USER_NOTICE);
            unset($bssid_black_list[$bssid]);
        }
    }
}

function get_best_bssid(&$ap_summaries)
{
    global $bssid_black_list;
    global $SIGNAL_LEVEL_DEVIATION_MAX;

    $best_signal_level = -9999;
    $best_bssid = false;
    foreach($ap_summaries as $bssid => &$ap_summary)
    {
        if ( ($ap_summary->signal_level > $best_signal_level) &&
             (($ap_summary->signal_level_max - $ap_summary->signal_level_min) <= $SIGNAL_LEVEL_DEVIATION_MAX)&&
             (!array_key_exists($bssid, $bssid_black_list)) )
        {
            $best_signal_level = $ap_summary->signal_level;
            $best_bssid = $bssid;
	    echo "Best BSSID is : $best_bssid\n";
        }
    }

    return $best_bssid;
}

function get_next_bssid(&$ap_summaries)
{
    global $current_bssid;
    global $SIGNAL_LEVEL_LOW_THRESHOLD;
    global $SIGNAL_LEVEL_DELTA_THRESHOLD;
    global $SIGNAL_LEVEL_DEVIATION_MAX;

    $current_signal_level = $ap_summaries[$current_bssid]->signal_level;
    $current_signal_level_max = $ap_summaries[$current_bssid]->signal_level_max;
    $current_signal_level_min = $ap_summaries[$current_bssid]->signal_level_min;

    if( ($current_signal_level <= $SIGNAL_LEVEL_LOW_THRESHOLD) ||
        ( ($current_signal_level_max - $current_signal_level_min) > $SIGNAL_LEVEL_DEVIATION_MAX))
    {
        $best_bssid = get_best_bssid($ap_summaries);
        if( ($best_bssid != $current_bssid) &&
            ($ap_summaries[$best_bssid]->signal_level >= ($current_signal_level + $SIGNAL_LEVEL_DELTA_THRESHOLD))
        )
        {
	    echo "Best BSSID is : $best_bssid\n";
            return $best_bssid;
        }
    }
    return false;
}

function scan_line_to_str($scan_line)
{
    return  "essid={$scan_line->essid},bssid={$scan_line->bssid},flags={$scan_line->flags},".
            "signal_level={$scan_line->signal_level}\n";
}

function iwlist_parse_init()
{
    global $bssid_count, $prev_bssid_count;
    
    $bssid_count = $prev_bssid_count = 0;
}

function iwlist_parse_line($line)
{
    global $essid, $flags, $bssid, $signal_level;
    global $bssid_count, $prev_bssid_count;

    $bssid_parse_done = false;

    $line = trim($line);
//	echo "line: $line\n"; //ran
//	echo "line array value 0: $line[0]\n"; //ran
//	echo "line array value 1: $line[1]\n"; //ran

    if($line[0] == 'C' && $line[1] == 'e')
    {
        //eg: Cell 01 - Address: 00:0B:6B:0B:4E:C2
        $bssid = substr($line, -17);
//	echo "BSSID: $bssid\n";  //ran
    }else
    if($line[0] == 'E' && $line[1] == 'S')
    {
        //eg: ESSID: "something"
       $essid = substr($line, 7, -1);
//  	   echo "ESSID: $essid\n";  //ran
    }else
    if($line[0] == 'M' && $line[1] == 'o')
    {
        //eg: Mode:Master
        $mode = substr($line, 5);
//	    echo "MODE: $mode\n";  //ran
        if($mode == "Master")
        {
            $flags = "wpa"; //TODO - we're not checking actual WPA status here
        }
    }else
    if($line[0] == 'Q' && $line[1] == 'u')
    {
        //eg: Quality=57/94 Signal level=-38 dBm Noise level=-95 dBm
        $n = strlen($line);
        $field_start_pos = -1;
        $field_name = "quality";
        for($i=0;$i<$n;$i++)
        {
            if($line[$i] == '=')
            {
                $field_start_pos = $i + 1;
                $field_len = 0;
            }else
            {
                if($line[$i] == ' ' || $line[$i] == '/')
                {
                    if($field_start_pos >= 0)
                    {
                        if($field_name == "signal_level")
                        {
			    //by now we have all the required fields
                            $bssid_count++;
                        }
                        $$field_name = substr($line, $field_start_pos,
                                        $field_len);
                        $field_start_pos = -1;
                        $field_len = 0;
                        if($field_name == "quality")
                        {
                            $field_start_pos = $i + 1;
                            $field_len = 0;
                            $field_name = "quality_base";
                        }else
                        if($field_name == "quality_base")
                        {
                            $field_name = "signal_level";
                        }else
                        if($field_name == "signal_level")
                        {
                            $field_name = "noise_level";
                        }
                    }
                }else
                {
                    $field_len++;
                }
            }
        }
    }

//	debug_print("prev_bssid_count: $prev_bssid_count\n");
//	debug_print("bssid_count: $bssid_count\n");

    if($prev_bssid_count != $bssid_count)
    {
        $bssid_parse_done = true;
        $prev_bssid_count = $bssid_count;
    }

//	echo "bssid_parse_done: $bssid_parse_done\n"; //ran
    return $bssid_parse_done;

}

function debug_print($s)
{
    global $debug;
    if($debug) echo $s;
}

function reset_route()
{
    global $testing;
    if($testing)
    {
        echo "system(\"route del default\")\n";    
        system("route del default");    
    }else
    {
        system("route del default");    
    }
}

function switch_route($ifc)
{
    global $testing;
    if($testing)
    {
        echo "system(\"switch_route $ifc\")\n";
//        system("switch_route $ifc");
        //we're pretending as if switch_route was successful
//        $def_route = "0.0.0.0   192.168.60.1    0.0.0.0     UG  0   0   0 $ifc";
    }else
    {
        system("switch_route $ifc");
        $def_route = system("route -n"); 
    }
    return $def_route;
}

function switch_ap($ifc, $bssid)
{
    global $testing;

    if($testing)
    {
        echo "system(\"switch_ap $ifc $bssid\")\n";
        system("switch_ap $ifc $bssid");
        $ap_info = system("iwconfig $ifc | grep \"Access Point\"");
        //for testing purpose we assume that switch_ap worked
//        $ap_info = "Mode:Master  Frequency:2.462 GHz  Access Point: $bssid";
    }else
    {
        system("switch_ap $ifc $bssid");
        $ap_info = system("iwconfig $ifc | grep \"Access Point\"");
	echo "AP Info is $ap_info\n"; 
    }
    return $ap_info;
}

function iwlist_open($ifc)
{
    global $iwlist;
    global $test_data_file;
    global $testing;
    global $iwlist_fp;
    global $current_bssid;
    global $counter; //ranjith added

    $iwcmd = "$iwlist $ifc scan";
//    echo "iwlist value: $iwlist\n";
    if($testing)
    {
        echo "Current BSSID: $current_bssid. Scanning: $iwcmd\n";
        if(!isset($iwlist_fp) || feof($iwlist_fp))
        {
            if(isset($iwlist_fp) && feof($iwlist_fp))
            {
                fclose($iwlist_fp);
            }
            $iwlist_fp = fopen($test_data_file, "r");
    	    $counter=$counter+1;
//            echo "ranjith echoes iwlist_fp value: $iwlist_fp\n";
//            echo "ranjith echoes counter value:$counter\n";

        }
        $p = $iwlist_fp;
        if(!$p)
        {
            trigger_error("Couldn't open $test_dat_file", E_USER_ERROR);
        }
    }else
    {
        if(isset($iwlist_fp))
        {
            pclose($iwlist_fp);
        }
        $iwlist_fp = popen($iwcmd, "r");
        $p = $iwlist_fp;
    }
    return $p;
}

?>
