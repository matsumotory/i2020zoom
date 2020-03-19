<?php
//	����
require("csvdbconf.inc");

//	mbstring������
mb_language("ja");
mb_internal_encoding("UTF-8");
mb_regex_encoding("UTF-8");
setlocale(LC_ALL, 'ja_JP.UTF-8');

//	dataFolder����
if (!file_exists($dataFolder)){
	mkdir($dataFolder, "0700");
	file_put_contents("$dataFolder/.htaccess", "deny from all\n");
}

//-----------------------------------------------------------------------------
//	�ؿ��������
  /**
     * �ե�����ݥ��󥿤���Ԥ��������CSV�ե�����ɤ��������
     * @param resource handle
     * @param int length
     * @param string delimiter
     * @param string enclosure
     * @return �ե�����ν�ü��ã��������ޤߡ����顼����FALSE���֤��ޤ���
     */
function fgetcsv_reg (&$handle, $length = null, $d = ',', $e = '"') {
    $d = preg_quote($d);
    $e = preg_quote($e);
    $_line = "";
    $eof = false;
    while (($eof != true)and(!feof($handle))) {
        $_line .= (empty($length) ? fgets($handle) : fgets($handle, $length));
        $itemcnt = preg_match_all('/'.$e.'/', $_line, $dummy);
        if ($itemcnt % 2 == 0) $eof = true;
    }
    $_csv_line = preg_replace('/(?:\\r\\n|[\\r\\n])?$/', $d, trim($_line));
    $_csv_pattern = '/('.$e.'[^'.$e.']*(?:'.$e.$e.'[^'.$e.']*)*'.$e.'|[^'.$d.']*)'.$d.'/';
    preg_match_all($_csv_pattern, $_csv_line, $_csv_matches);
    $_csv_data = $_csv_matches[1];
    for($_csv_i=0;$_csv_i<count($_csv_data);$_csv_i++){
        $_csv_data[$_csv_i]=preg_replace('/^'.$e.'(.*)'.$e.'$/s','$1',$_csv_data[$_csv_i]);
        $_csv_data[$_csv_i]=str_replace($e.$e, $e, $_csv_data[$_csv_i]);
    }
    return empty($_line) ? false : $_csv_data;
}

//	SJIS��BOM�դ�UTF-8��Csv������ɤ��롣UTF-8���Ѵ�����2��������ˤ����֤���
function loadCsv($fname){
	$sjis = @file_get_contents($fname);
	if (substr($sjis, 0,3) == "\xEF\xBB\xBF"){
		//	utf8 with bom
		$fp = fopen($fname, "r");
		fgetc($fp); fgetc($fp); fgetc($fp);	//	bom
		while ($data = fgetcsv_reg($fp)) {
			$csv[] = $data;
		}
	}else{
		$buf = mb_convert_encoding($sjis, "UTF-8", "sjis");
		$fp = tmpfile();
		fwrite($fp, $buf);
		rewind($fp);
		$csv=null;
		while ($data = fgetcsv_reg($fp)) {
			$csv[] = $data;
		}
	}
	return $csv;
}

//	2���������UTF-8BOM�դ���Csv�˥����֤��롣
function saveCsv($fname, $csv){
	$fpSave = fopen($fname, "w");
	fputs($fpSave, "\xEF\xBB\xBF");	//	BOM
	foreach($csv as $r) fputcsv($fpSave, $r);
	fclose($fpSave);
/*
	//	�ޤ�UTF-8����¸
	$utf8name = "$fname".".save.utf8.csv";
	$fpSave = fopen($utf8name, "w");
	fputs($fpSave, "\xEF\xBB\xBF");	//	BOM
	foreach($csv as $r) fputcsv($fpSave, $r);
	fclose($fpSave);
	//	SJIS���Ѵ���
	$save = mb_convert_encoding(file_get_contents($utf8name), "SJIS", "UTF-8");
	file_put_contents($fname, $save);
*/
}

//	���åץ����ɤ��줿�ե�������ݴɤ���ؿ�
function upload($type){
	global $record, $dataFolder;
	if (isset($_FILES[$type])){
		//	printArray($_FILES[$type]);
		$ext = strrchr($_FILES["$type"]["name"], ".");
		$base = "$dataFolder/" . $record["oid"]. $type;
		$tmp = "$dataFolder/" . "tmp_". $record["oid"]. $type;
		$name = $base . $ext;
		$old = glob("$base.*");
		foreach($old as $f) unlink($f);
		move_uploaded_file($_FILES["$type"]["tmp_name"], $tmp);
		if (file_exists($tmp)){
			rename($tmp, $name);
			return true;
		}
	}
	return false;
}

//	�ָ��ߤΥե�����פȤ����ե�����ؤΥ�󥯤�ɽ������ؿ���
function linkToFile($type){
	global $record, $dataFolder;
	if (sizeof(glob("$dataFolder/".$record["oid"].$type.".*"))>0){
		?>
		<a href="<?php echo $_SERVER['REQUEST_URI']?>&file=<?php echo $type?>">���ߤΥե�����</a>
		<?php
	}
}

//	�������Ȥ�ɽ������ؿ����ǥХå���
function printArray($a){
	if (is_array($a)){
		foreach($a as $k=>$v){
			echo "$k ";
			printArray($v);
		}
	}else{
		echo " => $a<br>\n";
	}
}

//	oid���б�����cid�����롣
function getCid($oid){
	global $cidSalt;
	return str_replace(".", "_", crypt($oid, $cidSalt));
}
//	CSV�ΰ���ܤ���Key��õ���ƥ������֤��֤���
function getPosFromKey($key, $csv){
	if(!is_array(@$csv[0])) return -1;
	foreach($csv[0] as $k=>$v){
		if($v==$key) return $k;
	}
	return -1;
}
//	cid���������쥳���ɤ��֤���
function getRecord($cid, $csv){
	$oidPos = getPosFromKey("oid", $csv);
	$record=NULL;
	$row=-1;
	if(!is_array(@$csv)) return NULL;
	foreach($csv as $key => $r){
		$c = getCid($r[$oidPos]);
		if ($c == $cid){
			if ($record){
				//	oid���ۤʤäƤ�cid��Ʊ���ˤʤ���������ʤ����Ȥϡ�checkcid.php�ǳ�ǧ���Ƥ�����
				echo "Ʊ���ID����ĥ쥳���ɤ�ʣ��¸�ߤ��ޤ������¹԰Ѱ��ˤ�Ϣ������������<br>";
				exit();
			}
			$record = $r;
			$row = $key;
		}
		//	���Υ����Ȥ�������ȡ�cid�ΰ�����ɽ�����ޤ���
		//	echo $r[$oidPos]. " => $cid<br>\n";
	}
	if (!$record) return NULL;
	$rv = array();
	foreach($csv[0] as $k => $v){
		$rv[$v] = $record[$k];
	}
	$rv['row'] = $row;
	return $rv;
}
function saveRecord(&$csv, $record){
	$row = $record['row'];
	foreach($csv[0] as $col => $key){
		$csv[$row][$col] = $record[$key];
		//	echo "csv[$row][$col] = ". $record[$key]. "<br>";
	}
}
//	���å���������å�
function lock(){
	global $fpLock, $dataFolder;
	$fpLock = @fopen("$dataFolder/lock.txt", "w");
	$count = 0;
	while(!$fpLock){
		//	echo 'lock() .. fopen("$dataFolder/lock.txt", "w") failed.<br>\n';
		usleep(100*1000);
		$fpLock = @fopen("$dataFolder/lock.txt", "w");
		$count ++;
		if ($count++ > 50){	//	5���Ԥ�
			?>
				<html>
				<meta http-equiv="content-type" content="text/html; charset=EUC-JP">
				<body>
					���ߡ������Ф��������Ƥ��뤿�ṹ�����Ǥ��ޤ���<br>
					���Ф餯��5�����١��ԤäƤ��顢
					Web�֥饦���Ρֺ��ɤ߹��ߡפޤ��ϡֹ����ץܥ���򲡤��ƤߤƤ���������
				</body>
				</html>
			<?php
			exit();
		}
	}
	flock($fpLock, LOCK_EX);
}
function unlock(){
	global $fpLock;
	if ($fpLock){
		flock($fpLock, LOCK_UN);
		fclose($fpLock);
	}
	$fpLock = NULL;
}
?>