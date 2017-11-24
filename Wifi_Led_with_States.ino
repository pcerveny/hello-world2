#include <SoftwareSerial.h>

#define DEBUG true
#define SSID "Your WiFi SSID"
#define PASS "Your Password"

SoftwareSerial dbgSerial(10, 11); // TX, RX
int counter;
int connectionId;

void setup()
{
  //setup RGB LED as indicator instead of softserial
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  delay(2000);
  digitalWrite(13, LOW);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial.setTimeout(5000);
  dbgSerial.begin(115200); //can't be faster than 19200 for softserial
  Serial.println("ESP8266 Demo");
  //test if the module is ready
  delay(1000);
  sendData("AT+RST\r\n", 1000, DEBUG); // reset module
  sendData("AT+CIPMUX=1\r\n", 1000, DEBUG); // configure for multiple connections
  //connect to the wifi
  if (connectWiFi())
  {

    sendData("AT+CIPMUX=1\r\n", 1000, DEBUG); // configure for multiple connections
    delay(1000);
    sendData("AT+CIPSERVER=1,80\r\n", 1000, DEBUG); // turn on server on port
    delay(1000);
    sendData("AT+CIFSR\r\n", 1000, DEBUG); // get ip address
    counter = 0;
  }

}

void loop()
{

  if (dbgSerial.available()) // check if the esp is sending a message
  {

    // Serial.println("dbgSerial available");
    if (dbgSerial.find("+IPD,"))
    {
      delay(1000); // wait for the serial buffer to fill up (read all the serial data)
      connectionId = dbgSerial.read() - 48; // subtract 48 because the read() function returns

      String t;
      t = dbgSerial.readStringUntil('\r');
      t.replace("GET /?", "");
      t.replace(" HTTP/1.1", "");

      Serial.print(t);

      if (t.indexOf("p") > 0)
      {
        manageLED();
      }
      else if (t.indexOf("s") > 0)
      {
        chkStates();
      }
      else
      {
        tryAgain();
      }

    }
  }
}

boolean connectWiFi()
{

  sendData("AT+CWMODE=1\r\n", 1000, DEBUG);
  String cmd = "AT+CWJAP=\"";
  cmd += SSID;
  cmd += "\",\"";
  cmd += PASS;
  cmd += "\"";
  sendData(cmd, 1000, DEBUG);
 Serial.println(cmd);
}

void tryAgain()
{
  String webpage = String("Something went wrong please try again");
  sendHTTPResponse(connectionId, webpage);
}
void manageLED()
{
  Serial.print("manage led called");

  dbgSerial.find("p"); // advance cursor to "pin="
  String webpage = "";
  if ( counter % 2 == 0) {
    //even
    digitalWrite(13, HIGH);
    webpage = String("Led Turned ON");
  }
  else
  { //odd
    digitalWrite(13, LOW);
    webpage = String("Led Turned OFF");
  }

  counter++;
  // generate web page
  sendHTTPResponse(connectionId, webpage);
}
void chkStates()
{
  Serial.print("check status called");
  String states = "";
  if ( counter % 2 == 0) {
    //even it means led is off
    states = "{ state: OFF }";
  }
  else
  { //odd it means led if on
    states = "{ state : ON }";
  }
  sendHTTPResponse(connectionId, states);
}
void sendHTTPResponse(int connectionId, String content)
{
  String httpResponse;
  String httpHeader;
  // HTTP Header
  httpHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n";
  httpHeader += "Content-Length: ";
  httpHeader += content.length();
  httpHeader += "\r\n";
  httpHeader += "Connection: close\r\n\r\n";
  httpResponse = httpHeader + content + " ";
  sendCIPData(connectionId, httpResponse);
}
void sendCIPData(int connectionId, String data)
{
  String cipSend = "AT+CIPSEND=";
  cipSend += connectionId;
  cipSend += ",";
  cipSend += data.length() ;
  cipSend += "\r\n";
  sendData(cipSend, 1000, true);
  sendData(data, 1000, true);
  
  String closeCommand = "AT+CIPCLOSE=";
  closeCommand += connectionId; // append connection id
  closeCommand += "\r\n";
  sendData(closeCommand, 1000, DEBUG); // close connection

}
String sendData(String command, const int timeout, boolean debug)
{
  String response = "";

  dbgSerial.print(command); // send the read character to the esp8266

  long int time = millis();

  while ( (time + timeout) > millis())
  {
    while (dbgSerial.available())
    {

      // The esp has data so display its output to the serial window
      char c = dbgSerial.read(); // read the next character.
      response += c;
    }
  }

  if (debug)
  {
    Serial.print(response);
  }

  return response;
}
