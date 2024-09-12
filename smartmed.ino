#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <RtcDS1302.h>

// WiFi credentials
const char* ssid = "vaidik";
const char* password = "vaidik1000";

// Pin definitionsz
#define DS1302_CLK D5
#define DS1302_DAT D6
#define DS1302_RST D7
#define BUZZER_PIN D8
#define LCD_SDA D1
#define LCD_SCL D2

// Create instances
ThreeWire myWire(DS1302_DAT, DS1302_CLK, DS1302_RST);
RtcDS1302<ThreeWire> rtc(myWire);
LiquidCrystal_PCF8574 lcd(0x27); // Replace 0x27 with your LCD's I2C address

// Alarm structure
struct Alarm {
  int hour;
  int minute;
  String medicine;
};

Alarm alarms[10];
int alarmCount = 0;
ESP8266WebServer server(80);

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(115200);

  // Initialize the LCD
  Wire.begin(LCD_SDA, LCD_SCL); // Initialize I2C with custom SDA and SCL pins
  lcd.begin(16, 2); // Initialize with the number of columns and rows
  lcd.setBacklight(255); // Turn on backlight

  // Initialize RTC
  rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  rtc.SetDateTime(compiled);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize web server
  server.on("/", handleRoot);
  server.on("/setAlarm", handleSetAlarm);
  server.on("/deleteAlarm", handleDeleteAlarm);
  server.on("/editAlarm", handleEditAlarm);
  server.begin();
  Serial.println("Web server started");
}

unsigned long buzzerEndTime = 0;  // Variable to track buzzer end time
bool isBuzzerActive = false;      // Variable to track if the buzzer is active

void loop() {
  server.handleClient();
  RtcDateTime now = rtc.GetDateTime();

  bool alarmTriggered = false;
  for (int i = 0; i < alarmCount; i++) {
    // Trigger the alarm if the current time matches any alarm's hour and minute
    if (now.Hour() == alarms[i].hour && now.Minute() == alarms[i].minute && now.Second() == 0) {
      if (!isBuzzerActive) {  // Only trigger the buzzer if it's not already active
        playMelody();
        alarmTriggered = true;
        buzzerEndTime = millis() + 60000;  // Set buzzer end time
        isBuzzerActive = true;  // Mark buzzer as active
      }
    }
  }

  // If the buzzer duration has elapsed, deactivate it and update the LCD
  if (isBuzzerActive && millis() >= buzzerEndTime) {
    isBuzzerActive = false;  // Reset buzzer state for the next day
  }

  // Always update the LCD to show the next alarm
  displayNextAlarm(now);

  delay(100); // Small delay to reduce CPU usage
}


void playMelody() {
  // Twinkle Twinkle Little Star: Notes (in Hz) and durations (in ms)
  int melody[] = {
    262, 262, 392, 392, 440, 440, 392, // Twinkle, twinkle, little star
    349, 349, 330, 330, 294, 294, 262, // How I wonder what you are
    392, 392, 349, 349, 330, 330, 294, // Up above the world so high
    392, 392, 349, 349, 330, 330, 294, // Like a diamond in the sky
    262, 262, 392, 392, 440, 440, 392, // Twinkle, twinkle, little star
    349, 349, 330, 330, 294, 294, 262  // How I wonder what you are
  };
  
  int duration[] = {
    500, 500, 500, 500, 500, 500, 1000, // Twinkle, twinkle, little star
    500, 500, 500, 500, 500, 500, 1000, // How I wonder what you are
    500, 500, 500, 500, 500, 500, 1000, // Up above the world so high
    500, 500, 500, 500, 500, 500, 1000, // Like a diamond in the sky
    500, 500, 500, 500, 500, 500, 1000, // Twinkle, twinkle, little star
    500, 500, 500, 500, 500, 500, 1000  // How I wonder what you are
  };

  int melodyLength = sizeof(melody) / sizeof(melody[0]);
  
  for (int i = 0; i < melodyLength; i++) {
    tone(BUZZER_PIN, melody[i], duration[i]); // Play note
    delay(duration[i] * 1.30); // Delay between notes
  }
  
  noTone(BUZZER_PIN); // Stop sound
}



void displayNextAlarm(const RtcDateTime &now) {
  int nextAlarmIndex = -1;

  // Find the next upcoming alarm
  for (int i = 0; i < alarmCount; i++) {
    if (alarms[i].hour > now.Hour() || 
       (alarms[i].hour == now.Hour() && alarms[i].minute > now.Minute())) {
      nextAlarmIndex = i;
      break;
    }
  }

  lcd.clear();
  if (nextAlarmIndex >= 0) {
    lcd.setCursor(0, 0);
    lcd.print("Next: ");
    lcd.print(alarms[nextAlarmIndex].hour < 10 ? "0" : "");
    lcd.print(alarms[nextAlarmIndex].hour);
    lcd.print(":");
    lcd.print(alarms[nextAlarmIndex].minute < 10 ? "0" : "");
    lcd.print(alarms[nextAlarmIndex].minute);
    lcd.setCursor(0, 1);
    lcd.print(alarms[nextAlarmIndex].medicine);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("No Upcoming");
    lcd.setCursor(0, 1);
    lcd.print("Alarms");
  }
}




void handleRoot() {
  String html = R"=====( 
  <!DOCTYPE html>
  <html>
  <head>
    <title>Medicine Reminder</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        background-color: #f4f4f4;
        margin: 0;
        padding: 0;
      }
      .container {
        width: 100%;
        max-width: 800px;
        margin: 20px auto;
        background: #fff;
        padding: 20px;
        box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        border-radius: 8px;
      }
      h1 {
        text-align: center;
        color: #333;
      }
      form {
        display: flex;
        flex-direction: column;
        margin-bottom: 20px;
      }
      label {
        font-size: 16px;
        margin-bottom: 5px;
        color: #333;
      }
      input[type="number"], input[type="text"] {
        padding: 10px;
        margin-bottom: 15px;
        border: 1px solid #ccc;
        border-radius: 5px;
        font-size: 16px;
      }
      input[type="submit"] {
        padding: 10px;
        background-color: #28a745;
        color: white;
        border: none;
        border-radius: 5px;
        font-size: 16px;
        cursor: pointer;
      }
      input[type="submit"]:hover {
        background-color: #218838;
      }
      .reminder-list {
        margin-top: 20px;
      }
      .reminder-item {
        background: #f9f9f9;
        margin-bottom: 10px;
        padding: 10px;
        border: 1px solid #ddd;
        border-radius: 5px;
        display: flex;
        justify-content: space-between;
      }
      .reminder-item a {
        margin-left: 10px;
        color: #007bff;
        text-decoration: none;
      }
      .reminder-item a:hover {
        text-decoration: underline;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>Medicine Reminder</h1>
      <form action="/setAlarm" method="get">
        <label for="hour">Hour (0-23):</label>
        <input type="number" id="hour" name="hour" min="0" max="23" required>
        
        <label for="minute">Minute (0-59):</label>
        <input type="number" id="minute" name="minute" min="0" max="59" required>
        
        <label for="medicine">Medicine Name:</label>
        <input type="text" id="medicine" name="medicine" required>
        
        <input type="submit" value="Set Reminder">
      </form>

      <div class="reminder-list">
        <h2>Current Reminders</h2>
)=====";

  for (int i = 0; i < alarmCount; i++) {
    html += "<div class=\"reminder-item\">";
    html += "<div>" + String(i + 1) + ": " + String(alarms[i].hour) + ":" + (alarms[i].minute < 10 ? "0" : "") + String(alarms[i].minute) + " - " + alarms[i].medicine + "</div>";
    html += "<div><a href=\"/editAlarm?id=" + String(i) + "\">Edit</a> | <a href=\"/deleteAlarm?id=" + String(i) + "\">Delete</a></div>";
    html += "</div>";
  }

  html += R"=====( 
      </div>
    </div>
  </body>
  </html>
  )=====";

  server.send(200, "text/html", html);
}

void handleSetAlarm() {
  if (alarmCount < 10) {
    alarms[alarmCount].hour = server.arg("hour").toInt();
    alarms[alarmCount].minute = server.arg("minute").toInt();
    alarms[alarmCount].medicine = server.arg("medicine");
    alarmCount++;
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(200, "text/html", "Max number of reminders reached. <a href=\"/\">Go back</a>");
  }
}

void handleDeleteAlarm() {
  int id = server.arg("id").toInt();
  if (id >= 0 && id < alarmCount) {
    for (int i = id; i < alarmCount - 1; i++) {
      alarms[i] = alarms[i + 1];
    }
    alarmCount--;
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleEditAlarm() {
  int id = server.arg("id").toInt();
  if (id >= 0 && id < alarmCount) {
    String html = R"=====( 
    <!DOCTYPE html>
    <html>
    <head>
      <title>Edit Alarm</title>
      <style>
        body {
          font-family: Arial, sans-serif;
          background-color: #f4f4f4;
          margin: 0;
          padding: 0;
        }
        .container {
          width: 100%;
          max-width: 600px;
          margin: 20px auto;
          background: #fff;
          padding: 20px;
          box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
          border-radius: 8px;
        }
        h1 {
          text-align: center;
          color: #333;
        }
        form {
          display: flex;
          flex-direction: column;
        }
        label {
          font-size: 16px;
          margin-bottom: 5px;
          color: #333;
        }
        input[type="number"], input[type="text"] {
          padding: 10px;
          margin-bottom: 15px;
          border: 1px solid #ccc;
          border-radius: 5px;
          font-size: 16px;
        }
        input[type="submit"] {
          padding: 10px;
          background-color: #007bff;
          color: white;
          border: none;
          border-radius: 5px;
          font-size: 16px;
          cursor: pointer;
        }
        input[type="submit"]:hover {
          background-color: #0056b3;
        }
      </style>
    </head>
    <body>
      <div class="container">
        <h1>Edit Alarm</h1>
        <form action="/editAlarm" method="get">
          <input type="hidden" name="id" value=")=====" + String(id) + R"=====(">
          <label for="hour">Hour (0-23):</label>
          <input type="number" id="hour" name="hour" min="0" max="23" value=")=====" + String(alarms[id].hour) + R"=====(" required>
          
          <label for="minute">Minute (0-59):</label>
          <input type="number" id="minute" name="minute" min="0" max="59" value=")=====" + String(alarms[id].minute) + R"=====(" required>
          
          <label for="medicine">Medicine Name:</label>
          <input type="text" id="medicine" name="medicine" value=")=====" + alarms[id].medicine + R"=====(" required>
          
          <input type="submit" value="Update Reminder">
        </form>
      </div>
    </body>
    </html>
    )=====";

    server.send(200, "text/html", html);
  } else {
    server.send(404, "text/html", "Alarm not found. <a href=\"/\">Go back</a>");
  }
}
