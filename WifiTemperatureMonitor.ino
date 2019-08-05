//Libraries
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <SoftwareSerial.h>


// DHT22 (Temp Sensor) Initializations
#define DHTPIN 7     // DATA Pin connection
#define DHTTYPE DHT22   // DHT 22  Temperature Sensor 
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

// ESP8266 Wifi Module Intializations
#define RX 11 //Define Pin 11 as RX TO Arduino FROM WIfi Module
#define TX 10 //Define Pin 10 as TX FROM Arduino TO WIFI Module


// ESP8266 Wifi Variables
String AP = "INSERT YOUR ACCESS POINT NAME";
String PWD = "INSERT YOUR ACCESS POINT PWD";

String APIKey = "INSERT YOUR API KEY FROM THINGS SPEAK";


int ReceiveSuccessCount;
bool ExpectedDataReceive = false;
bool WifiInitDone = false;

SoftwareSerial esp8266(RX,TX);




//Temperature Sensor Variables
float hum;  //Stores humidity value
float temp; //Stores temperature value

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
    digitalWrite(4, HIGH); 
    if(WifiInitDone == false)
    {
      WifiInitDone = WifiInit();
    }

    WifiInitDone = WifiSendTemperatureData();
    digitalWrite(4, LOW);
    delay(60000); //Get Temperature Point every minute

  }
      
}


//WifiInit - Initializes wifimodule
//Returns true if wifi initialization is successfull.
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

bool WifiSendTemperatureData()
{
   bool WifiSendSuccess = false; 
   bool CIPStartSuccess = false;
   bool CIPSendSuccess = false;
   bool SendSuccess = false;
   
   GetTemperatureHumidity();
   String Header = "GET /update?key="+ APIKey +"&field1="+ String(temp) + "&field2="+ String(hum);
   String HeaderLength = String(Header.length()+2);

   CIPStartSuccess = SendToWifiModule("AT+CIPSTART=\"TCP\",\"184.106.153.149\",80\r\n",3000, "CONNECT");
   CIPSendSuccess = SendToWifiModule("AT+CIPSEND="+ HeaderLength + "\r\n",1000,">");
   SendSuccess = SendToWifiModule(Header + "\r\n",20000,"+IPD");

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


//GetTemperatureHumidity - Gets temperature and humidty data from DHT22
//Temperature is stored in the temp global variable
//Humidity is store in the hum temp global variable
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


//SendToWifiModule - sends AT Command to wifi module and verifies if expected output was received
//Arguments: String command - AT Command to send to wifi module
//           int DelayBeforeReading - Time to wait before checking response from module
//           char ExpectedOutput[] = ExpectedOutput from Module.

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
   
   //#DEBUG Serial.println(esp8266.readString()); 
    if(esp8266.find(ExpectedOutput)) 
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


   
