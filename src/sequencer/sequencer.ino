#include "synth.h"
#include "QuickStats.h"
#include <LiquidCrystal_I2C.h>

#include <usbh_midi.h>
#include <usbhub.h>
// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif

#include <SPI.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

int sensorValue = 0;
int bit1 = 8;
int bit2 = 9;
int bit3 = 10;

int value = 0;
byte mode = 0;
byte osc = 0;

int aPitch[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int aSet[4] = {0, 500, 64, 64};

int mySensVals[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int aReading[8];
int aState[8] = {1, 1, 1, 1, 1, 1, 1, 1};
int aPrevious[8] = {1, 1, 1, 1, 1, 1, 1, 1};
const int quant = 10;
float med[quant];
String aOsc[6] = {"SIN", "TRI", "SQR", "SAW", "RMP", "WAV"};
long interval = 50;
unsigned long previousMillis = 0;
int i = 0;

uint8_t knob1PrevValues[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t knob2PrevValues[8] = {0, 0, 0, 0, 0, 0, 0, 0};

synth edgar;
QuickStats stats;

USB Usb;
//USBHub Hub(&Usb);
USBH_MIDI Midi(&Usb);

void MIDI_poll();

uint16_t pid, vid;

void setup()
{
  lcd.init();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("bronaSynth v0.1");
  pinMode(bit1, OUTPUT);
  pinMode(bit2, OUTPUT);
  pinMode(bit3, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT_PULLUP);
  delay(500);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("INIT ...");
  delay(500);

  vid = pid = 0;
  Serial.begin(115200);

//  if (Usb.Init() == -1)
//  {
//    lcd.clear();
//    lcd.setCursor(1, 0);
//    lcd.print("USB ERROR !");
//
//    while (1)
//      ; //halt
//       }     //if (Usb.Init() == -1...

  delay(500);

  lcd.clear();
  edgar.begin(CHB);
}

// Poll USB MIDI Controler and send to serial MIDI
void MIDI_poll()
{
  char buf[20];
  uint8_t bufMidi[64];
  uint16_t rcvd;

  if (Midi.RecvData(&rcvd, bufMidi) == 0)
  {
    Serial.print("MIDI ");
    for (int i = 0; i < 64; i++)
    {
      sprintf(buf, " %02X", bufMidi[i]);
      Serial.print(buf);
    }
    Serial.println("");

    if ((bufMidi[1] == 0x80) || (bufMidi[1] == 0x90))
    {
      if ((bufMidi[1] == 0x90) && (bufMidi[3] > 0x00))
        playNote(bufMidi[2], bufMidi[3]);
      if ((bufMidi[1] == 0x80) || ((bufMidi[1] == 0x90) && (bufMidi[3] == 0x00)))
        stopNote(bufMidi[2], bufMidi[3]);
    }

    //Buttons  1 - 8
    if ((bufMidi[2] == 0x24) || (bufMidi[2] == 0x25) || (bufMidi[2] == 0x26) || (bufMidi[2] == 0x27) || (bufMidi[2] == 0x28) || (bufMidi[2] == 0x29) || (bufMidi[2] == 0x2a) || (bufMidi[2] == 0x2b))
    {
      Serial.println("BUTTON");
      //bufMidi[4] 0 - 7C

      int count = 0;

      if (bufMidi[2] == 0x24)
        count = 0;
      if (bufMidi[2] == 0x25)
        count = 1;
      if (bufMidi[2] == 0x26)
        count = 2;
      if (bufMidi[2] == 0x27)
        count = 3;
      if (bufMidi[2] == 0x28)
        count = 4;
      if (bufMidi[2] == 0x29)
        count = 5;
      if (bufMidi[2] == 0x2a)
        count = 6;
      if (bufMidi[2] == 0x2b)
        count = 7;

      if (bufMidi[3] > 0x00)
      {
        if (aState[count] == LOW)
        {
          aState[count] = HIGH;
        }
        else
        {
          aState[count] = LOW;
        }
      }

      aPrevious[count] = aReading[count];
    }

    //Knobs  9 - 16
    if ((bufMidi[6] == 0x70) || (bufMidi[2] == 0x4a) || (bufMidi[2] == 0x47) || (bufMidi[2] == 0x4c) || (bufMidi[2] == 0x4d) || (bufMidi[2] == 0x5d) || (bufMidi[2] == 0x49) || (bufMidi[2] == 0x4b))
    {
      Serial.println("KNOB 1");
      //bufMidi[4] 0 - 7C & bufMidi[8] 3F (-) 41 (+)

      int count = 0;

      if (bufMidi[6] == 0x70)
        count = 0;
      if (bufMidi[2] == 0x4a)
        count = 1;
      if (bufMidi[2] == 0x47)
        count = 2;
      if (bufMidi[2] == 0x4c)
        count = 3;
      if (bufMidi[2] == 0x4d)
        count = 4;
      if (bufMidi[2] == 0x5d)
        count = 5;
      if (bufMidi[2] == 0x49)
        count = 6;
      if (bufMidi[2] == 0x4b)
        count = 7;

      //OSC Type
      if (count == 0)
      {
        if (bufMidi[7] <= 0x3F)
        {
          if (aSet[count] > 0)
          {
            aSet[count] = aSet[count] - 1;
          }
        }

        if (bufMidi[7] >= 0x41)
        {
          if (aSet[count] < 4)
          {
            aSet[count] = aSet[count] + 1;
          }
        }
      }

      //Speed
      if (count == 1)
      {

      if ((bufMidi[3] >= knob1PrevValues[count]) && ((aSet[count] >= 10) || (aSet[count] <= 350)))
      {
          aSet[count] = aSet[count] - 10;
      }
      else
      {
          aSet[count] = aSet[count] + 10;
      }

      knob1PrevValues[count] = bufMidi[3];
      }

      //Lenght && Filter
      if (count == 2)
      {

        aSet[count] = (int)bufMidi[3];
        //aSet[count] = map(mySensVals[count], 0, 1023, 10, 350);
      }

      //Filter
      if (count == 3)
      {

        aSet[count] = (int)bufMidi[3];
        //aSet[count] = map(mySensVals[count], 0, 1023, 10, 350);
      }
    }

    //Knobs  9 - 16
    if ((bufMidi[6] == 0x72) || (bufMidi[2] == 0x12) || (bufMidi[2] == 0x13) || (bufMidi[2] == 0x10) || (bufMidi[2] == 0x11) || (bufMidi[2] == 0x5b) || (bufMidi[2] == 0x4f) || (bufMidi[2] == 0x48))
    {
      Serial.println("KNOB 2");
      //bufMidi[4] 0 - 7C & bufMidi[8] 3F (-) 41 (+)
      //setupVoice( voice[0-3] , waveform[SINE,TRIANGLE,SQUARE,SAW,RAMP,NOISE] , pitch[0-127], envelope[ENVELOPE0-ENVELOPE3], length[0-127], mod[0-127, 64=no mod])
      int count = 0;

      if (bufMidi[2] == 0x72)
        count = 0;
      if (bufMidi[2] == 0x12)
        count = 1;
      if (bufMidi[2] == 0x13)
        count = 2;
      if (bufMidi[2] == 0x10)
        count = 3;
      if (bufMidi[2] == 0x11)
        count = 4;
      if (bufMidi[2] == 0x5b)
        count = 5;
      if (bufMidi[2] == 0x4f)
        count = 6;
      if (bufMidi[2] == 0x48)
        count = 7;

      if (count == 0)
      {
        if (bufMidi[7] == 0x3F)
        {
          aPitch[count] = aPitch[count] - 10;
        }

        if (bufMidi[7] <= 0x41)
        {
          aPitch[count] = aPitch[count] + 10;
        }
      }
      else
      {
        aPitch[count] = (int)bufMidi[3];
      }
    }

    //Sliders
    if ((bufMidi[1] == 0xe0) || (bufMidi[1] == 0xb0))
    {
      Serial.println("SLIDERS");
      //bufMidi[4]  0 - 7F
    }
  }
}

uint16_t midiToFreq(uint8_t midiNote)
{
  Serial.println((uint16_t)440.0 * pow(2.0, ((float)midiNote - 69.0) / 12.0));
  if (midiNote < 0x28)
    return (uint16_t)440.0 * pow(2.0, ((float)midiNote - 9.0) / 24.0);
  return (uint16_t)440.0 * pow(2.0, ((float)midiNote - 69.0) / 12.0);
}

void playNote(uint8_t midiNote, uint8_t midiVelocity)
{
  //Serial.print("Play note ");
  //Serial.println(midiToFreq(midiNote));
  play(midiToFreq(midiNote));
}
void stopNote(uint8_t midiNote, uint8_t midiVelocity)
{
  //Serial.print("Stop note ");
  //Serial.println(midiNote);
}

void loop()
{
  Usb.Task();
  //uint32_t t1 = (uint32_t)micros();
  if (Usb.getUsbTaskState() == USB_STATE_RUNNING)
  {
    MIDI_poll();
  }

  switch (aSet[0])
  {
  case 0:
  {
    edgar.setupVoice(0, SINE, 0, ENVELOPE0, 0, 64, 500);
    break;
  }
  case 1:
  {
    edgar.setupVoice(0, TRIANGLE, 0, ENVELOPE0, 0, 64, 500);
    break;
  }
  case 2:
  {
    edgar.setupVoice(0, SQUARE, 0, ENVELOPE0, 0, 64, 500);
    break;
  }
  case 3:
  {
    edgar.setupVoice(0, SAW, 0, ENVELOPE0, 0, 64, 500);
    break;
  }
  case 4:
  {
    edgar.setupVoice(0, RAMP, 0, ENVELOPE0, 0, 64, 500);
    break;
  }
  case 5:
  {
    edgar.setupVoice(0, NOISE, 0, ENVELOPE0, 0, 64, 500);
    break;
  }
  }
  unsigned long currentMillis = millis();
  while (currentMillis - previousMillis >= aSet[1] && i < 8)
  {
    previousMillis = currentMillis;
    //edgar.setMod(0, aSet[3]);
    edgar.setLength(0, aSet[2]);
    edgar.setPitch(0, aPitch[i]);
    edgar.setFilter(0, aSet[3]);

    // OSC type
    lcd.setCursor(13, 0);
    lcd.print(aOsc[aSet[0]]); //Osc

    //Reset
    lcd.setCursor(3, 1);
    lcd.print("             ");

    //Speed
    lcd.setCursor(4, 1);
    lcd.print((int)(60 * (float)1000 / (float)aSet[1])); //Speed
    lcd.setCursor(9, 1);

    //Lenght
    lcd.print(aSet[2]); //Length
    lcd.setCursor(13, 1);

    //Modulation - Pitch
    lcd.print(aSet[3]); //Mod
    lcd.setCursor(0, 0);

    //Progress bar
    lcd.print("        ");

    lcd.setCursor(1, 0);
    for (int y = 0; y < 8; y++)
    {
      lcd.setCursor(y, 0);
      if (aState[y] == HIGH)
      {
        if (y == i)
        {
          //Progress position
          //lcd.setCursor(i+1, 0);
          lcd.write(0xFF);
        }
        else
        {
          lcd.print('_');
        }
      }
      else
      {
        if (y == i)
        {
          //Progress position
          //lcd.setCursor(i+1, 0);
          lcd.print('-');
        }
        else
        {
          lcd.print(' ');
        }
      }
    }

    //Position ID
    lcd.setCursor(9, 0);
    lcd.print(i + 1);

    if (aState[i] == 1)
    {
      edgar.trigger(0);
    }
    else
    {
      delay(aSet[1]);
    }
    i++;
  }
  if (i == 8)
  {
    i = 0;
  }
}

void play(int freq)
{
  edgar.setFrequency(0, freq);
  edgar.trigger(0);
}
