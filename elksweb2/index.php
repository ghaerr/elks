<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<?php
	//Copyright 2002, Stefan de Konink for 
	//The Embeddable Linux Kernel Subset Project
	include("./Functions/total.php");

	if ($lang=="") $lang=$HTTP_ACCEPT_LANGUAGE;
	$lang=checklang($lang);
	$item=checkitem($lang, $item);
    	echo headers($lang);
    	echo body($lang, $item);
?>
</html>
