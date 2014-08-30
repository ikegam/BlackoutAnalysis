#include <Wire.h>
#include <RTClib_2.h>
#include <Time.h>
#include <SD.h>

#include <MsTimer2.h>
#include <avr/wdt.h> // WatchDog Timer

File myFile;

RTC_RTC8564 RTC;

// センサ測定値保持フィールド
long current_sampling_summary0=0;
long current_sampling_summary1=0;
long current_sampling_summary2=0;
long current_sampling_summary3=0;

// 100 = 1A
float current_current_result0=0;
float current_current_result1=0;
float current_current_result2=0;
float current_current_result3=0;

#define LED_GRN 13

char* digitalClockDisplay()
{
  static char date[128];
  now();
  sprintf(date, "%d/%02d/%02dT%02d:%02d:%02d",year(), month(), day(), hour(), minute(), second());
  return date;
}

void led_set(boolean g, boolean r)
{
  if(g){ digitalWrite(LED_GRN, HIGH); }else{ digitalWrite(LED_GRN, LOW); }
}

int p0;
int p1;
int p2;
int p3;

float writebuf0[10];
float writebuf1[10];
float writebuf2[10];
float writebuf3[10];

// 電流のサンプリングを行う (1ms毎に呼び出される)
void power_sampling()
{
  static unsigned int current_sampling_counter=0;
  //int zero=0;
  //int hundred=100;
  //float c1k=97.92;
  //float c3k=33.87;
  static float c1k=1.0;
  static float c3k=1.0;
  int tmp=0;
  int ref=511;
  p0=analogRead(0);
  p1=analogRead(1);
  p2=analogRead(2);
  p3=analogRead(3);
  // A4 and A5 are for I2C
  // 165

  tmp=(int)(p0-ref); current_sampling_summary0+=tmp*tmp;
  tmp=(int)(p1-ref); current_sampling_summary1+=tmp*tmp;
  tmp=(int)(p2-ref); current_sampling_summary2+=tmp*tmp;
  tmp=(int)(p3-ref); current_sampling_summary3+=tmp*tmp;

  if ((++current_sampling_counter)>=100) {
    current_sampling_counter=0;
    current_current_result0=(sqrt(current_sampling_summary0/100)*c1k);
    current_current_result1=(sqrt(current_sampling_summary1/100)*c1k);
    current_current_result2=(sqrt(current_sampling_summary2/100)*c3k);
    current_current_result3=(sqrt(current_sampling_summary3/100)*c3k);
    current_sampling_summary0=0;
    current_sampling_summary1=0;
    current_sampling_summary2=0;
    current_sampling_summary3=0;

    myFile.print(digitalClockDisplay());
    myFile.print(String("") + "," + (millis()) + ",");
    myFile.print(current_current_result0);
    myFile.print(",");
    myFile.print(current_current_result1);
    myFile.print(",");
    myFile.print(current_current_result2);
    myFile.print(",");
    myFile.println(current_current_result3);
  }
}

void save()
{
  // logging to Serial
  Serial.print(digitalClockDisplay());
  Serial.print(String("") + "," + (millis()) + ",");
  Serial.print(current_current_result0);
  Serial.print(",");
  Serial.print(current_current_result1);
  Serial.print(",");
  Serial.print(current_current_result2);
  Serial.print(",");
  Serial.print(current_current_result3);
  Serial.print(",");
  Serial.print(p0);
  Serial.print(",");
  Serial.print(p1);
  Serial.print(",");
  Serial.print(p2);
  Serial.print(",");
  Serial.println(p3);

  myFile.flush();
}

unsigned long time_sync()
{
  wdt_reset();
  return RTC.now().unixtime();
}

// 初期設定ルーチン
void setup()
{
  Serial.begin(9600);
  pinMode(LED_GRN, OUTPUT);
  analogReference(EXTERNAL);

   // WDT開始
  wdt_enable(WDTO_8S);
  wdt_reset();

  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);
  if (!SD.begin(8)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  Wire.begin();
  RTC.begin();
  wdt_reset();

  delay(1000);
  wdt_reset();

  setSyncProvider(time_sync);

  Serial.println("Marking...");
  myFile = SD.open("reset.txt", FILE_WRITE);
  myFile.print(digitalClockDisplay());
  myFile.println(String("") + "," + (millis()));
  myFile.flush();
  myFile.close();

  myFile = SD.open("log.csv", FILE_WRITE);
  wdt_reset();

  MsTimer2::set(1, power_sampling);
  MsTimer2::start();
}

void loop()
{
  int i;

  for (i=0; i<10; i++) {
    wdt_reset();
    delay(100);
  }
  led_set(true, true);

  wdt_reset();
  MsTimer2::stop();
  save();
  MsTimer2::start();
  led_set(false, false);

}

