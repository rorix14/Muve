#include <WiFi.h>

// Connection protocol, '#' signifies the beginning of a message and '|' signifies the end of a message
// Network related variables
const char* ssid = "";  //Network name
const char* password = "";  //Network password
const uint16_t port = 7748;//your port number ex: 7748
const char* hostIP = "";  //IP of the host (computer running the synthesizer) ex: 192.168.2.50

// Pins connected to the Accelerometer
const byte xPin = 1;
const byte yPin = 2;
const byte zPin = 3;

// Min Max voltage read values
const int RawMin = 0;
const int RawMax = 8192; // Analog to digital conveter is 13 bits

const int vectorLength = 3;
const int sampleSize = 10;

float lastVector[vectorLength] = {0.0, 0.0, 0.0};

// testing variables
int timeAgragate = 0;
int slow = 0;
int midSlow = 0;
int mid = 0;
int midFast = 0;
int fast = 0;

const bool testing = false; // Enable some test prints

void setup()
{
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Trying to connect...");
  }

  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());

  lastVector[0] = map(ReadAxis(xPin), RawMin, RawMax, -3000, 3000) / 1000.0;
  lastVector[1] = map(ReadAxis(yPin), RawMin, RawMax, -3000, 3000) / 1000.0;
  lastVector[2] = map(ReadAxis(zPin), RawMin, RawMax, -3000, 3000) / 1000.0;

  Serial.println("Program started");
}

void loop()
{
  WiFiClient client;

  if (!client.connect(hostIP, port))
  {
    Serial.println("Connection to host failed");
    delay(1000);
    return;
  }

  Serial.println("Connected to server seccessful!");
  while (client.connected())
  {
    float currentVector[vectorLength];
    currentVector[0] = map(ReadAxis(xPin), RawMin, RawMax, -3000, 3000) / 1000.0;
    currentVector[1] = map(ReadAxis(yPin), RawMin, RawMax, -3000, 3000) / 1000.0;
    currentVector[2] = map(ReadAxis(zPin), RawMin, RawMax, -3000, 3000) / 1000.0;

    client.print(MakeMesage(ProcessAccelerometerResults(currentVector)));  // could also try the write function

    if (testing)
      Serial.println("Message Sent..");

    delay(200);
  }

  Serial.println("Disconnecting...");
  client.stop();
  delay(1000);
}

long ReadAxis(const byte& axisPin)
{
  long reading = 0;
  delay(1);

  for (int i = 0; i < sampleSize; i++)
    reading += analogRead(axisPin);

  return reading / sampleSize;
}

int ProcessAccelerometerResults(const float* currentVector)
{
  int result = 0;
  int resultVector[vectorLength] = {0, 0, 0};

  for (int i = 0; i < vectorLength; i++)
  {
    if (lastVector[i] + 0.1 > currentVector[i] && lastVector[i] - 0.1 < currentVector[i])
      resultVector[i] = 0;
    else if (lastVector[i] + 0.35 > currentVector[i] && lastVector[i] - 0.35 < currentVector[i])
      resultVector[i] = 1;
    else if (lastVector[i] + 0.6 > currentVector[i] && lastVector[i] - 0.6 < currentVector[i])
      resultVector[i] = 2;
    else if (lastVector[i] + 0.9 > currentVector[i] && lastVector[i] - 0.9 < currentVector[i])
      resultVector[i] = 3;
    else
      resultVector[i] = 4;

    lastVector[i] = currentVector[i];

    if (result < resultVector[i])
      result = resultVector[i];
  }

  //if (testing)
    testWhoIsBigger(result);
    
  return result;
}

String MakeMesage(const int& value)
{
  String message = "#";
  message += value;
  message += "|";
  return message;
}

// Functions used for testing
void testWhoIsBigger(int result)
{
  if (timeAgragate >= 2000)
  {
    if (slow > midSlow && slow > mid && slow > midFast && slow > fast)
    {
      Serial.println("SLOW!");
    }
    else if (midSlow >= slow && midSlow >= mid && midSlow >= midFast && midSlow >= fast)
    {
      Serial.println("MID SLOW!");
    }
    else if (mid >= slow && mid >= midSlow && mid >= midFast && mid >= fast)
    {
      Serial.println("MID!");
    }
    else if (midFast >= slow && midFast >= midSlow && midFast >= mid && midFast >= fast)
    {
      Serial.println("MID FAST!");
    }
    else if (fast >= slow && fast >= midSlow && fast >= mid && fast >= midFast)
    {
      Serial.println("FAST!");
    }
    else
      Serial.println("UNDEFINED..");

    timeAgragate = 0;
    slow = 0;
    midSlow = 0;
    mid = 0;
    midFast = 0;
    fast = 0;
  }

  timeAgragate += 200;
  switch (result) {
    case 0:
      slow++;
      break;
    case 1:
      midSlow++;
      break;
    case 2:
      mid++;
      break;
    case 3:
      midFast++;
      break;
    case 4:
      fast++;
      break;
  }
}

void PrintAccelerometerResults(const float* vector)
{
  Serial.print("x: "); Serial.print(vector[0]); Serial.print("\t");
  Serial.print("y: "); Serial.print(vector[1]); Serial.print("\t");
  Serial.print("z: "); Serial.print(vector[2]); Serial.println();
}

// Old way of for x, y, z pin values.
/*
  int x = analogRead(xPin); //read from xpin
  delay(1); //
  int y = analogRead(yPin); //read from ypin
  delay(1);
  int z = analogRead(zPin); //read from zpin

  Serial.print("x: ");
  Serial.print(((float)x - 331.5) / 65 * 9.8);
  Serial.print("\t");
  Serial.print("y: ");
  Serial.print(((float)y - 329.5) / 68.5 * 9.8);
  Serial.print("\t");
  Serial.print("z: ");
  Serial.print(((float)z - 340) / 68 * 9.8);
  Serial.println();
*/
