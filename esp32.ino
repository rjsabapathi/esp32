#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <LiquidCrystal_I2C.h>

#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"
#define API_KEY "ApI key"
#define DATABASE_URL "url"

#define LED1_PIN 15
#define LDR_PIN 36
#define PWMChannel 0

int lcdColumns = 16;
int lcdRows = 2;

const int resolution = 8;
LiquidCrystal_I2C lcd(0x21, lcdColumns, lcdRows);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
int ldrData = 0;
float voltage = 0.0;

int pwmValue = 0;
bool ledStatus = false;

void setup()
{
  int freq = 5000;

  ledcSetup(PWMChannel, freq, resolution);
  ledcAttachPin(LED1_PIN, PWMChannel);
  lcd.init();
  lcd.backlight();

  Serial.begin(115200);

  lcd.setCursor(0, 1);
  lcd.print("WELCOME TO SEZ");
  delay(1000);
  lcd.clear();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Sign up successful");
    signupOK = true;
  }
  else
  {
    Serial.println(config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop()
{
  // Check if Wi-Fi is disconnected
  if (WiFi.status() != WL_CONNECTED)
  {
    // Wi-Fi is disconnected, turn off the LED
    ledcWrite(PWMChannel, 0);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WIFI DISCONNECTED");
    delay(300);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("MOTOR OFF");
    delay(300);

    // Update Wi-Fi disconnected status in Firebase
    if (Firebase.RTDB.setInt(&fbdo, "LED/wifi_status", 0))
    {
      Serial.println("Wi-Fi status updated to 0 (disconnected)");
    }
    else
    {
      Serial.println("Failed to update Wi-Fi status in Firebase: " + fbdo.errorReason());
    }

    // Update LED status in Firebase
    if (Firebase.RTDB.setInt(&fbdo, "LED/led_status", 0))
    {
      Serial.println("LED status updated to 0 (off)");
    }
    else
    {
      Serial.println("Failed to update LED status in Firebase: " + fbdo.errorReason());
    }

    return;
  }

  lcd.setCursor(0, 0);

  // Send data to Firebase every 5 seconds
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    ldrData = random(1, 1000);
    voltage = ldrData * 5.0 / 4095.0;

    if (Firebase.RTDB.setInt(&fbdo, "LED/ldr_data", ldrData))
    {
      Serial.println();
      Serial.print(ldrData);
      Serial.print("...successfully saved to: " + fbdo.dataPath());
      Serial.println("(" + fbdo.dataType() + ")");
    }
    else
    {
      Serial.println("FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "LED/voltage", voltage))
    {
      Serial.println();
      Serial.print(voltage);
      Serial.print("...successfully saved to: " + fbdo.dataPath());
      Serial.println("(" + fbdo.dataType() + ")");
    }
    else
    {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
  }

  if (Firebase.RTDB.getInt(&fbdo, "LED/analog"))
  {
    if (fbdo.dataType() == "string")
    {
      String pwmString = fbdo.stringData();
      pwmValue = pwmString.toInt();
      Serial.println("Successful READ From " + fbdo.dataPath() + ": " + pwmValue + " (" + fbdo.dataType() + ")");
      ledcWrite(PWMChannel, pwmValue);
      ledStatus = true;
    }

    // Check if PWM value is 255
    if (pwmValue == 255)
    {
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("LED  ON");
    }
    else
    {
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("LED OFF");
    }
  }

  if (ledStatus)
  {
    ledcWrite(PWMChannel, pwmValue);
  }
}
