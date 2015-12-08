<?
defined('DS') OR die('No direct access allowed.');
$jail = "#system";
$jailUrl="__system__";
?>
<table class="header" style="width:100%">
<tr>
    <th>
        <h1><center>Installed Applications</h1>
    </th>
</tr>
<br>
<br>
<table class="jaillist" style="width:100%">
<tr>
   <th></th>
   <th></th>
</tr>
<?

   $skipstop = $skip + 50;

   $totalCols = 3;

   $pkgoutput = syscache_ins_pkg_list($jail);

   if ( $jail == "#system" )
     $pbioutput = syscache_pbidb_list();
   else
     $pbioutput = syscache_pbidb_list("serverapps");

   $pkglist = $pkgoutput;
   $pbilist = $pbioutput;

   // Now loop through pbi origins
   $col=1;

   // Set the counter
   $curItem=0;
   $atEnd = true;

   foreach ($pkglist as $pbiorigin) {
     // Is this PBIs origin package installed?
     $sccmd = array("$jail app-summary $pbiorigin");
     $response = send_sc_query($sccmd);
     $pbiarray = $response["$jail app-summary $pbiorigin"];

     // If we have PBI data for this, the canremove will be on section 9
     if ( array_search($pbiorigin, $pbilist) !== false)
       $pbicanremove = $pbiarray[9];
     else
       $pbicanremove = $pbiarray[5];

     if ( "$pbicanremove" != "true" )
        continue;

     parse_details($pbiorigin, "$jail", $col, true, false, $pbiarray);
     if ( $col == $totalCols )
       $col = 1;
     else
       $col++;

     $curItem++;
   }

   echo "</tr>";
?>

</table>
</div>
