#include <PinChangeInt.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>
#include <Timelib.h>

#define MOTOR1 22
#define MOTOR2 24
#define MOTOR3 26
#define SELECT_BTN 9
#define UP_BTN 10
#define DOWN_BTN 8
#define DS1307_I2C_ADDRESS 0x68

byte ledcontrolv = 13;
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

byte decToBcd(byte val) { return ( (val/10*16) + (val%10) ); } 
// Convierte BCD (binario decimal codificado) a números normales decimales
byte bcdToDec(byte val){ return ( (val/16*10) + (val%16) ); }
void setDateDs1307(byte second,  byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year) 
 { 
 	Wire.beginTransmission(DS1307_I2C_ADDRESS); 
 	Wire.write(0); 
 	Wire.write(decToBcd(second)); // 0 a bit 7 inicia el reloj 
 	Wire.write(decToBcd(minute)); 
 	Wire.write(decToBcd(hour)); // Si usted quiere 12 hora am/pm usted tiene que poner el // bit 6 (tambien tiene que cambiar readDateDs1307) 
 	Wire.write(decToBcd(dayOfWeek)); 
 	Wire.write(decToBcd(dayOfMonth)); 
 	Wire.write(decToBcd(month)); 
 	Wire.write(decToBcd(year)); 
 	Wire.endTransmission();
 } 
// Establece la fecha y el tiempo del ds1307 
 void getDateDs1307(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year) { 
 // Resetea el registro puntero 
 	Wire.beginTransmission(DS1307_I2C_ADDRESS);
 	Wire.write(0); 
 	Wire.endTransmission(); 
 	Wire.requestFrom(DS1307_I2C_ADDRESS, 7); // Alguno de estos necesitan enmascarar porque ciertos bits son bits de control	 
 	*second = bcdToDec(Wire.read() & 0x7f); 
 	*minute = bcdToDec(Wire.read()); 
 	*hour = bcdToDec(Wire.read() & 0x3f); // Need to change this if 12 hour am/pm 
 	*dayOfWeek = bcdToDec(Wire.read()); 
 	*dayOfMonth = bcdToDec(Wire.read()); 
 	*month = bcdToDec(Wire.read()); 
 	*year = bcdToDec(Wire.read()); 
 } 
 /////////////// //ILUMINACION// ///////////////
 byte ledazul = 5 , ledblanco = 6 , ventled = 7;//Luces //Potencias 
 int pamanecer= 15; // Potencia amanecer 0-100% 
 int pdia= 40; // Potencia luz dia 0-100% 
 int psol= 60; // Potencia luz Sol 0-100% 
 int patardecer= 15; // Potencia atardecer 0-100% 
 int pluna=	 1; // Potencia de luna (1-10 valores PWM solo leds azules) //Horas
 int hamanecer= 11; // Hora fase amanecer 
 int hdia= 12; // Hora fase día 
 int hsol= 14; // Hora fase Sol 
 int tsol=	 4; // Duracion de Sol , horas 
 int hatardecer= 20; // Hora atardecer 
 int hluna=	21; // Hora fase luna 
 int tluna= 1; // Duración luna , horas 
 int vpwm=	10; // Tiempo del efecto en cada fase, minutos 
 //Calculos para el programa no modificar 
 int pwm=0; 
 int pwmamanecer=(pamanecer*2.55);//Calculamos la potencia de salida 
 int pwmdia=(pdia*2.55); 
 int pwmsol=(psol*2.55); 
 int pwmatardecer=(patardecer*2.55); 
 int pwmluna=pluna; 
 int faseluz = 0; 
 int delayamanecer=(vpwm*60000)/(pwmamanecer);
 //Calculamos cada cuanto tiempo da un paso de intensidad 
 int delaydia=(vpwm*60000)/(pwmdia-pwmamanecer); 
 int delaysol=(vpwm*60000)/(pwmsol-pwmdia);
 int delayatardecer=(vpwm*60000)/(pwmsol-pwmatardecer); 
 int delayluna=(vpwm*60000)/(pwmatardecer-pwmluna);   

RTC_DS1307 RTC;

LiquidCrystal lcd(12,11,5,4,3,2);

int flag = 0;
volatile int currentScreen;
volatile int menuSelected;
int carretPos,ledState ;
int changed = 0;
int blinking = 0;
int size_alarma = 0;

void printMenu(int current);
void printLogo();
void printTime(int menuSelected);
int alarmMenu(int menuSelected);
void checkAlarms(Calendario &c);


Calendario *tmp;
//Calendario *alarmas[3];
Calendario **alarmas;
int Up(void);
int Down(void);
int Right(void);
int Left(void);
int Select(void);

volatile int currentPulsed = 0;
volatile int changing = 0;
volatile int secPress = 0;


Calendario* createCal(int h_ini, int h_fin, int min_ini, int min_fin, int sec_ini, int sec_fin,int len, int *dias,int motor, int tipo){
  Hora h = {h_ini, h_fin, min_ini, min_fin, sec_ini, sec_fin};
  Calendario *tmp;
  tmp = (Calendario*) malloc(sizeof(struct Calendario));
  tmp->h =h;
  tmp->lenDias = len;
  tmp->motor = motor;
  tmp->Dias = (int*) malloc(sizeof(int)*tmp->lenDias);
  tmp->tipo = tipo;
  for(int i = 0;i<tmp->lenDias;i++)tmp->Dias[i] = dias[i];
  return tmp;
}

Calendario** addAlarm(Calendario **a, Calendario *c){
  Calendario **tmp = (Calendario **) malloc(sizeof(Calendario*)*(size_alarma+1));
  for(int i = 0; i< size_alarma; i++){
    tmp[i] = a[i];
  }
  tmp[size_alarma] = c;
  size_alarma++;
  return tmp;
}

void changeHour(int time_d);

void keyboardfunc(int val){
  Serial.println(val);
  switch(val) {
    case 1:
    Serial.println("SELECT");
    currentScreen = Select();
    break;
    case 10:
    Serial.println("UP");
    menuSelected = Left();
    break;
    case 100:
    Serial.println("DOWN");
    menuSelected = Right();
    break;
  }
}

int potencia(int base, int exponente){
  int n = 1;
  while(exponente-- > 0)
    n *= base;
  return n; 
}
/*
*/
int Navigation(int menu){
   switch(menu){
    case 1:
      Serial.print("OK");
      currentScreen = Select(); 
      break;
    case 2:
      menuSelected = Down();
      break;
    case 3:
      menuSelected = Up();
      break;
      //Serial.print("Arriba ");
    case 4:
      menuSelected = Left();
      break;
    //Serial.print("Izquierda ");
    case 5:
      menuSelected = Right();
      break;
    //Serial.print("Derecha ");
  }
  return menuSelected;
}
//alarm icon
byte alarma[8] = {
        B11011,
        B11011,
        B01110,
        B10001,
        B11001,
        B10101,
        B10001,
        B01110
};
//alarm icon
byte settings_ico[8] = {
        B00000,
        B00000,
        B11111,
        B00000,
        B11111,
        B00000,
        B11111,
        B00000
};
byte play_ico[8] = {
        B00000,
        B10000,
        B11000,
        B11100,
        B11110,
        B11100,
        B11000,
        B10000
};
void checkAlarms(Calendario &c){
  DateTime d = RTC.now();
  int found = 0,s_ini, s_fin, s_act;
  if(c.tipo == -1) {
  for(int j = 0; j<size_alarma;j++){
    for(int i=0;i<c.lenDias;i++){ 
     if(d.dayOfWeek() == c.Dias[i]) {
       
        s_ini = c.h.h_inicio * 3600 + c.h.m_inicio*60 + c.h.s_inicio;
        s_fin = c.h.h_final * 3600 + c.h.m_final*60 + c.h.s_final;
        s_act = d.hour()*3600 + d.minute()*60+d.second();
        if(s_act - s_ini > 0 && s_act - s_fin < 0){
          found = 1;
        }
        else if(s_act - s_fin > 0){
          digitalWrite(c.motor,LOW);
        }
      
     }
    }
    }
   } else {
     if(c.tipo < 0) {
       c.tipo++;
       found = 1;
       if(c.tipo == -2) {c.tipo = 0;}
     } 
     else {
        if ( c.tipo == 0 ) {
          c.tipo = 2*(c.h.h_final * 3600 + c.h.m_final*60 + c.h.s_final);
        }
        if(c.tipo == 1) {
                found = 1;
                
                c.tipo = -2*(c.h.h_inicio * 3600 + c.h.m_inicio*60 + c.h.s_inicio);
         } else {
          if(c.tipo > 1){
           c.tipo--;
          }
        }
     }
 if(!found)digitalWrite(c.motor,LOW);    
   }
   if(found == 1) digitalWrite(c.motor,HIGH);

}
/*
int *addDayToAlarm(int *a,int d){
  int len = sizeof(*a)/sizeof(a[0]);
  int (*tmp) = (int)malloc((len+1) * sizeof(int));
  for(int i = 0; i < len; i++){
    tmp[i] = a[i];
  }
  tmp[len] = d;
  return tmp;
}*/

void setup(){
  //VARIOS 
  pinMode (ledcontrolv, OUTPUT);
  //leds control 
  lcd.createChar(0, alarma);
  lcd.createChar(1, settings_ico);
  lcd.createChar(2, play_ico);
  lcd.begin(16,2);
  Wire.begin();
  RTC.begin();
  Serial.begin(9600);
  
  pinMode (ventled,OUTPUT); 
  digitalWrite (ventled,LOW); 
  pinMode (ledazul, OUTPUT); 
  pinMode (ledblanco, OUTPUT);
  
  pinMode(MOTOR1,OUTPUT);
  pinMode(MOTOR2,OUTPUT);
  pinMode(MOTOR3,OUTPUT);
  pinMode(SELECT_BTN,INPUT_PULLUP);
  pinMode(UP_BTN,INPUT_PULLUP);
  pinMode(DOWN_BTN,INPUT_PULLUP);
  ledState = HIGH;
  digitalWrite(MOTOR1,LOW);
  digitalWrite(MOTOR2,LOW);
  digitalWrite(MOTOR3,LOW);
  int dias[7] = {1,2,3,4,5,6,7};
  tmp = createCal (0,0,0,0,0,0,0,dias,MOTOR1,-1); 
  alarmas = addAlarm(alarmas, createCal (1,1,5,6,0,0,7,dias,MOTOR1,-1));
  alarmas = addAlarm(alarmas, createCal (0,0,0,0,50,15,7,dias,MOTOR2,0));
  alarmas = addAlarm(alarmas, createCal (1,1,7,8,0,0,7,dias,MOTOR3,-1));
  alarmas = addAlarm(alarmas, createCal (1,1,8,9,0,40,7,dias,MOTOR2,-1));
  if (!RTC.isrunning()) {
    Serial.println("Initializing");
    RTC.adjust(DateTime(__DATE__,__TIME__));
    Serial.println(RTC.now().hour());
  }
  carretPos = 0;
  currentScreen = 0;
  menuSelected = 0;
  //pinMode(A0,INPUT);

  
}

void loop(){
  if(flag == 0){
    flag = 1;
    lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("H2OTERRA");
   lcd.setCursor(0,1);
   lcd.print("Motor Controller"); 
    delay(3000); 
  }

  //ledvoff();//Apagamos led control 
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year); 
  //Miramos la hora 
  luz();// Ejecutamos el programa Luz 
  //ledvon(); //Encendemos led control 
  
  int sel = digitalRead(SELECT_BTN);
  int up = digitalRead(UP_BTN);
  int down = digitalRead(DOWN_BTN);
  if(sel == LOW) {keyboardfunc(1);
  }else {
    if(up == LOW){keyboardfunc(10);
    }else {
      if(down == LOW)keyboardfunc(100);
    }
  }
  

    for(int i = 0; i<size_alarma;i++){
      checkAlarms(*alarmas[i]);
    }

  switch(currentScreen){
   case 0:
      printTime(menuSelected,currentScreen);
      break;
   case 1:
      digitalWrite(MOTOR1,ledState);
      break;
   case 2:
   case 4:
      alarmMenu(menuSelected);
      break;
   case 3:
      printTime(menuSelected,currentScreen);
      break;
   case 5:
      chooseAlarm(menuSelected);
  }

  delay(500);
  
};
/*
*  Functions Declaration
*/

void printMenu(int current = 0){
  lcd.clear();
  lcd.setCursor(0,current);
  lcd.print(">");
  lcd.setCursor(2,0);
  lcd.print((char)0);
  lcd.setCursor(2,1);
  lcd.print((char)2);  
}

void printLogo(){
   menuSelected = 0;
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("H2OTERRA");
   lcd.setCursor(0,1);
   lcd.print("Motor Controller"); 
}

int Right(){
  switch(currentScreen){
    case 0:
      return (menuSelected + 1)%3;
      break;
    case 2:
    case 3:
    case 4:
      return (menuSelected + 1)%4;
      break;
    case 5:
      return (menuSelected + 1)%2;
      break;
  }
}
int Left(){

  switch(currentScreen){
    case 0:
      if(menuSelected <= 0) return 2;
      else{
        return (menuSelected - 1)%3;
      }
    break;
    case 2:
    case 3:
    case 4:
      if(menuSelected <= 0) return 0;
      else{
        return (menuSelected - 1)%4;
      }
    break;
   case 5:
     if(menuSelected <= 0) return 0;
        else{
          return (menuSelected - 1)%2;
        }
       break;
  }   
}
int Select(void){
 switch(currentScreen){
       case 0:
         switch(menuSelected){
             case 0:
              menuSelected = 0;
              ledState = !ledState;
              return 2;
              break;
             case 1:
              menuSelected = 0;
              return 3;
              break;
             case 2:
              menuSelected = 0;
              return 1;
              break;
         }
         break;
       case 2:
       if(menuSelected==3){
            menuSelected = 0;
            return 4; 
          }else{
            int total = potencia(60,2-menuSelected);
            int h = total/3600;
            int m = (total-(h*3600))/60;
            int s = (total-(h*3600)-(m*60));
            tmp->h.h_inicio += h;
            tmp->h.m_inicio += m;
            tmp->h.s_inicio += s;
            return 2;
          }
          break;
       case 3:
          if(menuSelected==3){
            
            menuSelected = 0;
            return 0; 
          }else{
            changeHour(potencia(60,2-menuSelected));
            return 3;
          }
          break;

     case 4:
       if(menuSelected==3){
            menuSelected = 0;
            return 5; 
          }else{
            int total = potencia(60,2-menuSelected);
            int h = total/3600;
            int m = (total-(h*3600))/60;
            int s = (total-(h*3600)-(m*60));
            tmp->h.h_final += h;
            tmp->h.m_final += m;
            tmp->h.s_final += s;
            return 4;
          }
          break; 
     case 5:
         if(menuSelected==1){
            menuSelected = 0;
            
            alarmas = addAlarm(alarmas, tmp);
            
            tmp->h.h_inicio = 0;
            tmp->h.m_inicio = 0;
            tmp->h.s_inicio = 0;
            tmp->h.h_final = 0;
            tmp->h.m_final = 0;
            tmp->h.s_final = 0;
            tmp->motor = MOTOR1;
            return 0; 
          } else {
            tmp->motor+=2;
         if(tmp->motor > MOTOR3) tmp->motor = MOTOR1;
         return 5;
          }
         
     break;
     default:
           menuSelected = 0;
           return 0;
           break;   
    } 
}

int hourChange(DateTime now){
  if(now.month() == 10 || now.month() == 3){
    if(now.dayOfWeek() == 0 && now.day()+7 > 31 && now.hour() == 3 && !changed){
      changed = 1;
      if(now.month() == 10)
        return -1;
      else 
        return 1;
    } 
  }
  changed = 0;
  return 0;
   
}

void chooseAlarm (int menuSelected) {
  Serial.println(menuSelected);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Luz");
  lcd.print(tmp->motor);
  lcd.setCursor(8,0);
  if(menuSelected == 1){
    if(blinking)lcd.print(" ");
    else lcd.print("Ok");
  } else {
    lcd.print("Ok");
  }
  blinking = !blinking;
}

void printTime(int menuSelected,int currentScreen){
  DateTime now = RTC.now();
  int cambio = hourChange(now);
  if(cambio != 0){
      changeHour((3600 * cambio));
  }
  
  lcd.clear();
  switch(currentScreen){
    case 0:
      lcd.setCursor(8,0);
      
      lcd.print(now.hour());
      lcd.print(':');
      lcd.print(now.minute());
      lcd.print(':');
      lcd.print(now.second());
      lcd.setCursor(1,1);
      lcd.print((char)0);
      lcd.setCursor(4,1);
      lcd.print((char)1);
      lcd.setCursor(7,1);
      lcd.print((char)2); 
      lcd.setCursor(menuSelected*3,1);
      lcd.print(">");
      break;
    case 3:
      lcd.setCursor(2,0);
      if(menuSelected == 0){
        if(blinking){
          if(now.hour() < 10){
            lcd.print("0");
            lcd.print(now.hour());
          }
          else lcd.print(now.hour());
        }
        else{
          lcd.print("  "); 
        }
        lcd.print(':');
        lcd.print(now.minute());
        lcd.print(':');
        lcd.print(now.second());  
        lcd.print(" OK");
      }

      if(menuSelected == 1){
        lcd.print(now.hour());
        lcd.print(':');
        if(blinking){
          if(now.minute() < 10){
            lcd.print("0");
            lcd.print(now.minute());
          }
          else lcd.print(now.minute());
        }
        else{
          lcd.print("  ");
        }
        lcd.print(':');
        lcd.print(now.second());
        lcd.print(" OK");      
      }
  
      if(menuSelected == 2){
        lcd.print(now.hour());
        lcd.print(':');
        lcd.print(now.minute());
        lcd.print(':');
        if(blinking){
          if(now.second() < 10){
            lcd.print("0");
            lcd.print(now.second());
          }
          else lcd.print(now.second());
        }
        else{
          lcd.print("  ");
        }
        lcd.print(" OK");    
      }
      if(menuSelected == 3){
        lcd.print(now.hour());
        lcd.print(':');
        lcd.print(now.minute());
        lcd.print(':');
        lcd.print(now.second());
        if(blinking){
          lcd.print("   ");
        }
        else{
          lcd.print(" OK");
        }
  
      }
      blinking = !blinking;
      break;
  }
}

void changeHour(int time_d){
  Serial.println("Entro");
  Serial.println(time_d);
  RTC.adjust(RTC.now().unixtime() + time_d);
}

int alarmMenu(int menuSelected){
  int h,m,s;
  char *t;
  switch(currentScreen){
    
     case 2:
       h = tmp->h.h_inicio;
       m = tmp->h.m_inicio;
       s = tmp->h.s_inicio;
       t = "H Inicio";
       break;
     case 4:
       h = tmp->h.h_final;
       m = tmp->h.m_final;
       s = tmp->h.s_final;
       t = "H Final";
       break;   
  } 
  lcd.clear();
  lcd.setCursor(2,0);
      if(menuSelected == 0){
        if(blinking){
          if(h < 10){
            lcd.print("0");
            lcd.print(h);
          }
          else lcd.print(h);
        }
        else{
          lcd.print("  "); 
        }
        lcd.print(':');
        lcd.print(m);
        lcd.print(':');
        lcd.print(s);  
        lcd.print(" OK");
      }

      if(menuSelected == 1){
        lcd.print(h);
        lcd.print(':');
        if(blinking){
          if(m < 10){
            lcd.print("0");
            lcd.print(m);
          }
          else lcd.print(m);
        }
        else{
          lcd.print("  ");
        }
        lcd.print(':');
        lcd.print(s);
        lcd.print(" OK");      
      }
  
      if(menuSelected == 2){
        lcd.print(h);
        lcd.print(':');
        lcd.print(m);
        lcd.print(':');
        if(blinking){
          if(s < 10){
            lcd.print("0");
            lcd.print(s);
          }
          else lcd.print(s);
        }
        else{
          lcd.print("  ");
        }
        lcd.print(" OK");    
      }
      if(menuSelected == 3){
        lcd.print(h);
        lcd.print(':');
        lcd.print(m);
        lcd.print(':');
        lcd.print(s);
        if(blinking){
          lcd.print("   ");
        }
        else{
          lcd.print(" OK");
        }
  
      }
      blinking = !blinking;
      lcd.setCursor(0,1);
      lcd.print(t);
  return 0;
}

void luz () {
  //FASE 1 - AMANECER 
  if (hour >= hamanecer && hour < hdia && faseluz ==0)
  //FASE1 
  { 
  	if (pwm!=pwmamanecer)
  	//Hasta que el valor de pwm no llege al valor pwmamanecer ira aumentandolo 
  	{ 
  		analogWrite (ledazul,pwm);
  		//Escribimos la potencia en el canal de leds azules 
  		analogWrite (ledblanco,pwm);
  		//Lo mismo para el blanco 
  		pwm++;
  		//sube un paso la intensidad 
  		delay(delayamanecer);
  		//Esperamos para cumplir el tiempo de dimeado deseado 
  	} else {faseluz = 1;}//Cuando el valor pwm ya ha llegado al deseado pasaremos a la siguiente fase 
  } 
  //FASE 2 - DIA 
  if (hour >= hdia && hour < hsol && faseluz <2)
  //FASE2 
  { 
  	if (pwm!=pwmdia) { analogWrite (ledazul,pwm); analogWrite (ledblanco,pwm); pwm++; delay(delaydia); } else {faseluz = 2;} 
  }
  //FASE 3 - SOL 
  if (hour >= hsol && hour < hatardecer && faseluz <3)
  //FASE3 
  { if (pwm!=pwmsol) { analogWrite (ledazul,pwm); analogWrite (ledblanco,pwm); pwm++; delay(delaysol); } else {faseluz = 3;} 
  } 
  //FASE 4 - ATARDECER I 
  if (hour >= hsol+tsol && hour < hatardecer && faseluz <4)
  //FASE4 
  { 
  	if (pwm!=pwmdia) { if (pwm==255){pwm=pwmdia-1;} 
  //Por si reiniciamos en esta fase 
  	analogWrite (ledazul,pwm); 
	analogWrite (ledblanco,pwm); 
	pwm--; 
	delay(delaysol); 
  } else {faseluz = 4;} 
  } 
  //FASE 5 - ATARDECER II 
  if (hour >= hatardecer && hour < hluna && faseluz <5)
  //FASE5
  { 
  	if (pwm!=pwmatardecer) { if (pwm==0){pwm=pwmatardecer+1;} 
  	//Por si reiniciamos en esta fase
  	analogWrite (ledazul,pwm); 
  	analogWrite (ledblanco,pwm); 
  	pwm--;
  	delay(delayatardecer); 
  } else {faseluz = 5;} } 
  //FASE 6 - LUNA 
  if (hour >= hluna && hour < hluna+tluna && faseluz <6)
  //FASE6 
  { if (pwm!=pwmluna) { if (pwm==255){pwm=pwmluna-1;} //Por si reiniciamos en esta fase 
  analogWrite (ledazul,pwm); 
  analogWrite (ledblanco,pwm);
  pwm--; 
  delay(delayluna); 
  } else { faseluz = 6; analogWrite (ledblanco,0); } } 
  //FASE 7 - APAGADO 
  if (hour >=hluna+tluna && faseluz==6) { if (pwm!=0) { pwm--; analogWrite (ledazul,pwm); delay(delayluna); } else{faseluz=0;} } 
} 

