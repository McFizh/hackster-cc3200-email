<?PHP
require "lib/libmqtt.php";
require "lib/common.php";
require "config/config.php";

$mqtt = null;

// Handle sigterm/sigint signals
declare(ticks = 1);
pcntl_signal(SIGTERM, "signal_handler");
pcntl_signal(SIGINT, "signal_handler");

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
   Create amazon IOT client
   ************************************************************************************** */   
echo "» Connecting to MQTT server\n";

$mqtt = new libMQTT\client( $config["aws_iot_host"], 8883, "backendClient");
$mqtt->setClientCert($config["aws_iot_crtfile"], $config["aws_iot_keyfile"]);
$mqtt->setCAFile($config["aws_iot_cafile"]);
$mqtt->setCryptoProtocol("tlsv1.2");
$mqtt->setVerbose(1);

if(!$mqtt->connect())
{
	echo "  » Connection failed\n";
	exit;
}

$topics = array();
$topics[ "sensor/cc3200/values" ] = [ "qos"=>1 , "function"=>"procmsg" ];

$mqtt->subscribe($topics);

$lastEmailCheck = 0;
while( 1 )
{
	$mqtt->eventLoop();
	usleep(250000);

	if( ( time() - $lastEmailCheck ) > 60 )
	{
		echo "» Reading email state\n";

		if(readImapStatus($config, $savedStatus, $db))
		{
			echo "  » New email found\n";
			$mqtt->publish("sensor/cc3200/cmd", "{\"mail\":true}", 1);
		}

		$lastEmailCheck = time();
	}
}

/* **************************************************************************************
   Signal and message handlers
   ************************************************************************************** */   
function procmsg($topic,$msg,$qos)
{
	global $db;

	if( $topic == "sensor/cc3200/values" )
	{
		$payload = json_decode($msg);
		if(!$payload)
			return;
		
		if( !isset($payload->hum) || !isset($payload->temp) || !isset($payload->time) )
			return;

		if( !is_numeric($payload->hum) || !is_numeric($payload->temp) )
			return;

		// (recorded,temperature,humidity) VALUES ('Sun Jan 17 21:15:58 2016', 2100.000000, 2600.000000)
		$t_time = date_parse_from_format("D M j G:i:s Y", $payload->time);
		if(!is_array($t_time))
			return;

		$time = sprintf("%s-%s-%s %s:%s:%s",
			$t_time["year"], $t_time["month"], $t_time["day"],
			$t_time["hour"], $t_time["minute"], $t_time["second"]);

		$sql = sprintf("INSERT INTO environment (recorded,temperature,humidity) VALUES ('%s', %f, %f)",
			$db->escapeString($time),
			floatval($payload->temp/100.0),
			floatval($payload->hum/100.0) );

		$res = $db->exec($sql);
		if(!$res)
		{
			echo "DB error: "+$db->lastErrorMsg()+"\n";
			return;
		}

	} else {
		echo "Msg Received: ".date("r")."\nTopic:{$topic}\n$msg\n";
	}
}

function signal_handler($signal) {
	global $mqtt;
	switch($signal) {
		case SIGTERM:
			print "Caught SIGTERM\n";
			unset($mqtt);
			exit;
		case SIGKILL:
			print "Caught SIGKILL\n";
			unset($mqtt);
			exit;
		case SIGINT:
			print "Caught SIGINT\n";
			unset($mqtt);
			exit;
		default:
			print "Unknown signal\n";
			unset($mqtt);
			exit;
	}
}

