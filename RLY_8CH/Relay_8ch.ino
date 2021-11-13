#include <string.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>

#define tx_BufferSize 100
#define tempCharBufferSize 50

char *ssid = "YOUR_WiFi_SSID"; // SSID
char *ssidPassword = "YOUR_WiFi_SSID_PASSWORD"; // SSID Password
char *internalIp = "192.168.1.14"; // Internal IP for ESP 8266
char *connectionpassword = "1234"; // Please change do not use this default

char *atCwjab = "AT+CWJAP=\"";
char *atCwjab2 = "\",\"";
char *dq = "\"";
char *atcpsta = "AT+CIPSTA=\"";
char *atcpsnd = "AT+CIPSEND=";
char *atcomma = ",";

char tempChar[tempCharBufferSize];
char rState2[2] = "\0";
char *splitWithN;
char *splitWithSpace;
char *splitWithAnd;

char tx_Buffer[tx_BufferSize];
int count;
int relayNumber;
int rState;

char *cmd; // 1=connectToWifi, 2=setCipsta, 3=setCipmux, 4=startCipserver
char delimiterSpace[] = " ";
char delimiterN[] = "\n";
char delimiterAnd[] = "&";

String espResponse;

char requestID[2] = "\0";
char *httpGetData;
String password;
String command;

const int answerWaitTime = 100;

// Messages PROGMEM!!!!!
const char messageHttpHeader[] PROGMEM  = "HTTP/1.1 200 OK\r\n\r\n";
const char messageOk[] PROGMEM  = "OK";
const char messagepasswordNotDefined[] PROGMEM  = "password_not_defined";
const char messagePasswordWrong[] PROGMEM  = "password_wrong";

const char atCipCLose[] PROGMEM  = "AT+CIPCLOSE=";
const char atCwMode3[] PROGMEM  = "AT+CWMODE=3";
const char atCipMux1[] PROGMEM  = "AT+CIPMUX=1";
const char atCipServer1[] PROGMEM  = "AT+CIPSERVER=1,80";


// Relays
int relays[] = {12, 2, 3, 4, 5, 6, 7, 8, 9};

void setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  onOff(2, "OFF");
  onOff(3, "OFF");
  onOff(4, "OFF");
  onOff(5, "OFF");
  onOff(6, "OFF");
  onOff(7, "OFF");
  onOff(8, "OFF");
  onOff(9, "OFF");

  pinMode(A3, OUTPUT); // Wifi connection LED
  pinMode(A2, OUTPUT); // Set IP LED
  pinMode(A1, OUTPUT); // Start server Led
  pinMode(A0, OUTPUT); // Data Led


  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  for (count = 1; count <= 8; count++) {
    rState = EEPROM.read(count );
    if (rState == 1) {
      onOff(relays[count], "ON");
    }
  }

  espResponse = sendToWifi("AT", true); // Send AT to Esp

  if (find(espResponse, "OK")) { // if get OK
      cleanTxBuffer();
      strcat_P(tx_Buffer, atCwMode3);
    espResponse = sendToWifi(tx_Buffer, true); // Set Esp CWMODE to 3
    if (find(espResponse, "OK")) { // if set to CWMODE
      cleanTxBuffer();
      strcat(tx_Buffer, atCwjab);
      strcat(tx_Buffer, ssid);
      strcat(tx_Buffer, atCwjab2);
      strcat(tx_Buffer, ssidPassword);
      strcat(tx_Buffer, dq);
      cmd = "1";
      espResponse = sendToWifi(tx_Buffer, false); // Try connect to SSID
    }
  }

}

void loop() {
  if ((Serial.available() > 0)) {
    digitalWrite(A0, HIGH);
    espResponse = "";
    espResponse = Serial.readString();
  } else {
    digitalWrite(A0, LOW);
  }

  if (find(espResponse, ",CONNECT")) { // We have HTTP connection request
    int str_len = espResponse.length() + 1;
    char char_array[str_len];
    espResponse.toCharArray(char_array, str_len);
    espResponse = "";

    splitWithN = strtok(char_array, delimiterN);
    count = 0;
    while (splitWithN != NULL) {

      if (count == 0) {
        requestID[0] = *(splitWithN);
      }

      if (count == 2) {
        httpGetData = splitWithN;
      }

      splitWithN = strtok(NULL, delimiterN);
      count++;
    }

    splitWithSpace = strtok(httpGetData, delimiterSpace);
    count = 0;
    while (splitWithSpace != NULL) {
      if (count == 1) {
        splitWithAnd = strtok(splitWithSpace, delimiterAnd);
        while (splitWithAnd != NULL) {
          if (find(splitWithAnd, "password=")) { // This is connection password variable
            password = splitWithAnd;
            password.replace("/?", "");
            password.replace("password=", "");
          }
          if (find(splitWithAnd, "command=")) { // This is command variable
            command = splitWithAnd;
            command.replace("command=", "");
          }
          splitWithAnd = strtok(NULL, delimiterAnd);
        }
      }

      splitWithSpace = strtok(NULL, delimiterSpace);
      count++;
    }

    if (password != "") {

      if (password.equals(connectionpassword)) { // Password correct

        // getinfo command
        if (find(command, "info")) {
          cleanTempChar();
          strcat_P(tempChar, messageHttpHeader);
          for (count = 1; count <= 8; count++) {
             rState  = EEPROM.read(count);
            
             rState2[0] = rState  +48; //Dec To Ascii
            strcat(tempChar, rState2);
            rState2[0] = '|';
            strcat(tempChar, rState2);
            
          }
          strcat_P(tempChar, messageOk);
          sendData(requestID, tempChar);
        }

        // relayon command
        if (find(command, "ron")) { // Relay ON command
          command.replace("ron", "");
          relayNumber = command.toInt();
          onOff(relays[relayNumber], "ON");
          EEPROM.update(relayNumber, 1);
          cleanTempChar();
          strcat_P(tempChar, messageHttpHeader);
          strcat_P(tempChar, messageOk);
          sendData(requestID, tempChar);
        }

        // relayoff command
        if (find(command, "roff")) { // Relay OFF command
          command.replace("roff", "");
          relayNumber = command.toInt();
          onOff(relays[relayNumber], "OFF");
          EEPROM.update(relayNumber, 0);
          cleanTempChar();
          strcat_P(tempChar, messageHttpHeader);
          strcat_P(tempChar, messageOk);
          sendData(requestID, tempChar);
        }

      } else { // Password wrong
        cleanTempChar();
        strcat_P(tempChar, messageHttpHeader);
        strcat_P(tempChar, messagePasswordWrong);
        sendData(requestID, tempChar);
      }
    } else { // Password not defined
      cleanTempChar();
      strcat_P(tempChar, messageHttpHeader);
      strcat_P(tempChar, messagepasswordNotDefined);
      sendData(requestID, tempChar);
    }
  }

  if (cmd == "1") {
    if (find(espResponse, "OK")) {
      cmd = "";
      //digitalWrite(A3, HIGH);
      cleanTxBuffer();
      strcat(tx_Buffer, atcpsta);
      strcat(tx_Buffer, internalIp);
      strcat(tx_Buffer, dq);
      cmd = "2";
      espResponse = sendToWifi(tx_Buffer, false); // Sets the IP Address of the ESP8266 Station
    }
  }

  if (cmd ==  "2") {
    if (find(espResponse, "OK")) {
      cmd = "";
      //digitalWrite(A2, HIGH);
      cmd = "3";
      cleanTxBuffer();
      strcat_P(tx_Buffer, atCipMux1);
      espResponse = sendToWifi(tx_Buffer, false); // Configure ESP for multiple connections
    }
  }

  if (cmd == "3") {
    if (find(espResponse, "OK")) {
      cmd = "";
      cmd = "4";
      cleanTxBuffer();
      strcat_P(tx_Buffer, atCipServer1);
      espResponse = sendToWifi(tx_Buffer, false); // Turn on server on port 80
    }
  }

  if (cmd == "4") {
    if (find(espResponse, "OK")) {
      cmd = "";
     // digitalWrite(A1, HIGH);
    }
  }

}

/*
  Name: sendToWifi
  Description: Function used to send data to ESP8266.
  Params: atCommand - the data/command to send; getAnswer - get return data from ESP8266 (true = yes, false = no)
  Returns: The response from the esp8266 (if there is a reponse and if getAnswer = true)
*/
String sendToWifi(char* atCommand, boolean getAnswer) {
  espResponse = "";
  Serial.println(atCommand);
  if (getAnswer == true) {
    delay(answerWaitTime);
    if (Serial.available() > 0) {
      while (Serial.available()) {
        espResponse = Serial.readString();
      }
    }
  }
  return espResponse;
}

/*
  Name: find
  Description: Function used to match two string
  Params: s - search in value
  Returns: true if match else false
*/
boolean find(String s, String value) {
  if (s.indexOf(value) >= 0) {
    return true;
  } else {
    return false;
  }
}

void sendData(char* reqID, char* dataStr) {

  char len[3] = "\0";
  char len_Value = strlen(dataStr);

  if (len_Value < 10 ) {
    len[0] = (len_Value % 10) + 48;
  }
  if (len_Value >= 10 ) {
    len[0] = ((len_Value / 10) % 10) + 48;
    len[1] = (len_Value % 10) + 48;
  }

  cleanTxBuffer();
  strcat(tx_Buffer, atcpsnd);
  strcat(tx_Buffer, reqID);
  strcat(tx_Buffer, atcomma);
  strcat(tx_Buffer, len);
  sendToWifi(tx_Buffer, false);
  delay(1200);
  sendToWifi(dataStr, false);
  delay(300);
  cleanTxBuffer();
  strcat_P(tx_Buffer, atCipCLose);
  strcat(tx_Buffer, reqID);
  sendToWifi(tx_Buffer, false);
}

void cleanTxBuffer() {
  for (count = 0; count < tx_BufferSize; count ++) {
    tx_Buffer[count] = '\0';
  }
}

void cleanTempChar() {
  for (count = 0; count < tempCharBufferSize; count ++) {
    tempChar[count] = '\0';
  }
}

void onOff(int pinNumber, String position) {
  if (position == "ON") {
    digitalWrite(pinNumber, LOW);
  } else {
    digitalWrite(pinNumber, HIGH);
  }
}
