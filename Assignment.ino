#include <WiFi.h>
#include <DHT.h>
#include <OneButton.h>
#include <MicroGear.h>
  
#define DHTPIN 32
#define SWITCHPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE); 

#define RELAY1_PIN 26
#define RELAY2_PIN 27
#define RELAY_ON 0
#define RELAY_OFF 1 

//Infomation for NETPIE.
#define APPID   "porametSmartHome"
#define KEY     "5kcc8MVdOHvU0JT"
#define SECRET  "PGIu5gWzoXr9KJavVjBooZCde"
#define ALIAS   "ESP32"

char str[32];
char buff[16];
 

const double send_to_thingspeak_interval = 1000*60*15; //time to send data to thingspeak in millisecound

//Infimation for Thinspeak.
String apiKey = "XA6QINFLOXG4QIM8";
const char* ssid = "Eyesfiniti-2G";
const char* password = "26022541";
const char* server = "api.thingspeak.com";
WiFiClient client;
WiFiClient netpieclient;

MicroGear microgear(netpieclient);

OneButton button(SWITCHPIN, true);

unsigned long previousMillisThingspeak = 0;
unsigned long previousMillisNETPIE = 0;

float temp;
int hum;

int relay1_state = RELAY_OFF;
int relay2_state = RELAY_OFF;

/* If a new message arrives, do this */
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
    Serial.print("Incoming message --> ");
    msg[msglen] = '\0';
    Serial.println((char *)msg);
    Serial.println((char *)topic);
    String value = (char *)msg;
    Serial.println(value);
    if(value == "relay1.ON"){
      relay1_state = RELAY_ON;
      digitalWrite(RELAY1_PIN, relay1_state);
      microgear.publish("/relay1", relay1_state);
    }
    else if(value == "relay1.OFF"){
      relay1_state = RELAY_OFF;
      digitalWrite(RELAY1_PIN, relay1_state);
      microgear.publish("/relay1", relay1_state);
    }

    else if(value == "relay2.ON"){
      relay2_state = RELAY_ON;
      digitalWrite(RELAY2_PIN, relay2_state);
      microgear.publish("/relay2", relay2_state);
    }
    else if(value == "relay2.OFF"){
      relay2_state = RELAY_OFF;
      digitalWrite(RELAY2_PIN, relay2_state);
      microgear.publish("/relay2", relay2_state);
    }
}

void onFoundgear(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.print("Found new member --> ");
    for (int i=0; i<msglen; i++)
        Serial.print((char)msg[i]);
    Serial.println();
    getDataFromDHT();
    sprintf(str, "%.2f,%d",temp, hum);
    Serial.println(str);
    Serial.println("Sending -->");
    microgear.publish("/data", str);
}

void onLostgear(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.print("Lost member --> ");
    for (int i=0; i<msglen; i++)
        Serial.print((char)msg[i]);
    Serial.println();
}

/* When a microgear is connected, do this */
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("Connected to NETPIE...");
    /* Set the alias of this microgear ALIAS */
    microgear.setName(ALIAS);
}


void oneclick(){
  relay2_state = !digitalRead(RELAY2_PIN);
  digitalWrite(RELAY2_PIN, relay2_state);
  Serial.print("Relay2 Change state to: ");
  Serial.println(!relay2_state);
  microgear.publish("/relay2", relay2_state);
}

void doubleclick(){
  relay1_state = !digitalRead(RELAY1_PIN);
  digitalWrite(RELAY1_PIN, relay1_state);
  Serial.print("Relay1 change state to: ");
  Serial.println(!relay1_state);
  microgear.publish("/relay1", relay1_state);

}

void getDataFromDHT(){
  temp = dht.readTemperature();
  hum = dht.readHumidity();
  Serial.print("Temperature: ");
  Serial.println(temp);
  Serial.print("Humidity: ");
  Serial.println(hum);
}

void sendDataToThingspeak(){
  const int httpPort = 80;
  if (!client.connect(server, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  String url = "/update?api_key=";
  url += apiKey;
  url += "&field1=";
  url += temp;
  url += "&field2=";  
  url += hum;
  url += "&field3=";  
  url += (!relay1_state);
  url += "&field4=";  
  url += (!relay2_state);

  Serial.print("Requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                "Host: " + server + "\r\n" + 
                "Connection: close\r\n\r\n");
                
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  Serial.println();
  Serial.println("closing connection");
}

void setup() {

    /* Add Event listeners */
    /* Call onMsghandler() when new message arraives */
  microgear.on(MESSAGE,onMsghandler);

    /* Call onFoundgear() when new gear appear */
  microgear.on(PRESENT,onFoundgear);

    /* Call onLostgear() when some gear goes offline */
  microgear.on(ABSENT,onLostgear);

    /* Call onConnected() when NETPIE connection is established */
  microgear.on(CONNECTED,onConnected);
  
  pinMode(DHTPIN, INPUT_PULLUP);

  pinMode(SWITCHPIN, INPUT_PULLUP);
  button.attachClick(oneclick);
  button.attachDoubleClick(doubleclick);
  
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, relay1_state);
  digitalWrite(RELAY2_PIN, relay2_state);
  
  Serial.begin(9600);
  
  dht.begin();
  
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //Connect to WiFi
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //connect to NETPIE.
  microgear.init(KEY,SECRET,ALIAS);
  microgear.connect(APPID);
  
  //initial value of dashboard.
  getDataFromDHT();
  sprintf(str, "%.2f,%d",temp, hum);
  Serial.println(str);
  Serial.println("Sending -->");
  microgear.publish("/data", str);
}


unsigned int timer = 0;

void loop() {
  
  button.tick();
  if(microgear.connected())
    microgear.loop();

  if(millis() - previousMillisThingspeak >= send_to_thingspeak_interval){
    previousMillisThingspeak = millis();
    sendDataToThingspeak();
  }

  if(millis() - previousMillisNETPIE >= 1000){
  	previousMillisNETPIE = millis();
  	if(microgear.connected()){
  		if(millis() - timer >= 1000*60*5){
        getDataFromDHT();
  			sprintf(str, "%.2f,%d",temp, hum);
  			Serial.println(str);
  			Serial.println("Sending -->");
  			microgear.publish("/data", str);
  			timer = millis();
  		}
  	}
  	else {
  		Serial.println("Connection lost, reconnect...");
  		if(millis() - timer >= 1000*5) {
  			microgear.connect(APPID);
  			timer = millis();
  		}
  	}
  }

}
