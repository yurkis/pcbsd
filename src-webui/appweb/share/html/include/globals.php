<?
   defined('DS') OR die('No direct access allowed.');

   use WebSocket\Client;

   // Create a websocket connection to syscache
   $wsport = "12142";
   $wslocal = "ws://127.0.0.1:" . $wsport;
   $wsremote = "ws://" . $_SERVER['SERVER_ADDR'] . ":" . $wsport;
   $scclient = new Client("$wslocal");

   // Check if we need to display desktop apps, or just server / CLI
   if ( file_exists("/usr/local/bin/startx") === false )
     $hasDesktop=false;
   else
     $hasDesktop=true;

   // Location of PBI index dir
   $pbiindexdir="/var/db/pbi/index";

   // Probally shouldn't modify these below
   ///////////////////////////////////////////////////

   // Set if we are viewing recommended or all PBI
   $allPBI = "false";
   if ( ! empty($_GET['allPBI']))
     $allPBI = $_GET['allPBI'];

   // Are we running through the AppCafe UI Qt program?
   $onDesktop = "false";
   if ( ! empty($_GET['AppCafeUI']))
     $onDesktop = $_GET['AppCafeUI'];
?>
