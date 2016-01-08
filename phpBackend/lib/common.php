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

