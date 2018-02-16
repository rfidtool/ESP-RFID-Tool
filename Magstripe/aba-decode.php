<?php

/*
USAGE:
Command Line: /usr/bin/php aba-decode.php 1101000001100000100011001001001010101101111000001010011101101111100010
Web:          www.LegacySecurityGroup.com/aba-decode.php?binary=1101000001100000100011001001001010101101111000001010011101101111100010
*/

/* Decode Track 2 data from binary */
if (defined('STDIN')) {
  $binary = filter_var($argv[1], FILTER_SANITIZE_NUMBER_INT);
  define( "LINEBREAK", PHP_EOL);
} else {
  if(isset($_POST['submit'])) {
    $binary = filter_input(INPUT_POST, 'binary', FILTER_SANITIZE_NUMBER_INT);
  }
  else {
    $binary = filter_input(INPUT_GET, 'binary', FILTER_SANITIZE_NUMBER_INT);
  }
  define( "LINEBREAK", "<br>");
}
if (empty($binary)) {
  $binary = "1101000001100000100011001001001010101101111000001010011101101111100010";
}

echo "https://github.com/rfidtool/ESP-RFID-Tool/blob/master/Magstripe/aba-decode.php" . LINEBREAK;
echo "For converting Track 2 Magstripe ABA Binary data to ASCII" . LINEBREAK . LINEBREAK;

echo "Original script by: AndrewMohawk" . LINEBREAK;
// andrew@andrewmohawk.com
echo "http://www.andrewmohawk.com" . LINEBREAK . LINEBREAK;

echo "Modified slightly by: Corey Harding" . LINEBREAK;
echo "www.LegacySecurityGroup.com / www.Exploit.Agency" . LINEBREAK . LINEBREAK;

if (!defined('STDIN')) {
?>
<html>
<body>
<form action="<?php basename(__FILE__, '.php'); ?>" method="post">
    <input type='text' name="binary" value="<?php echo $binary; ?>" />
    <input type="submit" name="submit" value="Submit" />
</form>
</body>
</html>
<?php
}

// this function by mtroy dot student at gmail dot com taken from http://php.net/manual/en/function.strpos.php
function strpos_r($haystack, $needle)
{
    if(strlen($needle) > strlen($haystack))
        trigger_error(sprintf("%s: length of argument 2 must be <= argument 1", __FUNCTION__), E_USER_WARNING);

    $seeks = array();
    while($seek = strrpos($haystack, $needle))
    {
        array_push($seeks, $seek);
        $haystack = substr($haystack, 0, $seek);
    }
    return $seeks;
}

function processBinary($binary)
{
	$AsciiOutput = "";
	
	//find start sentinel
	$start_sentinel = strpos($binary,"11010");
	if($start_sentinel === false)
	{
		echo "Could not find start sentinel" . LINEBREAK;
		return false;
	}
	
	//find end sentinel
	$end_sentinel = false;
	$end_sentinel = strrpos($binary,"11111");
	if(count($end_sentinel) == 0)
	{
		echo "Could not find end sentinel" . LINEBREAK;
		return false;
	}
	
	//Lets decode the data:
	$bit_length = 5; // 4 bits for data, 1 bit for odd-parity or LRC checking
	
	
	$data = substr($binary,$start_sentinel,($end_sentinel-$start_sentinel+5));
	
	$currentBits = "";
	$currentNum = 0;
	$finalString = "";
	
	for($i=0;$i<strlen($data);$i++)
	{
		if(strlen($currentBits) < $bit_length)
		{
			$currentBits .= $data[$i];
			
		}
		
		if(strlen($currentBits) == $bit_length)
		{
			$parityBit = $currentBits[4];
			$dataBits = substr($currentBits,0,4);
			
			$asciiChar = 0;
			
			
			for($x=0;$x<4;$x++)
			{
				$currentNum += $dataBits[$x];
			}
			
			
			 
			$dec = bindec($dataBits);
			$dec = str_pad($dec, 2, "0", STR_PAD_LEFT); // just so output is nice
			$asciiChar = chr(bindec(strrev($dataBits))+48); // reverse the binary (since its LSB first) then convert to dec, add 48 and then take it to ASCII
			echo "$currentBits - Data ($dataBits) Parity($parityBit) Decimal ($dec) Ascii($asciiChar)";
			if(($currentNum + $parityBit) % 2 == false)
			{
				echo " __ Parity: Invalid";
			}
			else
			{
				echo " __ Parity: Valid";
			}
			$AsciiOutput .= $asciiChar;
			echo LINEBREAK;
			$currentBits = "";
			$currentNum = 0;
			
		}
		
		
	}
	echo  LINEBREAK . LINEBREAK . "Total Out (ascii): $AsciiOutput" . LINEBREAK;
}
echo "Decoding " . $binary . LINEBREAK . LINEBREAK;
echo "Trying One way:" . LINEBREAK . LINEBREAK;
if (processBinary($binary) == false)
{
	//reverse.
	echo  LINEBREAK . LINEBREAK;
	echo "Trying The Reverse:" . LINEBREAK . LINEBREAK;
	processBinary(strrev($binary));
}