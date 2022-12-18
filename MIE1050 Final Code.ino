#include <Wire.h>
#include <SPI.h>

#include <Adafruit_ADS1015.h>   
#include <Adafruit_GFX.h>     
#include <Adafruit_Si7021.h>
#include <Adafruit_SSD1306.h>
#include <afstandssensor.h>
#include <movingAvg.h>

float t = 0, T = 0, HI = 0,feelslike = 0, rh = 0, rh1 = 0, db_sound = 0, avg_sound = 0, RH = 0, d1 = 0,d2 = 0, mos1 = 0, mos2 = 0, mic = 0, output = 0, PeakToPeak = 0, Decibels = 0, ac = 0, prESP = 0, mos1ESP = 0, mos2ESP = 0, acESP = 0, micESP = 0;
const uint8_t IO_PR = 15, IO_AC = 33, IO_MIC = 14, IO_MOS1 = 16, IO_MOS2 = 7, IO_BUZZER = 25, IO_LED = 32, CHANNEL_BUZZER = 1, CHANNEL_LED = 2; 
unsigned long runtime = 0;
uint16_t interval = 1000;
bool occupancy_state = false;

Adafruit_SSD1306 display(128, 64, &Wire, -1);   //128x64 OLED Display - Using default I2C - No reset pin (-1)
Adafruit_ADS1015 ads;                           //4-Channel Analog to Digital Converter 10-bit resolution
AfstandsSensor hcsr04(13, 27);                  //HC-SR04 Ultrasonic Distance Sensor - Trigger pin connected to IO13 - Echo pin connected to IO27 
Adafruit_Si7021 si7021 = Adafruit_Si7021();     //Temperature & Humidity Sensor
movingAvg avgTemp(10);  
movingAvg avgSound(5); 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);                         //115200 is the baud rate. When reading from serial monitor, use the same baud rate
  pinMode(IO_AC, INPUT);                        //Defining all the IOs as either input or output
  pinMode(IO_PR, INPUT);
  pinMode(IO_MOS1, INPUT);
  pinMode(IO_MOS2, INPUT);
  pinMode(IO_MIC, INPUT);
  //pinMode(IO_LED, OUTPUT);
  pinMode(IO_BUZZER, OUTPUT);
  ledcSetup(CHANNEL_BUZZER, 3200, 8);           //ledcSetup(Channel(0-15), Frequency(Hz), Resolution) - 8 being 8-bit (0-255)
  ledcSetup(CHANNEL_LED, 3200, 8);
  ledcAttachPin(IO_BUZZER, CHANNEL_BUZZER);     //ledcAttachPin(IO, Channel) - Connect channel from ledcSetup to IO
  //ledcAttachPin(IO_LED, CHANNEL_LED);
  ledcWrite(CHANNEL_BUZZER, 0);                 //ledcWrite - Set duty cycle of channel to 0 - (0/255 = 0%)
  //ledcWrite(CHANNEL_LED, 0);


  ledcWrite(CHANNEL_BUZZER, 128);               //ledcWrite - Set duty cycle of channel to 128 - (128/255 = 50%)
  delay(500);                                   //delay(ms)
  ledcWrite(CHANNEL_BUZZER, 0);
  ledcWrite(CHANNEL_LED, 128);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);    //Initialize OLED - Use internal power source - I2C address = 0x3C
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  si7021.begin();                               //Initialize temperature & humidity sensor
  ads.begin();                                  //Initialize ADC
  avgTemp.begin();
  avgSound.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  while(millis() - runtime > interval){         //millis() returns milliseconds since start of program - interval is 1000ms - "if" would work the same way here
    runtime = millis();
    getReadings();
    if (occupancy_state = false){
      updateScreen_non_occ();
    }else{
      updateScreen_occ();
    }
    printReadings();
    //updateScreen_occ();
  }
}

void updateScreen_non_occ(){
    display.clearDisplay();                                       //Clears the display
    display.print("No People inside room");
    display.display();                                            //Show everything on the screen
  }
  
void updateScreen_occ(){
    display.clearDisplay();                                       //Clears the display
    display.setCursor(0,0);//Move to 0,0 on display (Top Left)
    
    /*display.print("T:"); display.print(t,1);                      //print("...") - prints out the characters between "" on the display
    display.print(" RH:"); display.print(rh,0);                  //print(variable, 1) - prints out the variable with 1 decimal
    display.print(" d:"); display.println(d2, 1);                  //println - prints and then moves to the next line
    display.print("DB sound:"); display.print(db_sound, 1);
    display.print(" M2:"); display.print(mos2, 1);
    display.print(" AC:"); display.println(ac, 1);
    display.print("M1ESP:"); display.print(mos1ESP, 0);
    display.print(" M2ESP: "); display.println(mos2ESP, 0);*/

    display.println();
    if (db_sound < 50) {
      display.println("Noise level ideal");
    }  else {
      display.println("Noise level not ideal");
    }  

    display.print("Feels like:"); display.print((HI - 32) * 5 / 9, 0);display.println("'C");
    if (prESP < 40) {
      display.print("Dark");
    } else if (prESP < 800) {
      display.print("Dim");
    } else if (prESP < 2000) {
      display.print("Light");
    } else if (prESP < 3200) {
      display.print("Bright");
    } else {
      display.print("Very Bright");
    }  
    display.display();                                            //Show everything on the screen
  }

void getReadings(){
  t = si7021.readTemperature() ;
  t = avgTemp.reading(t);
  T = (t * 1.8) + 32; //Request update to temperature register and read said register from the SI7021 IC
  
  
  rh = si7021.readHumidity();
  RH = 0.01 * (rh * 0.7083 + 12.739);                                    //Request update to humidity register and read said register from the SI7021 IC
  HI = -42.379 + 2.04901523*T + 10.14333127*RH - .22475541*T*RH - .00683783*T*T - .05481717*RH*RH + .00122874*T*T*RH + .00085282*T*RH*RH - .00000199*T*T*RH*RH;
  feelslike = -42.379 + 2.04901523*T + 10.14333127*RH - 0.22475541*T*RH - 0.00683783*T*T - 0.05481717*RH*RH - 0.00122874*T*T*RH + 0.00085282*T*RH*RH - 0.00000199*T*T*RH*RH;
  if (hcsr04.afstandCM(t) == -1)
  {
  d1=-1;}
  else{
  d1 = 10*((hcsr04.afstandCM(t)-0.7415)/1.0204);}                                         //(cm)Start pulse and measure how long it takes to return - Convert duration into distance - Use hcsr04.afstandCM(temperature) to adjust for temperature
  delay(10); 
  if (hcsr04.afstandCM(t) == -1)
  {
  d2=-1;}
  else{
  d2 = 10*((hcsr04.afstandCM(t)-0.7415)/1.0204);}  
  if (abs(d1 - d2) > 200){
      occupancy_state = false;
  }else{
    occupancy_state = true;
  }
  
  mos1 = ads.readADC_SingleEnded(0)*3.0;                          //Read from ADC channel 0 - Multiply by 3.0 to convert from ADC to mV - See singleended.ino example
  mos2 = ads.readADC_SingleEnded(1)*3.0;
  mic = ads.readADC_SingleEnded(2)*3.0;
  ac = ads.readADC_SingleEnded(3)*3.0;
  /*prESP = analogRead(IO_PR)/4095.0*3300.0;                        //Read from ESP32s ADC (12-bit (0-4095) - 3.3V max) - convert to mV
  acESP = analogRead(IO_AC)/4095.0*3300.0;*/
  prESP = analogRead(IO_PR);
  output = analogRead(IO_MIC)/4095.0*3300.0; 
  
  unsigned long startTime = micros();
  unsigned long interval = 100000; // 100ms
  unsigned long cnt = 0;
  while(micros() - startTime < interval)
  {
  cnt = cnt + 1;
  mic = mic + ads.readADC_SingleEnded(2)*3.0;
  }
  
  avg_sound = avgSound.reading(mic/cnt);
  db_sound = avg_sound*2.629-173.6;  //Convert to estimated db from mV
 
  mos1ESP = analogRead(IO_MOS1)/4095.0*3300.0;
  mos2ESP = analogRead(IO_MOS2)/4095.0*3300.0;

  
}
void printReadings(){
  Serial.print("\t");
  //Serial.print("dist: "); Serial.print(d2);                        //Read serial prints from serial monitor - Use same baud rate as above
  
  //Serial.print("\t");
  //Serial.print("\tMOS1: "); Serial.print(mos1);
  //Serial.print("\t");
  //Serial.print("\tMOS2: "); Serial.print(mos2);
  //Serial.print("\t");
  //Serial.print("\tMIC: ");     
  if (db_sound < 50) {
      Serial.print("Noise level ideal");
    }  else {
      Serial.print("Noise level not ideal");
    }
  Serial.print("\t");
  //Serial.print("\tAC: "); Serial.print(ac);
  //Serial.print("\t");
  //Serial.print("MV_t: "); Serial.print(T);
  //Serial.print("\t");
  //Serial.print("RH: "); Serial.println(rh);
  //Serial.print("Feels like:"); 
  Serial.print((HI - 32) * 5 / 9, 0);
  Serial.print("\t");
      if (prESP < 40) {
      Serial.print("Dark");
    } else if (prESP < 800) {
      Serial.print("Dim");
    } else if (prESP < 2000) {
      Serial.print("Light");
    } else if (prESP < 3200) {
      Serial.print("Bright");
    } else {
      Serial.print("Very Bright");
    }  

  Serial.println();
}
