<?php
        //Copyright 2002, Stefan de Konink for 
        //The Embeddable Linux Kernel Subset Project

	function checklang($lang)
	{
		include("Functions/data.php");
		if (in_array($lang,array_keys($langitems), True)==False)
		{
			$return=array_keys($langitems);
			$result="$return[0]";
		}
		else $result=$lang;
		return $result;
	}

	function checkitem($lang, $item)
	{
		include("Functions/data.php");

		$arr=$langitems["$lang"]["menu"];
		$t = array();
    	while (list($k, $v) = each ($arr)) { $t[] = str_replace(" ", "", strtolower($v)); }
		$item=str_replace(" ", "", strtolower($item)); 
		if (in_array($item,array_values($t), True)==False) { $result=$t[8]; } else { $result=$item; }
		return $result;
		
//		if (in_array($item,$langitems["$lang"]["menu"], True)==False) { $result=$langitems["$lang"]["menu"][8]; } else { $result=$item; }
//		return $result;
	}
	
	function headers($lang)
	{
		include("Functions/data.php");

		$title=$langitems["$lang"]["title"];
        $meta="<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">";
		      "<meta name=\"description\" content=\"".$langitems["$lang"]["meta"][0].">".
		      "<meta name=\"keywords\" content=\"Linux,linux-86,linux-8086,16-bit,UNIX,ELKS\">";
       	$stylesheet="Functions/stylesheet.css";
       	$out="<head>$meta<title>$title</title><link href=\"$stylesheet\" rel=\"stylesheet\" type=\"text/css\"></head>";
       	return $out;
	}

	function getcontent($file)
	{
		$out="";
		if (!file_exists($file)) return $out;
		$fp = fopen ($file, "r");
		if (!$fp) return $out;
		while (!feof($fp))
		{
			$buffer = fgets($fp, 4096);
			$out=$out.$buffer;
		}
		fclose($fp);
		
		return $out;
	}


	function body($lang, $item)
	{
        $content=getcontent("Languages/".strtolower("$lang/$item.html"));
		$top=showtop();
		$menu=showmenu($lang);
                $footer=footer($lang);
                $out="<body link=\"#0000FF\" vlink=\"#FF00FF\" alink=\"#FF0000\">$top$menu$content$footer</body>";
        	return $out;
	}

	function showtop()
	{
		$out="<table border=\"0\" width=\"100%\">".
		"<tr><td width=\"20%\" align=\"left\" valign=\"top\"><img src=\"Images/ELKStag.gif\" alt=\"ELKS Logo\" height=\"77\" width=\"224\"></td>".
		"<td width=\"70%\" align=\"center\" valign=\"middle\"><img src=\"Images/ELKSbanner.gif\" alt=\"Linux in 640k\" height=\"60\" width=\"450\"></td>".
		"<td width=\"10%\" align=\"right\" valign=\"top\"><img src=\"Images/ELKSlogo.gif\" alt=\"Baby Linux Logo\" height=\"77\" width=\"65\"></td></tr></table>";
		return $out;
	}

	function showmenu($lang)
	{
		include("Functions/data.php");

		$data=$langitems[$lang]["menu"];
		$out="<div align=\"center\"><center><a href=\"CHANGELOG.txt\">$data[0]</a> | ".
		     "<a href=\"index.php?lang=$lang&+item=".str_replace(" ","",$data[1])."\">$data[1]</a> | ".
		     "<a href=\"index.php?lang=$lang&+item=".str_replace(" ","",$data[2])."\">$data[2]</a> | ".
		     "<a href=\"index.php?lang=$lang&+item=".str_replace(" ","",$data[3])."\">$data[3]</a> | ".
		     "<a href=\"index.php?lang=$lang&+item=".str_replace(" ","",$data[4])."\">$data[4]</a> | ".
		     "<a href=\"index.php?lang=$lang&+item=".str_replace(" ","",$data[5])."\">$data[5]</a> | ".
		     "<a href=\"index.php?lang=$lang&+item=".str_replace(" ","",$data[6])."\">$data[6]</a> | ".
		     "<a href=\"index.php?lang=$lang&+item=".str_replace(" ","",$data[7])."\">$data[7]</a> | ".
			 "<a href=\"index.php?lang=$lang&+item=".str_replace(" ","",$data[8])."\">$data[8]</a>".
		     "<hr width=\"90%\"></center></div>";
		return $out;
	}

	function footer($lang)
	{
		include("Functions/data.php");

		$data=$langitems[$lang]["footer"];
		$out="<div align=\"center\"><center><hr width=\"90%\">".(isset($data[0])?$data[0]:'')."<br>".(isset($data[1])?$data[1]:'')."</center></div>";
		return $out;
	}
?>
