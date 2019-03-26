#include <Arduino.h>
 
#define KEYBOARD_DATA 32
#define KEYBOARD_CLK  33

unsigned int shift = 0;
byte lastcode  = 0;
bool keyup = false;
byte rc = 0;
byte keymap[256];

void kb_interruptHandler()
{
    shift>>=1;
    shift|=(digitalRead(KEYBOARD_DATA)<<10);
    if(++rc==11)
    {
          uint8_t data=(shift>>1)&0xff;
          lastcode = data;
          rc=0;
          shift=0;
          if(!keyup)
          {
            if(data == 0xf0)
              keyup = true;
            else
              keymap[data] = 0;
          }
          else
          {
            keymap[data] = 1;
            keyup = false;
          }
    }
}



void kb_begin()
{
    pinMode(KEYBOARD_DATA,OUTPUT_OPEN_DRAIN);
    pinMode(KEYBOARD_CLK,OUTPUT_OPEN_DRAIN);
    digitalWrite(KEYBOARD_DATA,true);
    digitalWrite(KEYBOARD_CLK,true);
    attachInterrupt(digitalPinToInterrupt(KEYBOARD_CLK), kb_interruptHandler, FALLING);
    for(int gg = 0;gg < 256;gg++)
      keymap[gg] = 1;
    
}
