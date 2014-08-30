#include <Wire.h>
#include <RTClib_2.h>
#include <Time.h>
#include <SD.h>

#include <MsTimer2.h>
#include <avr/wdt.h> // WatchDog Timer

File myFile;
RTC_RTC8564 RTC;

unsigned long current_sampling_summary0=0;
unsigned long current_sampling_summary1=0;
unsigned long current_sampling_summary2=0;
unsigned long current_sampling_summary3=0;

int p0;
int p1;
int p2;
int p3;

float writebuf0[10];
float writebuf1[10];
float writebuf2[10];
float writebuf3[10];

unsigned char writebuf_counter = 0;

#define LED_GRN 13

char* date_to_str()
{
  static char date[128];
  now();
  sprintf(date, "%d/%02d/%02dT%02d:%02d:%02d",year(), month(), day(), hour(), minute(), second());
  return date;
}

void led_toggle()
{
  static boolean g = true;
  if (g) {
    digitalWrite(LED_GRN, HIGH);
    g = false;
  } else {
    digitalWrite(LED_GRN, LOW);
    g = true;
  }
}

void power_sampling()
{
  static unsigned int current_sampling_counter=0;
  static float c1k=1;
  static float c3k=3; // hack here
  long tmp=0;
  int ref=511;
  p0=analogRead(0);
  p1=analogRead(1);
  p2=analogRead(2);
  p3=analogRead(3);
  // A4 and A5 are for I2C

  tmp=(int)(p0-ref); current_sampling_summary0+=tmp*tmp;
  tmp=(int)(p1-ref); current_sampling_summary1+=tmp*tmp;
  tmp=(int)(p2-ref); current_sampling_summary2+=tmp*tmp;
  tmp=(int)(p3-ref); current_sampling_summary3+=tmp*tmp;

  if ((++current_sampling_counter)>=100) {
    current_sampling_counter=0;
    writebuf0[writebuf_counter]=(sqrt(current_sampling_summary0/100)*c1k);
    writebuf1[writebuf_counter]=(sqrt(current_sampling_summary1/100)*c1k);
    writebuf2[writebuf_counter]=(sqrt(current_sampling_summary2/100)*c3k);
    writebuf3[writebuf_counter]=(sqrt(current_sampling_summary3/100)*c3k);
    writebuf_counter++;
    current_sampling_summary0=0;
    current_sampling_summary1=0;
    current_sampling_summary2=0;
    current_sampling_summary3=0;
  }
}

void save()
{
  char *date;
  unsigned char i;
  float summary[4] = {};

  // logging to Serial
  date = date_to_str();

  myFile = SD.open("log.csv", FILE_WRITE);
  if (!myFile) {
    Serial.println("Open failed.");
    goto halted;
  }

  for (i=0; i<writebuf_counter; i++) {
    myFile.print(date);
    myFile.print(",");
    myFile.print(i);
    myFile.print(",");
    myFile.print(writebuf0[i]);
    myFile.print(",");
    myFile.print(writebuf1[i]);
    myFile.print(",");
    myFile.print(writebuf2[i]);
    myFile.print(",");
    myFile.print(writebuf3[i]);
    myFile.println();
    summary[0] += writebuf0[i];
    summary[1] += writebuf1[i];
    summary[2] += writebuf2[i];
    summary[3] += writebuf3[i];
  }

  Serial.print(date);
  Serial.print(",");
  Serial.print(i);
  Serial.print(",");
  Serial.print(summary[0]/(float)writebuf_counter);
  Serial.print(",");
  Serial.print(summary[1]/(float)writebuf_counter);
  Serial.print(",");
  Serial.print(summary[2]/(float)writebuf_counter);
  Serial.print(",");
  Serial.print(summary[3]/(float)writebuf_counter);
  Serial.print(",");
  Serial.print(p0);
  Serial.print(",");
  Serial.print(p1);
  Serial.print(",");
  Serial.print(p2);
  Serial.print(",");
  Serial.print(p3);
  Serial.println();

  myFile.flush();
  myFile.close();
  writebuf_counter = 0;
  return;

halted:
  Serial.println("System halted");
  while(1) {}
}

unsigned long time_sync()
{
  wdt_reset();
  return RTC.now().unixtime();
}

// 初期設定ルーチン
void setup()
{
  Serial.begin(115200);
  pinMode(LED_GRN, OUTPUT);
  analogReference(EXTERNAL);

   // WDT開始
  wdt_enable(WDTO_8S);
  wdt_reset();

  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);
  if (!SD.begin(8)) {
    Serial.println("failed!");
    goto halted;
  }
  Serial.println("done.");

  Serial.print("Initializing I2C...");
  Wire.begin();
  Serial.println("done.");

  Serial.print("Initializing RTC...");
  RTC.begin();
  Serial.println("done.");

  wdt_reset();

  delay(1000);
  wdt_reset();

  setSyncProvider(time_sync);

  Serial.print("Marking...");
  myFile = SD.open("reset.txt", FILE_WRITE);
  if (!myFile) {
    Serial.println("failed!");
    goto halted;
  }
  myFile.print(date_to_str());
  myFile.println(String("") + "," + (millis()));
  myFile.flush();
  myFile.close();
  Serial.println("done.");

  wdt_reset();

  MsTimer2::set(1, power_sampling);
  MsTimer2::start();
  return;

halted:
  Serial.println("System halted");
  while(1) {}
}

void loop()
{
  delay(1000);
  led_toggle();
  wdt_reset();

  MsTimer2::stop();
  save();
  MsTimer2::start();
  wdt_reset();
}
