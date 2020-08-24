// Start Work may 2019
// Rework Start on 15-08-2020
// A0 - Hour Tense , A1 - Hour Once ,  A2 - Min Tense , A3 - Min Once
// 595 Pin - 1-dp, 2-a, 3-b, 4-f, 5-g, 6-d, 7-e, 8-c,
// if Hour and Minute segment vertically oposite then Seg g and dp inter changed
 
#include <TimerOne.h>
// #include "TimerOne.h"

#include <Wire.h>
#include <RtcDS3231.h> // This Library Will Work With Mega 328 only
RtcDS3231<TwoWire> Rtc(Wire);

//#include <RtcDS1307.h>    // This Library Will Work With Mega 8 also
//RtcDS1307<TwoWire> Rtc(Wire);
/* for normal hardware wire use above */

#include <EEPROM.h> ////   /* EEPROM.write(addr, val);, value = EEPROM.read(address); */

/* to work with timer on astmega 8
/* https://github.com/rajasekarsarangapani/TimerOne-Atmega8/blob/master/TimerOne.h */

/* for normal hardware wire use above
// CONNECTIONS:
// DS1307 SDA --> SDA --> A5
// DS1307 SCL --> SCL --> A4
// DS1307 VCC --> 5v
// DS1307 GND --> GND
*/

//=========================================//
//============== Mode Configs =============//
#define NORMAL 1
#define SETTIME 2
#define SETALARMON 3
#define SETALARMOFF 4
byte currentMode = NORMAL;
//=========================================//

#define countof(a) (sizeof(a) / sizeof(a[0]))

//Define 74HC595 Connections with arduino
#define Data 2  // Pin 14 of 595
#define Clock 4 // Pin 11 of 595
#define Latch 3 // Pin 12 of 595

#define SEG0 A0
#define SEG1 A1
#define SEG2 A2
#define SEG3 A3

#define btn1 6   //
#define btn2 7   //
#define btn3 8   //
#define relay 9
#define led1 10   //
#define led2 11   //
#define led3 12   //

byte cc = 0;
byte Value[4];
byte button1_time_press_counter = 0;

volatile int counter = 0;
volatile bool dotIndicator = false;

int alarm_On_Hour = EEPROM.read(01);
int alarm_On_Minute = EEPROM.read(02);
int alarm_Off_Hour = EEPROM.read(03);
int alarm_Off_Minute = EEPROM.read(04);

// Some Old Data
//const char SegDataMinutes[] = {0x12, 0xBB, 0x4A, 0x2A, 0xA3, 0x26, 0x06, 0xBA, 0x02, 0x22};
//const char SegDataHours[] = {0x12, 0xBB, 0X58, 0x38, 0xB1, 0x34, 0x14, 0xBA, 0x10, 0x30};
// Com Anode
//const char SegDataMinutes[] = {0x22, 0xF3, 0x46, 0xC2, 0x93, 0x8A, 0x0A, 0xE3, 0x02, 0x82};
//const char SegDataHours[] = {0x22, 0xF3, 0x46, 0xC2, 0x93, 0x8A, 0x0A, 0xE3, 0x02, 0x82};
// Com Cathode // 01-h-dp,
               // 01-m-g,
const char SegDataMinutes[] = {0xFE, 0x21, 0xF6, 0xED, 0xB1, 0x00, 0x04, 0x00, 0x00, 0x00};
const char SegDataHours[] = {0xEF, 0x21, 0x6E, 0x6D, 0x39, 0x00, 0x0C, 0x00, 0x00, 0x00};
//                            0     1      2     3     4     5     6
void setup()
{

    Rtc.Begin();

    pinMode(Data, OUTPUT);
    pinMode(Clock, OUTPUT);
    pinMode(Latch, OUTPUT);
    pinMode(SEG0, OUTPUT);
    pinMode(SEG1, OUTPUT);
    pinMode(SEG2, OUTPUT);
    pinMode(SEG3, OUTPUT);

    pinMode(relay, OUTPUT);

    pinMode(led1, OUTPUT);
    pinMode(led2, OUTPUT);
    pinMode(led3, OUTPUT);

    setLeds(HIGH, HIGH, HIGH);

    pinMode(btn1, INPUT_PULLUP);
    pinMode(btn2, INPUT_PULLUP);
    pinMode(btn3, INPUT_PULLUP);

    //Initialize Display Scanner
    cc = 0;
    Timer1.initialize(2000);
    Timer1.attachInterrupt(timerIsr); // attach the service routine here
}

void loop()
{
    if (currentMode == NORMAL)
        NormalMode();
    else if (currentMode == SETTIME)
        SetTimeMode();
    else if (currentMode == SETALARMON)
        SetAlarmOnMode();
    else if (currentMode == SETALARMOFF)
        SetAlarmOffMode();

    if (digitalRead(btn1) == HIGH)
    {
        button1_time_press_counter = 0;
    }
}

//********************************************************************

void setLeds(int val1, int val2, int val3)
{
    digitalWrite(led1, val1);
    digitalWrite(led2, val2);
    digitalWrite(led3, val3);
}

void NormalMode()
{
    //=============================================
    //Get Current time section

    RtcDateTime now = Rtc.GetDateTime();
    int hours = now.Hour();
    int minutes = now.Minute();
    if (digitalRead(btn2) == LOW)
    {
        SetHoursValue(alarm_On_Hour);
        SetMinutesValue(alarm_On_Minute);
    }
    else if (digitalRead(btn3) == LOW)
    {
        SetHoursValue(alarm_Off_Hour);
        SetMinutesValue(alarm_Off_Minute);
    }
    else
    {
        SetHoursValue(hours);
        SetMinutesValue(minutes);
    }
    delay(1);

    //=============================================
    // alarm section

    if (hours >= alarm_On_Hour && minutes >= alarm_On_Minute)
    {
        digitalWrite(relay, HIGH);
    }
    if (hours >= alarm_Off_Hour && minutes >= alarm_Off_Minute)
    {
        digitalWrite(relay, LOW);
    }

    //=============================================
    // Mode Change Event
    // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
    if (button1_time_press_counter > 10)
    {
        currentMode = SETTIME;
        setLeds(LOW, HIGH, HIGH);
    }
}

void SetTimeMode()
{
    if (digitalRead(btn1) == LOW && button1_time_press_counter < 10)
    {
        currentMode = SETALARMON;
        SetRTCTime();
        delay(1000);
        setLeds(HIGH, LOW, HIGH);
    }
    if (digitalRead(btn3) == LOW)
    {
        SetHoursValue(inc_hrs(Value[0] * 10 + Value[1]));
        delay(500);
    }

    if (digitalRead(btn2) == LOW)
    {
        SetMinutesValue(inc_min(Value[2] * 10 + Value[3]));
        delay(500);
    }
    delay(1);
}

void SetAlarmOnMode()
{
    SetHoursValue(alarm_On_Hour);
    SetMinutesValue(alarm_On_Minute);

    if (digitalRead(btn1) == LOW)
    {
        EEPROM.write(01, alarm_On_Hour);
        EEPROM.write(02, alarm_On_Minute);
        currentMode = SETALARMOFF;
        delay(1000);
        setLeds(HIGH, HIGH, LOW);
    }
    if (digitalRead(btn3) == LOW)
    {
        alarm_On_Hour = inc_hrs(alarm_On_Hour);
        SetHoursValue(alarm_On_Hour);
        delay(500);
    }

    if (digitalRead(btn2) == LOW)
    {
        alarm_On_Minute = inc_min(alarm_On_Minute);
        SetMinutesValue(alarm_On_Minute);
        delay(500);
    }
    delay(1);
}

void SetAlarmOffMode()
{
    SetHoursValue(alarm_Off_Hour);
    SetMinutesValue(alarm_Off_Minute);

    if (digitalRead(btn1) == LOW)
    {
        EEPROM.write(03, alarm_Off_Hour);
        EEPROM.write(04, alarm_Off_Minute);
        currentMode = NORMAL;
        delay(1000);
        setLeds(HIGH, HIGH, HIGH);
    }

    if (digitalRead(btn3) == LOW)
    {
        alarm_Off_Hour = inc_hrs(alarm_Off_Hour);
        SetHoursValue(alarm_Off_Hour);
        delay(500);
    }

    if (digitalRead(btn2) == LOW)
    {
        alarm_Off_Minute = inc_min(alarm_Off_Minute);
        SetMinutesValue(alarm_Off_Minute);
        delay(500);
    }
    delay(1);
}
int inc_hrs(int hrs)
{
    hrs++;
    if (hrs > 23)
        hrs = 0;
    return hrs;
}

int inc_min(int min)
{
    min++;
    if (min > 59)
        min = 0;
    return min;
}

void SetHoursValue(int hours)
{
    if (hours < 10)
    {
        Value[0] = hours / 10;
        Value[1] = hours;
    }
    else if (hours >= 10 && hours < 24)
    {
        Value[0] = hours / 10;
        Value[1] = hours % 10;
    }

    Value[0] = Value[0] & 0x0F; //Anding with 0x0F to remove upper nibble
    Value[1] = Value[1] & 0x0F; //Ex. number 2 in ASCII is 0x32 we want only 2
}

void SetMinutesValue(int minutes)
{
    if (minutes < 10)
    {
        Value[2] = minutes / 10;
        Value[3] = minutes;
    }

    else if (minutes >= 10 && minutes < 60)
    {
        Value[2] = minutes / 10;
        Value[3] = minutes % 10;
    }

    Value[2] = Value[2] & 0x0F;
    Value[3] = Value[3] & 0x0F;
}

void SetRTCTime()
{
    char timeString[8];
    sprintf(timeString, "%01u%01u:%01u%01u:00",
            Value[0],
            Value[1],
            Value[2],
            Value[3]);
    RtcDateTime setable = RtcDateTime(__DATE__, timeString);
    Rtc.SetDateTime(setable);
}

//===============================================

void DisplayDigit(char d)
{
    int i;

    for (i = 0; i < 8; i++) //Shift bit by bit data in shift register
    {
        if ((d & 0x80) == 0x80)
        {
            digitalWrite(Data, HIGH);//
        }
        else
        {
            digitalWrite(Data, LOW);//
        }
        d = d << 1;

        //Give Clock pulse
        digitalWrite(Clock, LOW);
        digitalWrite(Clock, HIGH);
    }
    //Latch the data
    digitalWrite(Latch, LOW);
    digitalWrite(Latch, HIGH);
}
//===============================================
//    TIMER 1 OVERFLOW INTTERRUPT FOR DISPALY
//===============================================
void timerIsr()
{
    cc++;
    if (cc == 5) //We have only 4 digits
    {
        cc = 1;
    }
    Scanner();
    TCNT0 = 0xCC;
}

//===============================================
//    SCAN DISPLAY FUNCTION
//===============================================
void Scanner()
{
    switch (cc) //Depending on which digit is selcted give output
    {
    case 1:
        digitalWrite(SEG3, LOW);
        if (currentMode == NORMAL)
        {
            DisplayDigit(SegDataHours[Value[0]]);
        }
        else 
        {
            DisplayDigit(SegDataHours[Value[0]] & 0xFD);
        }
        digitalWrite(SEG0, HIGH);
        break;
    case 2:
        digitalWrite(SEG0, LOW);
        if (currentMode == NORMAL)
        {
            if (dotIndicator == true)
                DisplayDigit(SegDataHours[Value[1]] & 0xFD); //to turn on decimal point
            else
                DisplayDigit(SegDataHours[Value[1]]);
        }
        else
            DisplayDigit(SegDataHours[Value[1]]);
        digitalWrite(SEG1, HIGH);
        break;
    case 3:
        digitalWrite(SEG1, LOW);
        /*if (currentMode == NORMAL)
        {
             if (dotIndicator == true)
                DisplayDigit(SegDataMinutes[Value[2]] & 0xFD);
            else
                DisplayDigit(SegDataMinutes[Value[2]]); 
        }
        else
            DisplayDigit(SegDataMinutes[Value[2]] & 0xFD);*/
        DisplayDigit(SegDataMinutes[Value[2]]); 
        digitalWrite(SEG2, HIGH);
        break;
    case 4:
        digitalWrite(SEG2, LOW);
        DisplayDigit(SegDataMinutes[Value[3]]);
        digitalWrite(SEG3, HIGH);
        break;
    }
    counter++;
    if (counter >= 500)
    {
        counter = 0;
        dotIndicator = !dotIndicator;

        if (digitalRead(btn1) == LOW)
        {
            button1_time_press_counter++;
        }
    }
}
//==============================================
