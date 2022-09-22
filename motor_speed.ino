#include <WiFi.h>
#include "ThingsBoard.h"
#include <ArduinoJson.h>

#include "config.h"

int motor1Pin1 = 18; 
int motor1Pin2 = 19; 
int enable1Pin = 4; 

const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 255;
int curState = 1;

// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;

const size_t callbacks_size = 4;
//int curState = 1;

void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

void reconnect() 
{
  // Loop until we're reconnected
  status = WiFi.status();
  if ( status != WL_CONNECTED) 
  {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected to AP");
  }
}


RPC_Response processStateChange(const RPC_Data &data)
{
  Serial.println("Received the set state RPC method");

  int nextstate = data["state"];

  switch(nextstate)
  {
    case 0:
    motorStop();
    break;
    
    case 1:
    motorForward();
    break;
    
    case 2:
    motorBackward();
    break;
    
    default :
    Serial.println("Invalid RPC");
    }
    Serial.println("Current state");
    Serial.println(nextstate);
    
    tb.sendTelemetryInt("State", nextstate);
    if(nextstate == 0)
    {
      curState = 1;
    }
    else
    {
      curState = nextstate;
    }
    
    return RPC_Response("State", curState);
}


RPC_Response processSpeedChange(const RPC_Data &data)
{
  Serial.println("Received the set speed method");

  // Process data

  //int speed = data["speed"];
  int speed = data;
  setDuty(speed);
  tb.sendTelemetryInt("Speed", speed);
  return RPC_Response("motorEdgeSpeed", speed);
}

//const size_t callbacks_size = 2;
RPC_Callback callbacks[callbacks_size] = {
  { "motorState",    processStateChange },
  { "motorSpeed",         processSpeedChange }
};

void setup() {
  Serial.begin(115200);

  Serial.print("Testing DC Motor...");
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  
  ledcSetup(pwmChannel, freq, resolution);
  
  ledcAttachPin(enable1Pin, pwmChannel);

  InitWiFi();
  setDuty(dutyCycle); 
  motorForward();
}

void loop() 
{

  if (WiFi.status() != WL_CONNECTED)
  {
    reconnect();
  }

  if (!tb.connected())
  {
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) 
    {
      Serial.println("Failed to connect");
      return;
    }

    if (!subscribed) 
    {
      Serial.println("Subscribing for RPC...");
      if (!tb.RPC_Subscribe(callbacks, callbacks_size)) 
      {
        Serial.println("Failed to subscribe for RPC");
        return;
      }
  
      Serial.println("Subscribe done");
      subscribed = true;
    }
  } 
 // delay(1000);
  tb.loop();
}

void setDuty(int dc)
{

  ledcWrite(pwmChannel, dc);
  Serial.println("Received Speed is ");
  Serial.println(dc);
  if((dc == 0)||(curState==0))
  {
    motorStop();
    tb.sendTelemetryInt("State", 0);  //motorEdgeSpeed
    RPC_Response("motorEdgeSpeed", 0);   
  }
  else if((dc > 0)&&(curState==2))
  {
    motorBackward();
  }
  else if((dc > 0)&&(curState==1))
  {
    motorForward();
  }
}

void motorForward()
{
  Serial.println("Moving Forward");
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH); 
}

void motorBackward()
{
  Serial.println("Moving Backwards");
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW); 
}

void motorStop()
{
  Serial.println("Motor stopped");
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  delay(10);
}
