/**************************************************
* Wifi Temperature Monitor
*
* Author: Maitreya Panse
* 
*
* This is a simple program that samples Temperature
* and Humidity Data from the DHT22 Sensor and 
* sends it to ThingSpeak over Wifi. 
*
* 09/03/97 - Version 1.1 - Beluga
*   Changed temperature sampling time to 5 minutes
*   Cleaned up code - Added Comments as per 
*   Jack Ganssle standard. 
* 07/12/97 - Version 1.0 - Orca
*   Initial release
**************************************************/

/***************************************************/
/*Include Section*/
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SoftwareSerial.h>
/***************************************************/

/***************************************************/
/*Defines Section */
// DHT22 (Temp Sensor) Initializations
#define DHTPIN 7     // DATA Pin connection
#define DHTTYPE DHT22   // DHT 22  Temperature Sensor 

// ESP8266 Wifi Module Intializations
#define RX 11 //Define Pin 11 as RX TO Arduino FROM WIfi Module
#define TX 10 //Define Pin 10 as TX FROM Arduino TO WIFI Module
/***************************************************/


/***************************************************/
/*Global Variables Section */
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino
// ESP8266 Wifi Variables
String AP = "INSERT YOUR ACCCESS POINT HERE";
String PWD = "INSERT YOUR PASSWORD HERE";
String APIKey = "INSERT YOUR THINGSPEAK API KEY HERE";

int ReceiveSuccessCount;
bool ExpectedDataReceive = false;
bool WifiInitDone = false;

SoftwareSerial esp8266(RX,TX);

//Temperature Sensor Variables
float hum;  //Stores humidity value
float temp; //Stores temperature value

/***************************************************/

void setup()
{
  Serial.begin(9600); //Serial Monitor Debug Baudrate
  esp8266.begin(9600); //ESP8266 communication baudrate
  
  pinMode(4,OUTPUT); //LED Turns ON when RXing Temperature data DEBUG
  dht.begin(); //Initialize DHT22 Temperature sensor
  delay(3000);

}

void loop()
{
  
  while(1)
  {
    //Chip Enable on Wifi Module
    digitalWrite(4, HIGH); 
    delay(5000);
    if(WifiInitDone == false)
    {
      WifiInitDone = WifiInit();
    }
    if(WifiInitDone = true)
    {
        WifiInitDone = WifiSendTemperatureData();
        WifiInitDone = false;
    }
   
    digitalWrite(4, LOW);
    delay(300000); //Get Temperature Point every 5 minute

  }
      
}

/**************************************************
* WifiInit  : bool WifiInit()
*    returns    : return true if Wifi Initalization is successfull
* Created by    : Maitreya Panse
* Date created    : 05/08/2019
* Description   : Establishes communication with Wifi Module and connects to Access Point
**************************************************/
bool WifiInit()
{
  //Flags to keep track if commands are sent successfully
  bool InitSuccess = false; 
  bool ATSuccess = false;   
  bool CWSuccess = false;
  bool CIPMUXSuccess = false;
  bool ATAPSuccess = false;

  ATSuccess = SendToWifiModule("AT",1000,"OK");
  CWSuccess = SendToWifiModule("AT+CWMODE=1",1000,"OK");
  CWSuccess = SendToWifiModule("AT+CWMODE?",1000,"MODE:1");
  
  CIPMUXSuccess = SendToWifiModule("AT+CIPMUX=0",1000,"OK");
  CIPMUXSuccess = SendToWifiModule("AT+CIPMUX?",1000,"+CIPMUX:0");
  
  ATAPSuccess =  SendToWifiModule("AT+CWJAP=\""+ AP +"\",\"" + PWD + "\"",10000, "OK");
  ATAPSuccess = SendToWifiModule("AT+CWJAP?",2000,"SHAW");

  if(ATSuccess && CWSuccess &&  CIPMUXSuccess && ATAPSuccess)
  {
    Serial.println("Wifi Initialized Successfully");
    InitSuccess = true; 
  }
  else
  {
    InitSuccess = false;
    Serial.println("Wifi Initialization FAILED");
  }

  
  return InitSuccess;
}

/**************************************************
* WifiSendTemperatureData  : bool WifiSendTemperatureData()
*    returns    : return true if Temperature Data is successfully sent to ThingSpeak
* Created by    : Maitreya Panse
* Date created    : 05/08/2019
* Description   : Function opens TCP connection and sends temperature data to ThingSpeak
**************************************************/

bool WifiSendTemperatureData()
{
   bool WifiSendSuccess = false; 
   bool CIPStartSuccess = false;
   bool CIPSendSuccess = false;
   bool SendSuccess = false;
   
   GetTemperatureHumidity();
   String Header = "GET /update?key="+ APIKey +"&field1="+ String(temp) + "&field2="+ String(hum);
   String HeaderLength = String(Header.length()+1);

   CIPStartSuccess = SendToWifiModule("AT+CIPSTART=\"TCP\",\"184.106.153.149\",80\r\n",3000, "CONNECT");
   if(CIPStartSuccess == false)return false;
   
   CIPSendSuccess = SendToWifiModule("AT+CIPSEND="+ HeaderLength + "\r\n",1000,">");
   if(CIPSendSuccess == false) return false;
   
   SendSuccess = SendToWifiModule(Header + "\r\n",5000,"+IPD");

  if(CIPStartSuccess && CIPSendSuccess && SendSuccess)
  {
    WifiSendSuccess = true;  
    Serial.println("Data Sent Successfully");
  }

  else
  {
    WifiSendSuccess = false; 
    Serial.println("Data NOT Sent");
  }
   
   
   return WifiSendSuccess;
}


/**************************************************
* GetTemperatureHumidity  : void GetTemperatureHumidity()
* Created by    : Maitreya Panse
* Date created    : 05/08/2019
* Description   : Function gets temperature and humidity data from DHT22 sensor
**************************************************/
void GetTemperatureHumidity()
{
   bool GetDHT22DataSuccess = false; 
   int attempts = 0;

  while(GetDHT22DataSuccess == false || attempts > 10)
  {
    //Get Humidity and Temperature Data
    hum = dht.readHumidity();
    temp= dht.readTemperature();

   if(!isnan(hum) && !isnan(temp))
   {
      Serial.println("DHT22 Read Successful");
      GetDHT22DataSuccess = true;
   }

   else
   {
      Serial.println("DHT22 Read Unsuccessfull");
      GetDHT22DataSuccess = false;
      delay(5000);
   }
   
  }
   Serial.print("Humidity: ");
   Serial.print(hum);
   Serial.print(" %, Temp: ");
   Serial.print(temp);
   Serial.println(" Celsius");

}


/**************************************************
* SendToWifiModule  : bool SendToWifiModule(String command, int DelayBeforeReading, char ExpectedOutput)
*    returns    : return true if expected response is received from the wifi module
*    Command    : AT command to send to wifi module    
*    DelayBeforeReading : Time to wait before reading response from wifi module
*    ExpectedOutput : Expected response from WifiModule
* Created by    : Maitreya Panse
* Date created    : 05/08/2019
* Description   : Function sends AT commands to wifi module and checks if 
**************************************************/

bool SendToWifiModule(String command,int DelayBeforeReading, char ExpectedOutput[]) 
{
  
  int IterationCount = 0; //counter for number of retry iterations
  bool found = false; //flag for indicating when expected command is found
  String receivedData;
 
  
  
  while(IterationCount < 10)
  {
    delay(5000);
    Serial.print("Sending: ");
    Serial.println(command);
    esp8266.println(command); //Send AT Command to Wifi Module
    delay(DelayBeforeReading);
    receivedData = esp8266.readString();
    Serial.println(receivedData); 
    if(receivedData.indexOf(ExpectedOutput)>= 0) 
    { 
       Serial.print("RECEIVED EXPECTED RESPONSE ");
       Serial.println(ExpectedOutput);
       
      found = true;
      break;
    }  
    IterationCount = IterationCount + 1; 
  }

  if(found == true)
  {
    return true;
  }

  else
  {
    return false; 
  }
 
 }


   
