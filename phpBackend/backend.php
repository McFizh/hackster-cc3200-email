<?PHP
require "lib/aws.phar";
require "lib/common.php";
require "config/config.php";

/* **************************************************************************************
   Open SQLite file and create temperature/status tables if they are not already there 
   ************************************************************************************** */
echo "» Reading state from sqlite\n";

$db = new SQLite3($config["sqlite_file"]);
$db->exec("CREATE TABLE IF NOT EXISTS environment (
	id INTEGER PRIMARY KEY,
	recorded DATETIME,
	temperature REAL,
	humidity REAL)");

$db->exec("CREATE TABLE IF NOT EXISTS imap_status (
	id INTEGER PRIMARY KEY,
	key TEXT,
	value TEXT)");

$savedStatus = readStatusFromSqlite($db);

/* **************************************************************************************
   Open imap connection 
   ************************************************************************************** */
echo "» Reading email state\n";

$mbox = imap_open($config["imap_server"], $config["imap_user"], $config["imap_password"]);
$imapStatus = imap_status($mbox, $config["imap_server"], SA_ALL);

// Read message IDs of UNSEEN mails
$unseen = [];

if( $imapStatus->unseen > 0 )
{
	$messages = imap_search($mbox, "UNSEEN");
	foreach($messages as $msgID)
	{
		$msg = imap_headerinfo($mbox, $msgID);
		$unseen[]=$msg->message_id;
	}
}

imap_close($mbox);

// 
$last_unseen = json_decode($savedStatus->unseen);
$new_message_found = false;

foreach($unseen as $key=>$value)
{
	// Does 'unseen' contain msgID we haven't seen before?
	if( array_search($value, $last_unseen) === FALSE )
		$new_message_found = true;
}

// Current unseen list doesn't match saved one
if( count($unseen) != count($last_unseen) || $new_message_found )
{
	saveStatusToSqlite($db, "unseen", json_encode($unseen));
}

if($new_message_found)
{
	echo "  » New email found\n";
}

/* **************************************************************************************
   Create amazon IOT client
   ************************************************************************************** */
echo "» Reading thing state\n";

$iot = new Aws\IotDataPlane\IotDataPlaneClient([
	'region'  => 'eu-west-1',
	'version' => '2015-05-28',
	'credentials' => [
		'key' => $config['aws_iot_access_key'],
		'secret' => $config['aws_iot_secret_key']
	]
]);

$result = $iot->getThingShadow([
	'thingName' => $config["aws_iot_thing_name"]
]);

