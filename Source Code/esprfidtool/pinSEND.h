      void pinSEND(int pinDELAY,String pinBIN) {
        for (int i=0; i<=pinBIN.length(); i++) {
          if (pinBIN.charAt(i) == '0') {
            digitalWrite(DATA0, LOW);
            delayMicroseconds(txdelayus);
            digitalWrite(DATA0, HIGH);
          }
          else if (pinBIN.charAt(i) == '1') {
            digitalWrite(DATA1, LOW);
            delayMicroseconds(txdelayus);
            digitalWrite(DATA1, HIGH);
          }
          delay(txdelayms);
        }
        yield();
        delay(pinDELAY);
        pinBIN="";
        pinDELAY=100;
      }
