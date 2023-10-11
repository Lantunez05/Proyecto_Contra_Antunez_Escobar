//*********************************************
/* Librería para el uso de la pantalla ILI9341 en modo 8 bits
 * Basado en el código de martinayotte - https://www.stm32duino.com/viewtopic.php?t=637
 * Adaptación, migración y creación de nuevas funciones: Pablo Mazariegos y José Morales
 * Con ayuda de: José Guerra
 * IE3027: Electrónica Digital 2 - 2019
 */


 
//*********************************************
// Librerias
//*********************************************
#include <SPI.h>
#include <SD.h>
#include <stdint.h>
#include <stdbool.h>
#include <TM4C123GH6PM.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"


#include "bitmaps.h"
#include "font.h"
#include "lcd_registers.h"




#define LCD_RST PD_0
#define LCD_CS PD_1
#define LCD_RS PD_2
#define LCD_WR PD_3
#define LCD_RD PE_1
int DPINS[] = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7};  




//*********************************************
// Functions Prototypes
//*********************************************

//Funciones pantalla
void LCD_Init(void);    //inicializacion
void LCD_CMD(uint8_t cmd);  //envio de comando
void LCD_DATA(uint8_t data);  //envio de dato
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);  //setear la ventana
void LCD_Clear(unsigned int c); //limpiar la pantalla
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c); //Crea una linea horizontal
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);  //Crea una linea vertical
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);  //crea el contorno de un rectangulo
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c); //crea un rectangulo
void LCD_Print(String text, int x, int y, int fontSize, int color, int background); //genera un texto
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]); //Dibujar una imagen
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],int columns, int index, char flip, char offset); //dibuja un sprite
bool Impacto(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

//funciones SD
int ascii2hex(int a);
void mapeo_SD(char doc[], int x, int y,int w, int h);

//funciones juego
void menu();
void rigth(int X, int Y, int player);
void left(int X, int Y, int player);
void jump(int X, int Y,int player);
void stand(int X, int Y,int player);
bool canserbero(int X, int Y,int player);
void star();
void musica(bool mute);
void disparo();
void disparoJ2();

//****************************************
// variables
//****************************************

extern uint8_t plataforma1[];
extern uint8_t plataforma2[];

const int PUSHB1 = PF_4; 
const int PUSHB2 = PF_0;
const int PUSHB3 = PD_7;
const int PUSHB4 = PD_6; 
const int PUSHB5 = PC_7;
const int PUSHB6 = PC_6;
const int PUSHB7 = PC_5; // Disparo jugador 1
const int PUSHB8 = PC_4; // Disparo jugador 2
File myFile;

/*estado in-game*/
bool Fall_action = false;
bool coin = false;
bool mute = false;
bool image = false;
bool selection = false;
bool balaEnMovimiento = false;
bool balaEnMovimiento2 = false;
bool impact = false; //Variables impactos

/*estado boton*/
int b1 = 0;
int b2 = 0;
int b3 = 0;
int b4 = 0;
int b5 = 0;
int b6 = 0;
int b7 = 0;  // Disparo J1
int b8 = 0;  // Disparo J2

int turnos1=0;
int turnos2=4;

/*posicion personaje*/
int POSX = 10;
int POSY = 183;
int POSX2 = 150;
int POSY2 = 29;
int bala = 172;
int bala2 = 42;
int temporalX=0;
int temporalY=0;
int dir_disp = 0;
int dir_dispJ2 = 0;

//*********************************************
// Inicialización
//*********************************************
void setup() {
  SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
  Serial.begin(9600);
  GPIOPadConfigSet(GPIO_PORTB_BASE, 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);
  Serial.println("Inicio");
  LCD_Init();
  LCD_Clear(0x00);

  Serial.begin(9600);
  while(!Serial);
  SPI.setModule(0);

  Serial.print("Initializing SD card...");

  pinMode(PA_3, OUTPUT);

   if (!SD.begin(PA_3)) 
   {
      Serial.println("initialization failed!");
      return;
    }
    Serial.println("initialization done.");

    
   //INPUTS
    pinMode(PUSHB1, INPUT_PULLUP); 
    pinMode(PUSHB2, INPUT_PULLUP);
    pinMode(PUSHB3, INPUT_PULLUP);
    pinMode(PUSHB4, INPUT_PULLUP); 
    pinMode(PUSHB5, INPUT_PULLUP);
    pinMode(PUSHB6, INPUT_PULLUP);
    pinMode(PUSHB7, INPUT_PULLUP); // Disparo J1
    pinMode(PUSHB8, INPUT_PULLUP); // Disparo J2
    

  menu();
}


//*********************************************
// Main
//*********************************************
void loop() 
{
  /*lectura de botones*/
  b1 = digitalRead(PUSHB1);
  b2 = digitalRead(PUSHB2);
  b3 = digitalRead(PUSHB3);
  b4 = digitalRead(PUSHB4);
  b5 = digitalRead(PUSHB5);
  b6 = digitalRead(PUSHB6);
  b7 = digitalRead(PUSHB7);
  b8 = digitalRead(PUSHB8);

  
  
  if(coin == true)
  { 
    /*player1*/ 
    if(turnos1 < 4)
    {
        Fall_action = canserbero(POSX,POSY,1);
        if(Fall_action == true)
        {
         POSX = temporalX;
         POSY = temporalY;
        }
            
        //* correr derecha
        if(b1 == LOW)
        {  turnos1++; 
         rigth(POSX,POSY,1);     
         POSX = temporalX;
         POSY = temporalY;
         dir_disp = 0;
        }
      
        //* correr izquierda
        if(b2 == LOW)
        { turnos1++;  
          left(POSX,POSY,1);
          POSX = temporalX;
          POSY = temporalY;
          dir_disp = 1;
        }
    
        //*salto
        if(b3 == LOW)
        { turnos1++;
          jump(POSX,POSY,1);
          POSX = temporalX;
          POSY = temporalY;
        }
    
        //*Disparo J1
        if(b7 == LOW)
        {turnos1++;
          disparo();
        }

    }
    else if(turnos2 == 4)
    {
      turnos1 = 0;
    }
    
    /*player 2*/
    if(turnos2<4)
    {    
          Fall_action = canserbero(POSX2,POSY2,1);
          if(Fall_action == true)
          {
           POSX2 = temporalX;
           POSY2 = temporalY;
          }
              
          //* correr derecha
          if(b4 == LOW)
          {  turnos2++; 
           rigth(POSX2,POSY2,1);     
           POSX2 = temporalX;
           POSY2 = temporalY;
           dir_dispJ2 = 0;
          }
        
          //* correr izquierda
          if(b5 == LOW)
          {  turnos2++;  
            left(POSX2,POSY2,1);
            POSX2 = temporalX;
            POSY2 = temporalY;
            dir_dispJ2 = 1;
          }
      
          //*salto
          if(b6 == LOW)
          {turnos2++; 
            jump(POSX2,POSY2,1);
            POSX2 = temporalX;
            POSY2 = temporalY;
          }
      
           //*Disparo J2
          if(b8 == LOW)
          {turnos2++; 
            disparoJ2();
          }
      }
      else if(turnos1 == 4)
      {
        turnos2 = 0;
      }
  }
}  
   
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Funciones Juego  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//****************************************
// Menu
//****************************************
void menu(){
  
  LCD_Clear(0x00);//limpia la pantalla
  
  if(image == false)
  mapeo_SD("menu1.txt",0 ,0,320,240);
  else if(image == true)
  mapeo_SD("menu2.txt",0 ,0,320,240);
  
  
  delay(500);
  

  b1 = digitalRead(PUSHB1); //lee el estado del boton
  b2 = digitalRead(PUSHB2);
  bool confirmation = false;
  
  while(b1 == HIGH)//mientras no se presione el boton de confirmacion
   { 
       b1 = digitalRead(PUSHB1); 
       b2 = digitalRead(PUSHB2);

        if(b1 == LOW)
       {
         confirmation = true; 
       }
       
       if(b2 == LOW)
       {
        selection = !selection;
        image = !image;
        menu();
       }
       
   }
   
   
   if(selection == false && confirmation == true)
   {
     star();
   }
   else if(selection == true && confirmation == true)
   {
      mute = !mute;
      musica(mute);
      menu();
   }
   
   return;
}

//****************************************
// musica
//****************************************
void musica(bool mute){
  if(mute==true)
  Serial.println("no baila");
  else
  Serial.println("baila baila");
  return;
}

//****************************************
// carga el juego
//****************************************
void star()
{  
   coin = true; //bandera de juego → TRUE
   
   LCD_Clear(0x00);
   String text_begin = "Ok Let's GO!";
   for(int k=0; k<3; k++) //despliega el texto de forma intermitente 3 veces 
   {
      
      LCD_Print(text_begin,50,120 , 2, 0xffff, 0x0);
      delay(500);
      LCD_Clear(0x00);
   }

   delay(300);
   mapeo_SD("fondo.txt",0 ,0,320,240); //carga la imagen del menu


   delay(50);
   LCD_Bitmap(POSX, POSY, 24,33,parado); //genera el personaje 1

   delay(50);
   LCD_Bitmap(POSX2, POSY2, 24,33,parado); //genera el personaje 2

   return;
}
//****************************************
// Parado
//****************************************
void stand(int X, int Y, int player){   
 
 
  LCD_Bitmap(X, Y, 24,33,parado); 
  
  temporalX = X;
  temporalY = Y;
  return;
}

//****************************************
// Correr a la derecha
//****************************************
void rigth(int X, int Y, int player){
  int anim1;
  while(b1 == LOW || b4 == LOW)
  {
    b1 = digitalRead(PUSHB1);
    b4 = digitalRead(PUSHB4);
    if(X<290)
    X++;
    
    anim1 = (X/10)%4;


    LCD_Sprite(X,Y,24,33,correrR,4,anim1,0,1);
    
    
    delay(10);
  }
  temporalX = X;
  temporalY = Y;
  return;
}


//****************************************
// Correr a la izquierda
//****************************************
void left(int X, int Y, int player){
  int anim2;
  while(b2 == LOW || b5 == LOW)
  {
    b2 = digitalRead(PUSHB2);
    b5 = digitalRead(PUSHB5);
    if(X>0)
    X--;
    
    anim2 = (X/10)%4;


    LCD_Sprite(X,Y,24,33,correrL,4,anim2,0,1); 
    
    delay(10);
  }
  temporalX = X;
  temporalY = Y;
  return;
}



//****************************************
// saltar
//***************************************
void jump(int X, int Y, int player){
  int temporal = Y; //tierra
  Y = Y + 8; //
  
  
  for(int up=13; up>=0; up--)//subida
  { delay(50);
    Y = Y - up;
    int anim1 = (Y/2)%4;

    if(player == 1)
    LCD_Sprite(X,Y,22,25,salto,4,anim1,0,0);


    
    FillRect(X, Y+26, 22, 12, 0XBBD);
  }
  
  for(int down=0; down<=12; down++)
  { //caida
      delay(50);
      Y = Y + down;
      int anim2 = (Y/2)%4;

      if(player == 1)
      LCD_Sprite(X,Y,22,25,salto,4,anim2,0,0); 

      
      FillRect(X, Y-6, 22, 12, 0XBBD);
      
      if(X<129 && X>14 )//plataforma 1   
      { if(Y>134-33 && Y<134)
        {
          Y = 134-33;
          LCD_Bitmap(34, 135, 96, 14, plataforma1); //recarga la plataforma
          stand(X,Y,player);
          temporalX = X;
          temporalY = Y;
          return;
        }
      }
      if(X<253 && X>103) //plataforma 2
      { if(Y>62-33 && Y<62)
        {
          Y = 62-33;
          LCD_Bitmap(126, 63, 128, 14, plataforma2); //recarga la plataforma
          stand(X,Y,player);
          temporalX = X;
          temporalY = Y;          
          return;
        }
      }
  }
  Y = temporal;
  stand(X,Y,player);
  temporalX = X;
  temporalY = Y;
  return;  
}

//****************************************
//Caida
//***************************************
bool canserbero(int X, int Y, int player)
{
  if(Y == 101) //si se encuentra en la plataforma 1
  {
    if(X>129 || X<14) //si se encuentra en los bordes
    { 
      for(int down=0; down<=12; down++) 
      {
        delay(50);
        Y = Y + down;
        int anim2 = (Y/2)%4;

        if(player == 1)
        LCD_Sprite(X,Y,22,25,salto,4,anim2,0,0);
        
        FillRect(X, Y-6, 22, 12, 0XBBD);
      }
      Y=183;
      stand(X,Y,player);
      return true;
      
    }    
  } 
  if(Y == 29) //se se encuentra en la plataforma 2
  {
    if(X<104)
    {
       for(int down=0; down<=12; down++) 
      {
        delay(50);
        Y = Y + down;
        int anim2 = (Y/2)%4;

        if(player == 1) 
        LCD_Sprite(X,Y,22,25,salto,4,anim2,0,0);

        
        FillRect(X, Y-6, 22, 12, 0XBBD);
      }
      Y=101;
      stand(X,Y,player);
      return true;
    }
    else if(X>253)
    {
      for(int down=0; down<18; down++) 
      {
        delay(50);
        Y = Y + down;
        int anim2 = (Y/2)%4;

        if(player==1)
        LCD_Sprite(X,Y,22,25,salto,4,anim2,0,0);
                
        FillRect(X, Y-15, 22, 20, 0XBBD);
      }
      Y=183;
      stand(X,Y,player);
      return true;
    }
  }
  
  return false;
}

//****************************************
// Disparo                                  
//***************************************
void disparo() {
  if (dir_disp == 0){
    if (!balaEnMovimiento) {
      bala = 321;
      bala = POSX+22; // Restablece la posición de la bala a la posición inicial del jugador
      balaEnMovimiento = true; // Establece la bandera de movimiento de la bala a verdadero
    }
    
    if (bala < 321 && bala > -11 ) 
    {      
      while(bala <= 321 && impact == false )
      {   // Dibuja la bala en la posición actual
          LCD_Bitmap(bala, POSY, 8, 8, disparoJ1);
          V_line(bala - 1, POSY, 8, 0XBBD);
          bala++; // Mueve la bala hacia la derecha
          impact = Impacto(bala, POSY, 8, 8, POSX2,POSY2, 24, 33);
          if (impact == true)
          {
            String text_begin = "Gana J1";
            for(int k=0; k<3; k++) //despliega el texto de forma intermitente 3 veces 
            {   
              LCD_Print(text_begin,50,120 , 2, 0xffff, 0x0);
              delay(500);
              LCD_Clear(0x00);
            }
          }
          delay(5);     
       }
    } else {
      // La bala ha salido de la pantalla
      balaEnMovimiento = false;
      bala = POSX+22;
      delay(500);
    }
  }

  if (dir_disp == 1){
    if (!balaEnMovimiento) {
      bala = -11;
      bala = POSX-2; // Restablece la posición de la bala a la posición inicial del jugador
      balaEnMovimiento = true; // Establece la bandera de movimiento de la bala a verdadero
    }
  
    if (bala < 321 && bala > -11  ) {
      // Dibuja la bala en la posición actual
      while(bala >= -11 && impact == false )
      {
          LCD_Bitmap(bala, POSY, 8, 8, disparoJ1);
          V_line(bala + 8, POSY, 8, 0XBBD);
          bala--; // Mueve la bala hacia la derecha
          impact = Impacto(bala, POSY, 8, 8, POSX2,POSY2, 24, 33);
          if (impact == true)
          {
            String text_begin = "Gana J1";
            for(int k=0; k<3; k++) //despliega el texto de forma intermitente 3 veces 
            {
              LCD_Print(text_begin,50,120 , 2, 0xffff, 0x0);
              delay(500);
              LCD_Clear(0x00);
            }    
          }
          delay(5);
      }
      
    } else {
      // La bala ha salido de la pantalla
      balaEnMovimiento = false;
      bala = POSX-2;
      delay(500);
    }
  }
}

void disparoJ2() {
  if (dir_dispJ2 == 0){
    if (!balaEnMovimiento2) {
      bala2 = 321;
      bala2 = POSX2+22; // Restablece la posición de la bala a la posición inicial del jugador
      balaEnMovimiento2 = true; // Establece la bandera de movimiento de la bala a verdadero
    }
    
    if (bala2 < 321 && bala2 > -11  ) {
      while(bala2 <= 321 && impact == false )
      {
          // Dibuja la bala en la posición actual
          LCD_Bitmap(bala2, POSY2, 8, 8, disparoJ1);
          V_line(bala2 - 1, POSY2, 8, 0XBBD);
          bala2++; // Mueve la bala hacia la derecha
          impact = Impacto(bala2, POSY2, 8, 8, POSX,POSY, 24, 33);
          if (impact == true)
          {
            String text_begin = "Gana J2";
              for(int k=0; k<3; k++) //despliega el texto de forma intermitente 3 veces 
             {           
                LCD_Print(text_begin,50,120 , 2, 0xffff, 0x0);
                delay(500);
                LCD_Clear(0x00);
             }
              
          }
        delay(5);
      }
     
    } else {
      // La bala ha salido de la pantalla
      balaEnMovimiento2 = false;
      bala2 = POSX2+22;
      delay(500);
    }
  }

  if (dir_dispJ2 == 1){
    if (!balaEnMovimiento2) 
    {
      bala2 = -11;
      bala2 = POSX2-2; // Restablece la posición de la bala a la posición inicial del jugador
      balaEnMovimiento2 = true; // Establece la bandera de movimiento de la bala a verdadero
    }
  
    if (bala2 < 321 && bala2 > -11 ) {
      while(bala2 >= -11 && impact == false )
      {       
          // Dibuja la bala en la posición actual
          LCD_Bitmap(bala2, POSY2, 8, 8, disparoJ1);
          V_line(bala2 + 8, POSY2, 8, 0XBBD);
          bala2--; // Mueve la bala hacia la derecha
          impact = Impacto(bala2, POSY2, 8, 8, POSX,POSY, 24, 33);
          if (impact == true)
          {
                String text_begin = "Gana J2";
                for(int k=0; k<3; k++) //despliega el texto de forma intermitente 3 veces 
               { 
                  LCD_Print(text_begin,50,120 , 2, 0xffff, 0x0);
                  delay(500);
                  LCD_Clear(0x00);
                }           
            }
          delay(5);
      }
    } else {
      // La bala ha salido de la pantalla
      balaEnMovimiento2 = false;
      bala2 = POSX2-2; 
      delay(500);
    }
  }
}

//****************************************
// impacto
//***************************************
bool Impacto(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2){
  return (x1 < x2 + w2) && (x1 + w1 > x2) && (y1 < y2 + h2) && (y1 + h1 > y2);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Funciones SD ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//****************************************
//Ascii → Hex
//***************************************
int ascii2hex(int a)
{
  switch(a)
  {
     case(48):
        return 0; //Caso 0
     case(49):
        return 1; //Caso 1
     case(50):
        return 2; //Caso 2
     case(51):
        return 3; //Caso 3
     case(52):
        return 4; //Caso 4
     case(53):
        return 5; //Caso 5
     case(54):
        return 6; //Caso 6
     case(55):
        return 7; //Caso 7
     case(56):  
        return 8; //Caso 8
     case(57):
        return 9; //Caso 9
     case(97):
        return 10; //Caso A
     case(98):
        return 11; //Caso B
     case(99):
        return 12; //Caso C
     case(100):
        return 13; //Caso D
     case(101):
        return 14; //Caso E
     case(102):
        return 15; //Caso F     
  }
}

//****************************************
// Obtencion imagen desde la SD
//***************************************
void mapeo_SD(char doc[], int x, int y,int w, int h)
{
  myFile = SD.open(doc, FILE_READ); //se toma el archivo de la imagen
  int hex1 = 0;
  int val1 = 0;
  int val2 = 0;
  int mapear = 0;
  int vertical = y;
  unsigned char maps[640]; //arreglo que la imagen Hex

   if(myFile) //si el archivo existe
   {
      while(myFile.available()) //mientras hayan datos disponibles
      {
        mapear = 0;
        while(mapear<640)
        {
          hex1 = myFile.read();
          if(hex1 == 120)
          {
            val1 = myFile.read();
            val2 = myFile.read();
            val1 = ascii2hex(val1);
            val2 = ascii2hex(val2);
            maps[mapear] = val1 * 16 + val2;
            mapear++;
          }
        }

        LCD_Bitmap(x,vertical, w, 1, maps);
        vertical++;
        if(vertical == y + h)
        {
          return;
        }
      }

      myFile.close();
   }
   else
   {
      Serial.println("no se encontro el archivo en la memoria");
      myFile.close(); 
   }
  
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Funciones pantalla  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//*********************************************
// Función para inicializar LCD
//*********************************************
void LCD_Init(void) {
  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_WR, OUTPUT);
  pinMode(LCD_RD, OUTPUT);
  for (uint8_t i = 0; i < 8; i++){
    pinMode(DPINS[i], OUTPUT);
  }


  
  //****************************************
  // Secuencia de Inicialización
  //****************************************
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_WR, HIGH);
  digitalWrite(LCD_RD, HIGH);
  digitalWrite(LCD_RST, HIGH);
  delay(5);
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
  digitalWrite(LCD_CS, LOW);
  //**************
  LCD_CMD(0xE9);  // SETPANELRELATED
  LCD_DATA(0x20);
  //**************
  LCD_CMD(0x11); // Exit Sleep SLEEP OUT (SLPOUT)
  delay(100);
  //**************
  LCD_CMD(0xD1);    // (SETVCOM)
  LCD_DATA(0x00);
  LCD_DATA(0x71);
  LCD_DATA(0x19);
  //**************
  LCD_CMD(0xD0);   // (SETPOWER) 
  LCD_DATA(0x07);
  LCD_DATA(0x01);
  LCD_DATA(0x08);
  //**************
  LCD_CMD(0x36);  // (MEMORYACCESS)
  LCD_DATA(0x40|0x80|0x20|0x08); // LCD_DATA(0x19);
  //**************
  LCD_CMD(0x3A); // Set_pixel_format (PIXELFORMAT)
  LCD_DATA(0x05); // color setings, 05h - 16bit pixel, 11h - 3bit pixel
  //**************
  LCD_CMD(0xC1);    // (POWERCONTROL2)
  LCD_DATA(0x10);
  LCD_DATA(0x10);
  LCD_DATA(0x02);
  LCD_DATA(0x02);
  //**************
  LCD_CMD(0xC0); // Set Default Gamma (POWERCONTROL1)
  LCD_DATA(0x00);
  LCD_DATA(0x35);
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x02);
  //**************
  LCD_CMD(0xC5); // Set Frame Rate (VCOMCONTROL1)
  LCD_DATA(0x04); // 72Hz
  //**************
  LCD_CMD(0xD2); // Power Settings  (SETPWRNORMAL)
  LCD_DATA(0x01);
  LCD_DATA(0x44);
  //**************
  LCD_CMD(0xC8); //Set Gamma  (GAMMASET)
  LCD_DATA(0x04);
  LCD_DATA(0x67);
  LCD_DATA(0x35);
  LCD_DATA(0x04);
  LCD_DATA(0x08);
  LCD_DATA(0x06);
  LCD_DATA(0x24);
  LCD_DATA(0x01);
  LCD_DATA(0x37);
  LCD_DATA(0x40);
  LCD_DATA(0x03);
  LCD_DATA(0x10);
  LCD_DATA(0x08);
  LCD_DATA(0x80);
  LCD_DATA(0x00);
  //**************
  LCD_CMD(0x2A); // Set_column_address 320px (CASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x3F);
  //**************
  LCD_CMD(0x2B); // Set_page_address 480px (PASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0xE0);
//  LCD_DATA(0x8F);
  LCD_CMD(0x29); //display on 
  LCD_CMD(0x2C); //display on

  LCD_CMD(ILI9341_INVOFF); //Invert Off
  delay(120);
  LCD_CMD(ILI9341_SLPOUT);    //Exit Sleep
  delay(120);
  LCD_CMD(ILI9341_DISPON);    //Display on
  digitalWrite(LCD_CS, HIGH);
}



//*********************************************
// Función para enviar comandos a la LCD - parámetro (comando)
//*********************************************
void LCD_CMD(uint8_t cmd) {
  digitalWrite(LCD_RS, LOW);
  digitalWrite(LCD_WR, LOW);
  GPIO_PORTB_DATA_R = cmd;
  digitalWrite(LCD_WR, HIGH);
}



//*********************************************
// Función para enviar datos a la LCD - parámetro (dato)
//*********************************************
void LCD_DATA(uint8_t data) {
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_WR, LOW);
  GPIO_PORTB_DATA_R = data;
  digitalWrite(LCD_WR, HIGH);
}


//*********************************************
// Función para definir rango de direcciones de memoria con las cuales se trabajara (se define una ventana)
//*********************************************
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) {
  LCD_CMD(0x2a); // Set_column_address 4 parameters
  LCD_DATA(x1 >> 8);
  LCD_DATA(x1);   
  LCD_DATA(x2 >> 8);
  LCD_DATA(x2);   
  LCD_CMD(0x2b); // Set_page_address 4 parameters
  LCD_DATA(y1 >> 8);
  LCD_DATA(y1);   
  LCD_DATA(y2 >> 8);
  LCD_DATA(y2);   
  LCD_CMD(0x2c); // Write_memory_start
}


//*********************************************
// Función para borrar la pantalla - parámetros (color)
//*********************************************
void LCD_Clear(unsigned int c){  
  unsigned int x, y;
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);   
  SetWindows(0, 0, 319, 239); // 479, 319);
  for (x = 0; x < 320; x++)
    for (y = 0; y < 240; y++) {
      LCD_DATA(c >> 8); 
      LCD_DATA(c); 
    }
  digitalWrite(LCD_CS, HIGH);
} 


//*********************************************
// Función para dibujar una línea horizontal - parámetros ( coordenada x, cordenada y, longitud, color)
//********************************************* 
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {  
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + x;
  SetWindows(x, y, l, y);
  j = l;// * 2;
  for (i = 0; i < l; i++) {
      LCD_DATA(c >> 8); 
      LCD_DATA(c); 
  }
  digitalWrite(LCD_CS, HIGH);
}


//*********************************************
// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
//********************************************* 
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {  
  unsigned int i,j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + y;
  SetWindows(x, y, x, l);
  j = l; //* 2;
  for (i = 1; i <= j; i++) {
    LCD_DATA(c >> 8); 
    LCD_DATA(c);
  }
  digitalWrite(LCD_CS, HIGH);  
}


//*********************************************
// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//*********************************************
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  H_line(x  , y  , w, c);
  H_line(x  , y+h, w, c);
  V_line(x  , y  , h, c);
  V_line(x+w, y  , h, c);
}


//*********************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//*********************************************
/*void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  unsigned int i;
  for (i = 0; i < h; i++) {
    H_line(x  , y  , w, c);
    H_line(x  , y+i, w, c);
  }
}
*/

void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW); 
  
  unsigned int x2, y2;
  x2 = x+w;
  y2 = y+h;
  SetWindows(x, y, x2-1, y2-1);
  unsigned int k = w*h*2-1;
  unsigned int i, j;
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      LCD_DATA(c >> 8);
      LCD_DATA(c);
      
      //LCD_DATA(bitmap[k]);    
      k = k - 2;
     } 
  }
  digitalWrite(LCD_CS, HIGH);
}


//*********************************************
// Función para dibujar texto - parámetros ( texto, coordenada x, cordenada y, color, background) 
//*********************************************
void LCD_Print(String text, int x, int y, int fontSize, int color, int background) {
  int fontXSize ;
  int fontYSize ;
  
  if(fontSize == 1){
    fontXSize = fontXSizeSmal ;
    fontYSize = fontYSizeSmal ;
  }
  if(fontSize == 2){
    fontXSize = fontXSizeBig ;
    fontYSize = fontYSizeBig ;
  }
  
  char charInput ;
  int cLength = text.length();
  Serial.println(cLength,DEC);
  int charDec ;
  int c ;
  int charHex ;
  char char_array[cLength+1];
  text.toCharArray(char_array, cLength+1) ;
  for (int i = 0; i < cLength ; i++) {
    charInput = char_array[i];
    Serial.println(char_array[i]);
    charDec = int(charInput);
    digitalWrite(LCD_CS, LOW);
    SetWindows(x + (i * fontXSize), y, x + (i * fontXSize) + fontXSize - 1, y + fontYSize );
    long charHex1 ;
    for ( int n = 0 ; n < fontYSize ; n++ ) {
      if (fontSize == 1){
        charHex1 = pgm_read_word_near(smallFont + ((charDec - 32) * fontYSize) + n);
      }
      if (fontSize == 2){
        charHex1 = pgm_read_word_near(bigFont + ((charDec - 32) * fontYSize) + n);
      }
      for (int t = 1; t < fontXSize + 1 ; t++) {
        if (( charHex1 & (1 << (fontXSize - t))) > 0 ) {
          c = color ;
        } else {
          c = background ;
        }
        LCD_DATA(c >> 8);
        LCD_DATA(c);
      }
    }
    digitalWrite(LCD_CS, HIGH);
  }
}


//*********************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//*********************************************
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]){  
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW); 
  
  unsigned int x2, y2;
  x2 = x+width;
  y2 = y+height;
  SetWindows(x, y, x2-1, y2-1);
  unsigned int k = 0;
  unsigned int i, j;

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      //LCD_DATA(bitmap[k]);    
      k = k + 2;
     } 
  }
  digitalWrite(LCD_CS, HIGH);
}


//*********************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//*********************************************
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],int columns, int index, char flip, char offset){
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW); 

  unsigned int x2, y2;
  x2 =   x+width;
  y2=    y+height;
  SetWindows(x, y, x2-1, y2-1);
  int k = 0;
  int ancho = ((width*columns));
  if(flip){
  for (int j = 0; j < height; j++){
      k = (j*(ancho) + index*width -1 - offset)*2;
      k = k+width*2;
     for (int i = 0; i < width; i++){
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      k = k - 2;
     } 
  }
  }else{
     for (int j = 0; j < height; j++){
      k = (j*(ancho) + index*width + 1 + offset)*2;
     for (int i = 0; i < width; i++){
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      k = k + 2;
     } 
  }
    
    
    }
  digitalWrite(LCD_CS, HIGH);
}
