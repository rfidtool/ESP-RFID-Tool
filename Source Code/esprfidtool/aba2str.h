String aba2str (String magstripe, int magStart, int magEnd, String swipeDirection) {
  String ABA="";
  String aba2str="";
  int magCount=abs(magEnd-magStart);
  aba2str=(String()+"\"Cleaned\" Binary:"+magstripe.substring(magStart,magEnd)+"\n");
  aba2str+=(String()+" * Possible "+swipeDirection+" Card Data\(ASCII\):");
  while (magCount>0) {
    ABA=magstripe.substring(magStart,magStart+4);
    if (ABA=="1101") {aba2str+=(";");}
    else if (ABA=="0000") {aba2str+=("0");}
    else if (ABA=="1000") {aba2str+=("1");}
    else if (ABA=="0100") {aba2str+=("2");}
    else if (ABA=="1100") {aba2str+=("3");}
    else if (ABA=="0010") {aba2str+=("4");}
    else if (ABA=="1010") {aba2str+=("5");}
    else if (ABA=="0110") {aba2str+=("6");}
    else if (ABA=="1110") {aba2str+=("7");}
    else if (ABA=="0001") {aba2str+=("8");}
    else if (ABA=="1001") {aba2str+=("9");}
    else if (ABA=="0011") {aba2str+=("<");}
    else if (ABA=="0111") {aba2str+=(">");}
    else if (ABA=="0101") {aba2str+=(":");}
    else if (ABA=="1011") {aba2str+=("=");}
    else if (ABA=="1111") {aba2str+=("?");}
    else {aba2str+=("_UNKNOWN-CHARACTER_");}
    magStart=magStart+5;
    magCount=magCount-5;
  }
  return aba2str;
}
