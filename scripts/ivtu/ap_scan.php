<?

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
	echo "ts_AP_assoc_in_loop: $ts_AP_assoc_in_loop\n";
	
	$time_in_assoc_loop = ($ts_AP_assoc_in_loop - $ts_before_AP_assoc);
	echo "time_in_assoc_loop: $time_in_assoc_loop\n";

} while (($time_in_assoc_loop < 10) && ($associated == "FALSE"));

($bssid == $ap_bssid_entered) || die("Association is not correct\n");

?>
