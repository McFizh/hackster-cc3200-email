<?PHP

function readStatusFromSqlite($db)
{
	$status = new stdClass();
	$status->unseen = "[]";

	$result = $db->query("SELECT key,value FROM imap_status");
	while($row=$result->fetchArray())
	{
		if( $row["key"] == "unseen" )
		  $status->unseen = $row["value"];
	}

	return $status;
}

function saveStatusToSqlite($db, $key, $value)
{
	$res = $db->exec("DELETE FROM imap_status WHERE key='".$key."'");
	if(!$res)
	{
		echo "DB error: "+$db->lastErrorMsg()+"\n";
		return;
	}

	$eValue = $db->escapeString($value);
	$res = $db->exec("INSERT INTO imap_status (key,value) VALUES ('".$key."', '".$eValue."')");
	if(!$res)
	{
		echo "DB error: "+$db->lastErrorMsg()+"\n";
		return;
	}
}

function readImapStatus($config, $savedStatus, $db)
{
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

	return $new_message_found;
}
