## ABA Encoding  
  
Wiegand Magstripe Readers Tested:  
* HID 3110-6445 Magstripe Pass-Through Reader  
  * set to rotary position B (Raw Data - All Bits Wiegand)  
* HID multiCLASS RM40 iCLASS/Magstripe Reader 6220CKN000B  
  * purchase a reader with the last digit B in item # (ex: 6220CKN000**B**)  
  
RFID-Tool Specific Settings  
 * set buffer size to 256 bits or greater  
  
See [aba-decode.php](aba-decode.php) script for converting binary card data to ascii (Script by: AndrewMohawk)  
 * Command Line: /usr/bin/php aba-decode.php 1101000001100000100011001001001010101101111000001010011101101111100010  
 * Web:          https://www.LegacySecurityGroup.com/aba-decode.php  
  
Binary:  
5 bits  
Little Endian Format  
  
LRC(Longitudinal Redundancy Check):  
Count # of set bits(1's) in column  
EVEN = 0  
ODD  = 1  
  
Track 2 Debit/Credit Card Format(for example, as I could not find actual magstripe access control cards):  
;1234567890123456=YYMMSSSDDDDDDDDDDDDDD?*  
; = Start Sentinel  
1234567890123456 = 16 Digit Card #  
= = End Card #  
YY = Expiration Year  
MM = Expiration Month  
SSS = Service Code (As Understood From Wikipedia: "201" means chip required, "101" means no chip, be sure to recalculate the LRC if changing, it is not advised to experiment here without knowing the laws involved)  
DDDDDDDDDDDDDD = Discretionary Data  
? = End Sentinel  
*=LRC  
  
Binary Reference:  
11010 ; - Start Sentinel  
00001 0  
10000 1  
01000 2  
11001 3  
00100 4  
10101 5  
01101 6  
11100 7  
00010 8  
10011 9  
00111 <  
01110 >  
01011 :  
10110 = - End Card Number  
11111 ? - End Sentinel  
00010 LRC  
