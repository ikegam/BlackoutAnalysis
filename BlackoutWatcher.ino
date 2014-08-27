#include <Time.h>
#include <Wire.h>
#include <RTC8564.h>

#include <MsTimer2.h>
#include <avr/wdt.h> // WatchDog Timer


// センサ測定値保持フィールド
int current_sampling_counter=0;
float current_sampling_summary0=0;
float current_sampling_summary1=0;
float current_sampling_summary2=0;
float current_sampling_summary3=0;

// 100 = 1A
int current_current_result0=0;
int current_current_result1=0;
int current_current_result2=0;
int current_current_result3=0;

unsigned int voltage_setting=100;

// Calculated Watt (Demand)
long watt_summary0=0;
long watt_summary1=0;
long watt_summary2=0;
long watt_summary3=0;
int watt_counter=0;

unsigned short watt_va0; // VA0
unsigned short watt_va1; // VA1
unsigned short watt_va2; // VA2
unsigned short watt_va3; // VA3

unsigned short watt_vas0; // VAS0
unsigned short watt_vas1; // VAS1

#define LED_GRN 13

void led_set(boolean g, boolean r)
{
  if(g){ digitalWrite(LED_GRN, HIGH); }else{ digitalWrite(LED_GRN, LOW); }
//  if(r){ digitalWrite(LED_RED, HIGH); }else{ digitalWrite(LED_RED, LOW); }
}

int p0;
int p1;
int p2;
int p3;

// 電流のサンプリングを行う (1ms毎に呼び出される)
void power_sampling()
{
  //int zero=0;
  //int hundred=100;
  //float c1k=97.92;
  //float c3k=33.87;
  int c1k=10;
  int c3k=10;
  int tmp=0;
  int ref=511;
  p0=analogRead(0);
  p1=analogRead(1);
  p2=analogRead(2);
  p3=analogRead(3);
  // A4 and A5 are for I2C
  // 165

  //Serial.println(String("") + p0 + "," + p1 + "," + p2 + "," + p3);

  tmp=(int)(p0-ref); current_sampling_summary0+=tmp*tmp;
  tmp=(int)(p1-ref); current_sampling_summary1+=tmp*tmp;
  tmp=(int)(p2-ref); current_sampling_summary2+=tmp*tmp;
  tmp=(int)(p3-ref); current_sampling_summary3+=tmp*tmp;

  if ((++current_sampling_counter)>=100) {
    current_sampling_counter=0;
    current_current_result0=(int)(sqrt(current_sampling_summary0/100)*c1k);
    current_current_result1=(int)(sqrt(current_sampling_summary1/100)*c1k);
    current_current_result2=(int)(sqrt(current_sampling_summary2/100)*c3k);
    current_current_result3=(int)(sqrt(current_sampling_summary3/100)*c3k);
    current_sampling_summary0=0;
    current_sampling_summary1=0;
    current_sampling_summary2=0;
    current_sampling_summary3=0;
    watt_summary0+=(long)current_current_result0;
    watt_summary1+=(long)current_current_result1;
    watt_summary2+=(long)current_current_result2;
    watt_summary3+=(long)current_current_result3;
    watt_counter++;
  }

}

void save()
{
  //Serial.println(String("") + (p0 - p1) + "," + (p2 - p3));
  Serial.println(String("raw,") + p0 + "," + p1 + "," + p2 + "," + p3);
  Serial.println(String("100,") + current_current_result0 + "," + current_current_result1 + "," + current_current_result2 + "," + current_current_result3);
  Serial.println(String(".25,") + watt_va0 + "," + watt_va1 + "," + watt_va2 + "," + watt_va3);
}

void time_sync()
{
  //time_syncsetTime(Rtc.hours(), Rtc.minutes(), Rtc.seconds(), Rtc.days(), Rtc.months(), 0x2000+Rtc.years());
}

// 初期設定ルーチン
void setup()
{
  Serial.begin(9600);
  pinMode(LED_GRN, OUTPUT);
   // WDT開始
  wdt_enable(WDTO_8S);
  wdt_reset();

  setTime(0);
  time_sync();

  analogReference(EXTERNAL);

  MsTimer2::set(1, power_sampling);
  MsTimer2::start();
}

void loop()
{
  int i;

  if (watt_counter==0) {
    return ;
  }

  {
    MsTimer2::stop(); // この間 に データのキャプチャ(＆クリア)処理を行う (ここから)
    watt_va0=(unsigned short)(watt_summary0/watt_counter);
    watt_va1=(unsigned short)(watt_summary1/watt_counter);
    watt_va2=(unsigned short)(watt_summary2/watt_counter);
    watt_va3=(unsigned short)(watt_summary3/watt_counter);
    watt_vas0=(unsigned short)((watt_summary0+watt_summary1)/watt_counter);
    watt_vas1=(unsigned short)((watt_summary2+watt_summary3)/watt_counter);
    watt_summary0=0;
    watt_summary1=0;
    watt_summary2=0;
    watt_summary3=0;
    watt_counter=0;
    MsTimer2::start(); // この間 に データのキャプチャ(＆クリア)処理を行う (ここまで)
  }

  wdt_reset();
  save();

  led_set(false, false);
  for (i=0; i<5; i++) {
    wdt_reset();
    delay(50);
  }
  led_set(true, true);
  MsTimer2::stop();
  time_sync();
  MsTimer2::start();
}

