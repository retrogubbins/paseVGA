
//#include "Z80.h"
//extern typedef Z80_Regs;

extern byte *bank1;

typedef int32_t      dword;
typedef signed char    offset;

typedef union
{
#ifdef __128BIT__
 #ifdef LSB_FIRST
   struct { byte l,h,h2,h3,h4,h5,h6,h7,
                 h8,h9,h10,h11,h12,h13,h14,h15; } B;
   struct { int16_t l,h,h2,h3,h4,h5,h6,h7; } W;
   dword D;
 #else
   struct { byte h15,h14,h13,h12,h11,h10,h9,h8,
                 h7,h6,h5,h4,h3,h2,h,l; } B;
   struct { int16_t h7,h6,h5,h4,h3,h2,h,l; } W;
   dword D;
 #endif
#elif __64BIT__
 #ifdef LSB_FIRST
   struct { byte l,h,h2,h3,h4,h5,h6,h7; } B;
   struct { int16_t l,h,h2,h3; } W;
   dword D;
 #else
   struct { byte h7,h6,h5,h4,h3,h2,h,l; } B;
   struct { int16_t h3,h2,h,l; } W;
   dword D;
 #endif
#else
 #ifdef LSB_FIRST
   struct { byte l,h,h2,h3; } B;
   struct { int16_t l,h; } W;
   dword D;
 #else
   struct { byte h3,h2,h,l; } B;
   struct { int16_t h,l; } W;
   dword D;
 #endif
#endif
} pair;

typedef struct
{
  pair AF,BC,DE,HL,IX,IY,PC,SP;
  pair AF2,BC2,DE2,HL2;
  int16_t IFF1,IFF2,HALT,IM,I,R,R2;
} Z80_Regs;

extern void Z80_SetRegs (Z80_Regs *Regs);

void load_speccy()
{
  File  lhandle,lhandle2;
  unsigned int  buf_p=0,quot=0,size_read,load_address;
  unsigned char csum,flag_byte,header_byte1;
  unsigned int cdd,f,ret,blocksize,data_leng,param1;
  unsigned int tap_leng,exec_address, buf_pale;
  char csum_ok[10];
int tape_type = 0;
//  pump_string("mload\"\"\x0d");
int file_id; 
byte *lbuffer;
Z80_Regs i;

  Serial.print("Free Heap: ");
  Serial.println(system_get_free_heap_size());
 
  // open a file for input      
 // lhandle = SD.open("/AirRaid.tap", FILE_READ);
 // lhandle = SD.open("/FloydsBank .tap", FILE_READ);
//  lhandle = SD.open("/OhMummy.tap", FILE_READ);
  lhandle = SD.open("/manic.sna", FILE_READ);
//  lhandle = SD.open("/jetpac.sna", FILE_READ);
//  lhandle = SD.open("/atic.sna", FILE_READ);
 // lhandle = SD.open("/sabre.sna", FILE_READ);
 // lhandle = SD.open("/underw.sna", FILE_READ);
// lhandle = SD.open("/alien8.sna", FILE_READ);
//  lhandle = SD.open("/emerald.sna", FILE_READ);

  size_read=0;
  if(lhandle!=NULL)
  {
    Serial.println("Loading:");
    //Read in the registers
    i.I=lhandle.read();
    i.HL2.B.l=lhandle.read();
    i.HL2.B.h=lhandle.read();
    i.BC2.B.l=lhandle.read();
    i.BC2.B.h=lhandle.read();
    i.DE2.B.l=lhandle.read();
    i.DE2.B.h=lhandle.read();
    i.AF2.B.l=lhandle.read();
    i.AF2.B.h=lhandle.read();

    i.HL.B.l=lhandle.read();
    i.HL.B.h=lhandle.read();
    i.BC.B.l=lhandle.read();
    i.BC.B.h=lhandle.read();
    i.DE.B.l=lhandle.read();
    i.DE.B.h=lhandle.read();
    i.IY.B.l=lhandle.read();
    i.IY.B.h=lhandle.read();
    i.IX.B.l=lhandle.read();
    i.IX.B.h=lhandle.read();

    byte inter =lhandle.read();
  Serial.println("inter address: ");
    Serial.println((unsigned int)inter, HEX);
    if(inter & 0x04 == 0)
        i.IFF2 = 0;
    else
        i.IFF2 = 1;

    i.R =lhandle.read();
 Serial.println("R : ");
    Serial.println((unsigned int)i.R, HEX);
 
    i.AF.B.l=lhandle.read();
    i.AF.B.h=lhandle.read();
 Serial.println("AF : ");
    Serial.println((unsigned int)i.AF.B.l, HEX);
    Serial.println((unsigned int)i.AF.B.h, HEX);
        
    i.SP.B.l=lhandle.read();
    i.SP.B.h=lhandle.read();
 // Serial.println("SP address");
 //   Serial.println((unsigned int)i.SP.W);
  Serial.println("SP address: ");
    Serial.println((unsigned int)i.SP.B.l, HEX);
    Serial.println((unsigned int)i.SP.B.h, HEX);

    i.IM = lhandle.read();
 Serial.println("IM : ");
    Serial.println((unsigned int)i.IM, HEX);
 
    byte bordercol =lhandle.read();
 Serial.println("Border : ");
    Serial.println((unsigned int)bordercol, HEX);
 

    i.IFF1 = i.IFF2;

   unsigned int thestack =  i.SP.B.h * 256 + i.SP.B.l;
    unsigned int buf_p = 0;
    while (lhandle.available())
    {
      bank1[buf_p] = lhandle.read();
      buf_p++;
    }
    lhandle.close();
    Serial.println("noof bytes");
    Serial.println(buf_p);


    Serial.println("STACK:");
    for(int yy = 0;yy < 16;yy++)
          Serial.println(bank1[thestack - 0x4000 + yy], HEX);



    //   unsigned int retaddr = bank1[(i.SP.D >>16) - 0x4000];
    unsigned int retaddr = bank1[thestack - 0x4000] + bank1[thestack+1 - 0x4000] * 256 ;
  
     Serial.println("sp before");
     Serial.println((unsigned int) i.SP.D, HEX);
    i.SP.D++;
    i.SP.D++;
     Serial.println("sp after");
     Serial.println((unsigned int) i.SP.D, HEX);
    i.PC.D = 0x8400; //retaddr;  //0x7c19;  // 0x8400;  //manic miner is 0x8400   // 0x6000; atic atac doesnt run properly
    Serial.println("retn address");
    Serial.println(retaddr, HEX);

    Z80_SetRegs (&i);
  }
  else
  {
    Serial.println("Couldn't Open SNA file ");
    return;
  }
}

/* 
  blocksize=0;
//  while(buf_p<size_read)
  {
    //Get block size
    blocksize=(lbuffer[buf_p]+(lbuffer[buf_p+1]*0x100));
   Serial.print("Blocksize: ");
    Serial.println(blocksize);

    //sprintf(lbl,"Specbuf n=%2x   N+1=%2x,  BLocksize %2x",specbuf[cdd],specbuf[cdd+1],blocksize);
    //ret=MessageBox(NULL,lbl,"PALE Load Speccy",MB_ICONERROR | MB_OK);
    if(blocksize<65000 && blocksize!=0)
    {
//        blockpos=0;
        buf_p+=2;
    
        //Header
        flag_byte = lbuffer[buf_p++];
    Serial.print("Flag byte: ");
    Serial.println(flag_byte);
        header_byte1 = lbuffer[buf_p++];
    Serial.print("Header 1st byte: ");
    Serial.println(header_byte1);

    for(int jj = 0;jj < 10;jj++)
    {
      Serial.write(lbuffer[buf_p + jj]);
    }
    Serial.println(" ");
    buf_p+=10;

        data_leng = lbuffer[buf_p]+lbuffer[buf_p+1]*0x100;
    Serial.print("Data leng: ");
    Serial.println(data_leng);
    buf_p+=2;
        param1 =  lbuffer[buf_p]+lbuffer[buf_p+1]*0x100;
    Serial.print("Param 1: ");
    Serial.println(param1);
    buf_p+=2;
        param2 =  lbuffer[buf_p]+lbuffer[buf_p+1]*0x100;
    Serial.print("Param 2: ");
    Serial.println(param2);

*/

      
    //    while(blockpos<blocksize)
    //    {
    //      //Convert each byte into a pulse train
    //      convert_specbyte(specbuf[cdd]);
    //      cdd++;
    //      blockpos++;
    //    }
//    }
//
//  }
//   free(lbuffer);  
//}
