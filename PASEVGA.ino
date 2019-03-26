
// ____________________________________________________
// Pase Spectrum Emulator (KOGEL esp32)
// Pete Todd 2017
//     You are not allowed to distribute this software commercially     
//     Please, notify me, if you make any changes to this file        
// ____________________________________________________
#include <ESP32Lib.h>  // Bitluni VGA driver library

#include <bt.h>
//#include <esp_wifi.h>
#include "PS2Kbd.h"
//#include "SPI.h"
#include <SPI.h>
//#include <WiFi.h>
//#include <WiFiMulti.h>
#include "FS.h"
#include "SD.h"
#include "paledefs.h"

// ________________________________________________
// SWITCHES
//
#define SD_ENABLED
//#define DEBUG
bool run_debug = false;

// ____________________________________________________________________
// PINOUT DEFINITIONS
//

#define KEYBOARD_DATA 32
#define KEYBOARD_CLK  33

const int redPin = 27;
const int greenPin = 21;
const int bluePin = 14;
const int hsyncPin = 0;
const int vsyncPin = 22;

#define DEBUG_PIN 2
#define DEBUG_PIN2 16



// ____________________________________________________________________
// INSTANTS
//

//VGA Device
VGA3Bit vga;

// ________________________________________________________________________
// EXTERNS + GLOBALS
//
unsigned Z80_GetPC (void);         /* Get program counter                   */
void Z80_Reset (void);             /* Reset registers to the initial values */
unsigned int  Z80_Execute ();           /* Execute IPeriod T-States              */

void pump_key(char k);

byte Z80_RDMEM(uint16_t A);
void Z80_WRMEM(uint16_t A,byte V);
extern byte bank_latch;
extern int start_im1_irq;

File myFile;

// ________________________________________________________________________
// GLOBALS
//
byte *bank1;
byte *bank2;
byte *bank3;
uint16_t *tftmem;
byte z80ports_in[32];


// SETUP *************************************
// SETUP *************************************
// SETUP *************************************
SemaphoreHandle_t xMutex;

void setup()
{
  // Turn off peripherals to gain memory (?do they release properly)
  esp_bt_controller_deinit();
  esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
  //  esp_wifi_set_mode(WIFI_MODE_NULL);
  
  pinMode(SOUND_PIN,OUTPUT);
  digitalWrite(SOUND_PIN,LOW);

  pinMode(DEBUG_PIN,OUTPUT);
  digitalWrite(DEBUG_PIN,LOW);
  pinMode(DEBUG_PIN2,OUTPUT);
  digitalWrite(DEBUG_PIN2,LOW);
  
  //we need double buffering for smooth animations
  vga.setFrameBufferCount(1);
  //initializing i2s vga (with only one framebuffer)
  vga.init(vga.MODE320x240.custom(256, 192), redPin, greenPin, bluePin, hsyncPin, vsyncPin);
  
  kb_begin();
  
  Serial.begin(115200);

  #ifdef SD_ENABLED
      test_sd();
  #endif

  // ALLOCATE MEMORY
  //
  bank1 = (byte *)malloc(49152);
  if(bank1 == NULL)Serial.println("Failed to allocate Bank 1 memory");

  // START Z80
  // START Z80
  // START Z80
  Serial.println("RESETTING Z80");
  Z80_Reset(); 

  // make sure keyboard ports are FF
  for(int t = 0;t < 32;t++)
  {
    z80ports_in[t] = 0xff;
  }

  Serial.print("Setup: MAIN Executing on core ");
  Serial.println(xPortGetCoreID());
  Serial.print("Free Heap: ");
  Serial.println(system_get_free_heap_size());

 
xMutex = xSemaphoreCreateMutex();
  
  xTaskCreatePinnedToCore(
                      videoTask,   /* Function to implement the task */
                      "videoTask", /* Name of the task */
                      2048,      /* Stack size in words */
                      NULL,       /* Task input parameter */
                      0,          /* Priority of the task */
                      NULL,       /* Task handle. */
                      0);  /* Core where the task should run */
  load_speccy();



  
}


// VIDEO core 0 *************************************
// VIDEO core 0 *************************************
// VIDEO core 0 *************************************

void videoTask( void * parameter )
{
   unsigned int ff,i,byte_offset;
   unsigned char color_attrib,pixel_map,zx_fore_color,zx_back_color;
   unsigned int zx_vidcalc;
   while(1)
   {
        xSemaphoreTake( xMutex, portMAX_DELAY );
  digitalWrite(DEBUG_PIN,HIGH);
        for(unsigned int lin = 0;lin < 192;lin++)
        {
          for(ff=0;ff<32;ff++)  //foreach byte in line
          {
            byte_offset=lin*32+ff;//*2+1;
            //spectrum attributes
            //bit 7 6   5 4 3   2 1 0
            //    F B   P A P   I N K
            color_attrib=bank1[0x1800+(calcY(byte_offset)/8)*32+ff];//get 1 of 768 attrib values
            pixel_map=bank1[byte_offset];
            zx_fore_color=color_attrib & 0x07;
            zx_back_color=(color_attrib & 0x38)>>3;
    
            for(i=0;i<8;i++)  //foreach pixel within a byte
            {
                zx_vidcalc=ff*8+i;
                byte bitpos = (0x80 >> i);
                if((pixel_map & bitpos)!=0)
                               vga.dotFast(zx_vidcalc, calcY(byte_offset),zxcolor(zx_fore_color)) ;          
                else
                               vga.dotFast(zx_vidcalc, calcY(byte_offset),zxcolor(zx_back_color)) ;          
    
            }
          }
        } 
 digitalWrite(DEBUG_PIN,LOW);
        xSemaphoreGive( xMutex ); 
        vTaskDelay(1) ;
     }
}
      
// SPECTRUM SCREEN DISPLAY
// SPECTRUM SCREEN DISPLAY
// SPECTRUM SCREEN DISPLAY
//
/* Calculate Y coordinate (0-192) from Spectrum screen memory location */
int calcY(int offset){
  return ((offset>>11)<<6)        // sector start
       +((offset%2048)>>8)        // pixel rows
       +((((offset%2048)>>5)-((offset%2048)>>8<<3))<<3);  // character rows
}

/* Calculate X coordinate (0-255) from Spectrum screen memory location */
int calcX(int offset){
  return (offset%32)<<3;
}


unsigned int zxcolor(byte c)
{
  if(c == 0)
    return vga.RGB(0,0,0);
  else if (c == 1)
    return vga.RGB(0,0,255);
  else if (c == 2)
    return vga.RGB(255,0,0);
  else if (c == 3)
    return vga.RGB(255,0,255);
  else if (c == 4)
    return vga.RGB(0,255,0);
  else if (c == 5)
    return vga.RGB(0,255,255);
  else if (c == 6)
    return vga.RGB(255,255,0);
  else if (c == 7)
    return vga.RGB(255,255,255);
  return 0;  
}



// LOOP core 1 *************************************
// LOOP core 1 *************************************
// LOOP core 1 *************************************

void loop() 
{
  while (1) 
  { 
        do_keyboard();
        xSemaphoreTake( xMutex, portMAX_DELAY );
  digitalWrite(DEBUG_PIN2,HIGH);
             Z80_Execute();
   digitalWrite(DEBUG_PIN2,LOW);
        xSemaphoreGive( xMutex );
        start_im1_irq=1;    // keyboard scan is run in IM1 interrupt
        vTaskDelay(1) ;  //important to avoid task watchdog timeouts - change this to slow down emu
  } 
}




void do_keyboard()
{
    bitWrite(z80ports_in[0], 0, keymap[0x12]);
    bitWrite(z80ports_in[0], 1, keymap[0x1a]);
    bitWrite(z80ports_in[0], 2, keymap[0x22]);
    bitWrite(z80ports_in[0], 3, keymap[0x21]);
    bitWrite(z80ports_in[0], 4, keymap[0x2a]);

    bitWrite(z80ports_in[1], 0, keymap[0x1c]);
    bitWrite(z80ports_in[1], 1, keymap[0x1b]);
    bitWrite(z80ports_in[1], 2, keymap[0x23]);
    bitWrite(z80ports_in[1], 3, keymap[0x2b]);
    bitWrite(z80ports_in[1], 4, keymap[0x34]);

    bitWrite(z80ports_in[2], 0, keymap[0x15]);
    bitWrite(z80ports_in[2], 1, keymap[0x1d]);
    bitWrite(z80ports_in[2], 2, keymap[0x24]);
    bitWrite(z80ports_in[2], 3, keymap[0x2d]);
    bitWrite(z80ports_in[2], 4, keymap[0x2c]);

    bitWrite(z80ports_in[3], 0, keymap[0x16]);
    bitWrite(z80ports_in[3], 1, keymap[0x1e]);
    bitWrite(z80ports_in[3], 2, keymap[0x26]);
    bitWrite(z80ports_in[3], 3, keymap[0x25]);
    bitWrite(z80ports_in[3], 4, keymap[0x2e]);
    
    bitWrite(z80ports_in[4], 0, keymap[0x45]);
    bitWrite(z80ports_in[4], 1, keymap[0x46]);
    bitWrite(z80ports_in[4], 2, keymap[0x3e]);
    bitWrite(z80ports_in[4], 3, keymap[0x3d]);
    bitWrite(z80ports_in[4], 4, keymap[0x36]);
    
    bitWrite(z80ports_in[5], 0, keymap[0x4d]);
    bitWrite(z80ports_in[5], 1, keymap[0x44]);
    bitWrite(z80ports_in[5], 2, keymap[0x43]);
    bitWrite(z80ports_in[5], 3, keymap[0x3c]);
    bitWrite(z80ports_in[5], 4, keymap[0x35]);
    
    bitWrite(z80ports_in[6], 0, keymap[0x5a]);
    bitWrite(z80ports_in[6], 1, keymap[0x4b]);
    bitWrite(z80ports_in[6], 2, keymap[0x42]);
    bitWrite(z80ports_in[6], 3, keymap[0x3b]);
    bitWrite(z80ports_in[6], 4, keymap[0x33]);
    
    bitWrite(z80ports_in[7], 0, keymap[0x29]);
    bitWrite(z80ports_in[7], 1, keymap[0x14]);
    bitWrite(z80ports_in[7], 2, keymap[0x3a]);
    bitWrite(z80ports_in[7], 3, keymap[0x31]);
    bitWrite(z80ports_in[7], 4, keymap[0x32]);
}


#ifdef SD_ENABLED
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void test_sd()
{
      if(!SD.begin(16, SPI, 1000000)){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
 
 
  // re-open the file for reading:
  myFile = SD.open("/AirRaid.tap", FILE_READ);
  if (myFile) {
    Serial.println("COMMAND.TXT:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening COMMAND.TXT");
  }

listDir(SD, "/", 0);

}
#endif

/*         
//taskENTER_CRITICAL(&screenMux);
//taskEXIT_CRITICAL(&screenMux);
  //    xSemaphoreTake( xMutex, portMAX_DELAY );       
   //   xSemaphoreGive( xMutex );
//        taskENTER_CRITICAL(&screenMux);
//        taskEXIT_CRITICAL(&screenMux);


//  timer = timerBegin(0, 80, true);
//  timerAttachInterrupt(timer, &onTimer, true);
//  timerAlarmWrite(timer, 1000000, true);
//  timerAlarmEnable(timer);

    int fpsval;
    if (interruppted > 0)
    {
     
        portENTER_CRITICAL(&timerMux);
        fpsval = interruptfps; 
        interruptfps = 0;
        interruppted = 0;
        portEXIT_CRITICAL(&timerMux);
     
           // totalInterruptCounter++;
         
        //    Serial.print("FPS: ");
        //    Serial.println(fpsval);
        //    Serial.print("HEAPSIZE: ");
        //    Serial.println(system_get_free_heap_size());
    }   


//xSemaphoreTake( xMutex, portMAX_DELAY );
//  taskENTER_CRITICAL_ISR(&screenMux);
      
      // portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
      // taskENTER_CRITICAL(&myMutex);
    //   vTaskDelay(1) ;
//interruptfps++;     
//   taskEXIT_CRITICAL_ISR(&screenMux);
//xSemaphoreGive( xMutex );   
      // taskEXIT_CRITICAL(&myMutex);



SemaphoreHandle_t xMutex;


int interruptfps = 0;
int interruppted = 0;

//hw_timer_t * timer = NULL;
//portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

portMUX_TYPE screenMux = portMUX_INITIALIZER_UNLOCKED;

//void IRAM_ATTR onTimer() 
//{
//  portENTER_CRITICAL_ISR(&timerMux);
//  interruppted++;
//  portEXIT_CRITICAL_ISR(&timerMux);
// 
//}

    
 */
