<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<?php
	//Copyright 2002, Stefan de Konink for 
	//The Embeddable Linux Kernel Subset Project
	include("./Functions/total.php");
	if (isset($_GET['lang']) && strlen(trim($_GET['lang']))!=0) $lang=$_GET['lang']; else $lang=$_SERVER["HTTP_ACCEPT_LANGUAGE"];
	$lang=checklang($lang);
	if (isset($_GET['item']) && strlen(trim($_GET['item']))!=0) $item=$_GET['item']; else $item='';
	$item=checkitem($lang, $item);
   	echo headers($lang);
   	echo body($lang, $item);
?>
</html>
