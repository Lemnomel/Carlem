#include <SPI.h>      
#include <EEPROM2.h>
#include <SD.h>
#include <Wire.h>                         
#include "RTClib.h"
#include <Base64.h>  
#include "OLedI2C.h"
#include <PString.h>
#include <TVoutSerial.h>
OLedI2C LCD;                
TVoutSerial TV;

RTC_DS1307 RTC;
File myFile;     
int countmeasure=0;  
int Btpin = 3;                    
int ResetPin = 4;
int ledPin = 13;                                   
int inPin = 7;                                           
int inPin1 = 6; 
int Pinstart = 8; 
int datatooledmode[4]={0,1,2,3}; //mode 0-acc off, engine off, ecu off;  //mode 1-acc on, engine off, ecu off;  // mode 2-acc on, engine off, ecu on; // mode 3-acc on, engine on, ecu on;
int savemode;
const int ACCPin = A8;    
const int SFuelPin = A1;                                    
int val = 0;                                                        
unsigned int n=0;
unsigned long Eventcount=0;
byte countpacket;
char* dayweek[]={"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
unsigned int serialspeed=9600; 
unsigned int logsize=30;
unsigned int logind[30];
unsigned long logtime[30];
byte logevent[30];
double logvolume[30];
double logdistance[30];
unsigned long logtimeengine[30];
unsigned int logstartcount[30];
unsigned long lastsynctime;
byte STstage;
boolean STsimon=false;
byte MPleight;
byte MPleightbyte;
byte MPdata[256];
byte lastbyte=0;
byte buffer3[256];
byte endbuffer3=0;
byte beginpacket3=0;
byte leghtpacket3=0;
byte indpacket3=0;
byte verifypacket3=244;
byte buffer2[256];
byte endbuffer2=0;
byte beginpacket2=0;
byte leghtpacket2=0;
byte indpacket2=0;
byte verifypacket2=254;
unsigned long timelastpacket=0;
byte myArray[69]={244,87,1,0,180,244,146,1,7,65,0,0,0,0,143,50,162,241,163,55,79,159,35,111,25,0,61,140,128,0,71,66,66,134,21,25,35,35,0,0,40,0,5,182,1,210,146,120,2,152,175,0,0,255,0,128,13,0,176,12,194,1,88,2,0,165,228,184,173};
double Volume=0;            
unsigned long Volumelong=0;            
unsigned int Tankfuel=0;//мл            
double Volumebeforesrart=0;
unsigned long Volumesave=0;
double Distancebeforestart=0;
double Distance=0;    
unsigned long Distancelong=0;    
double flow=0;
double flowinjector=219;          
unsigned long Worktimeingine=0;     
unsigned long Worktimeinginelong=0;    
double Pricefuel=30;  //руб/л      
double Cash=0;         
unsigned long Cashlong=0;         
unsigned long Cashbeforestart=0;         

byte byteinpacket=1;
byte bytevalue=1;
unsigned long momentbytevalue=0;
unsigned long moment=0;
unsigned long moment1=0;
unsigned long Worktimeinginebeforestart=0;
unsigned long interval=0;
byte proverka=0;
unsigned int Countstartengine=0;
int lastprm=0;
                                          
String incomingdata="";
String messagesim900;
                         
boolean led=false;
boolean D=false;
boolean Btpush=false;
boolean packetfind=false;

byte flags=0;
const byte FLAG_autoquery=B10000000;
const byte FLAG_SIM900enable=B01000000;
const byte FLAG_ircenable=B00100000;
const byte FLAG_bridgePCECU=B00010000;
const byte FLAG_bridgePCSIM900=B00001000;
           
long timesendautoquery=0;
int delaytestpacket=0;
int periodautoquery=400  ;
unsigned int queryperiod=400;
unsigned int querydelay=50;
String saveTypedata;
File logFile;
File root;


class TASK
{
  public:
  unsigned long taskstart[10];
  unsigned long taskperiod[10];
  String taskcommand[10];
  unsigned long taskcount[10];
  TASK(int i){;};
  int add(int item, String command,unsigned long start, unsigned long period, unsigned long count){
    taskstart[item]=start;
    taskperiod[item]=period;
    taskcount[item]=count;
    taskcommand[item]=command;
  }
  void runtask(){
    for (int ti=0;ti<10;ti++){
      if (!(taskcount[ti]==0)){
        if (D) Serial2.println("t" + String(ti));
        if(millis()>taskstart[ti]){
          taskstart[ti]=millis()+taskperiod[ti];
          if (taskcount[ti]>0){taskcount[ti]=taskcount[ti]-1;}
//          Serial2.println("Task: " + taskcommand[i] + " run");  
          Runcommand(taskcommand[ti]);
        }
      }
    }
  }
  void restarttask(int item){
    taskstart[item]=millis()+taskperiod[item];
  }
  void restarttask(int item, unsigned long delaytime){
    taskstart[item]=millis()+delaytime;
  }
  
};

TASK Mytask(1);

class SIM900
{
  public:
    int powerpin=-1;
    boolean busy=false;
    boolean on=false;
    boolean ircwork=false;
    String lastcommand="";
    String answer="";  
    String incomingprefix="And:";
    String outgoingprefix="Ard:";
    String ircadress; 
    int ircport; 
    String ircnick; 
    String ircuser; 
    String ircchanel; 
    String ircpassword;
    
    SIM900(int pin){powerpin=pin;};
    
    boolean poweron(){
         Serial2.println("poweron");
         if(sendcomand("AT","OK\r\n", 1000, 1)){on=true; return true;}else{on=false;}
         power();
         if(!sendcomand("","RDY",3000,1)){return true;}
         power();
         return sendcomand("AT","OK\r\n", 1000, 3);
    }
    boolean poweroff(){
         Serial2.println("poweroff");
         if (!on)return true;         
         power();
         if(!sendcomand("","NORMAL POWER DOWN",3000,1)){return true;}
//         if (!on)return true; 
//         if(!sendcomand("","NORMAL POWER DOWN",3000,1)){return true;}
//         power();
//         if(!sendcomand("","NORMAL POWER DOWN",3000,1)){return true;}
//         if (!on)return true;
         return false; 
    }
    void power(){
      if (powerpin>-1){
        digitalWrite(powerpin, HIGH);
        delay(1500);
        digitalWrite(powerpin, LOW);        
      }
    }
    boolean sendcomand(String command,String OK, unsigned long timedelay, int attempts){
      if (busy){Serial2.println("SIM900 busy " + lastcommand);return false;}
        answer="";
        lastcommand=command;
        busy=true;
        for(int i=attempts;i>0;i--){
          unsigned long begintime=millis();
          if(!command.equals("")){Serial1.println(command);}
          while((millis()-begintime)<timedelay){
            zikl();
            if(answer.indexOf(OK)>0){busy=false; return true;}
          }
        }
        busy=false;       
        return false;
    }
    void zikl(){
      if (D){Serial2.println("m1");}
      Mytask.runtask(); 
      if (D){Serial2.println("m2");}
      Processingcommand();
//      if (D){Serial2.println("m3");}
      Processingsim900(); 
//      if (D){Serial2.println("m4");}                    
      serialEvent();
      serialEvent1();
//      if (D){Serial2.println("m5");} 
      serialEvent2();
//      if (D){Serial2.println("m6");} 
      serialEvent3();
//      if (D){Serial2.println("m7");} 
    }
    boolean sendsms(String number,String message){
      String comm="AT+CMGS=";
      comm=comm+'"'+number+'"';
      if(!sendcomand(comm,">",1000,3)){return false;}
      comm=message + char(0x1A);
      if(!sendcomand(comm,"OK\r\n",1000,1)){return false;}
      return true;
    }
    boolean call(String number){
      String comm="ATD"+number+";";
      if(!sendcomand(comm,"OK\r\n",1000,1)){return false;}
      return true;
    }
    boolean connectIPserver(String adress, int port){
      if(!sendcomand("AT+CGREG=1","OK\r\n",5000,1)){return false;}
      if(!sendcomand("AT+CIPSHUT","OK\r\n",5000,1)){return false;}
      String comm="AT+CIPSTART="; 
      comm=comm+'"'; comm=comm+"TCP"; comm=comm+'"';comm=comm+',';
      comm=comm+'"'; comm=comm+adress; comm=comm+'"';comm=comm+',';
      comm=comm+'"'; comm=comm+String(port); comm=comm+'"';
      if(!sendcomand(comm,"CONNECT",5000,1)){return false;}
      return true;
    }
    boolean tcpsend(String data){
      String comm="AT+CIPSEND";
      if(!sendcomand(comm,">",1000,1)){return false;}
      comm=data + char(0x1A);
      if(!sendcomand(comm,"SEND OK\r\n",1000,1)){return false;}
      return true;
    }
    boolean ircconnect(String nick, String user){
      String comm="CAP LS\r\n";
      comm=comm + "NICK " + nick + "\r\n";
      comm=comm + "USER " + user + " 0 * :...\r\n";
      comm=comm + "PROTOCTL NAMESX\r\n";
      if(!answertcpsend(comm,user,5000)){return false;}
      return true;
    }
    boolean ircjoinchanel(){
      String comm="JOIN #" + ircchanel +" " + ircpassword+"\r\n";
      if(!answertcpsend(comm,ircchanel,5000)){return false;}
      return true;     
    }
    boolean answertcpsend (String message,String OK ,unsigned long timedelay){
      if(!tcpsend(message)){return false;}
      if(!sendcomand("",OK,timedelay,1)){return false;}
      return true;   
    }
    boolean waitsilent(unsigned long timesilent, unsigned long timewait){
      unsigned long beginwait=millis();
      while((millis()-beginwait)<timewait){
        if(!sendcomand("","\r\n",timesilent,1)){return true;}
      }
      return false;
    }
    void setircparam(String adress, int port, String nick, String user, String chanel, String password){
      ircadress=adress; 
      ircport=port; 
      ircnick=nick; 
      ircuser=user; 
      ircchanel=chanel; 
      ircpassword=password;
    }
    boolean ircstart(){
      if(!connectIPserver(ircadress, ircport)){return false;}
      waitsilent(2000,20000);
      if(!ircconnect(ircnick, ircuser)){;};
      waitsilent(2000,20000);
      if(!ircjoinchanel()){return false;};
      ircwork=true;
      return true;
    }
    boolean ircping(){
      Serial2.println("ircping");
//      if(!answertcpsend ("Proverka\r\n","SwiftIRC" ,2000)){
      ircsend("ping");
      if(answertcpsend ("PING " + ircadress + "\r\n","PONG" ,3000)){return true;}
      if(answertcpsend ("PING " + ircadress + "\r\n","PONG" ,3000)){return true;}
      return false;
    }
    boolean ircsend(String message){
      String comm="PRIVMSG #" + ircchanel + " :"+ message + "\r\n";
      if(!tcpsend(comm)){return false;}
      return true;
    }
    boolean sendfromard(String message){
      message=outgoingprefix+message;
      return ircsend(message);
    }
    boolean sendfromard(byte bytearray[], int len){
      String message=outgoingprefix;
      return ircsend(message);
    }
    void wait(unsigned long timedelay){
      boolean copybusy=busy;
      if (!busy){busy=true;}
        unsigned long begintime=millis();
        while((millis()-begintime)<timedelay){
            zikl();
        }
      if (!copybusy){busy=false;}     
    }
    
    boolean ircreconnect(){
      if (ircping()){
        Mytask.restarttask(0);
        Mytask.restarttask(1);
        return true;};
      if(ircstart()){return true;};
//      if (ircping()){return true;};
//      if(ircstart()){return true;};
//      poweroff();
//      poweron();
      return false;
    }    

  private:
    int _pin;
};

SIM900 SIM900(2);

class And
{
  public:
  String Andmessage="";
    And(int port){;};
    void println(String message){
      print(message + "\r\n");
      Serial2.print(Andmessage);
      if (flags&FLAG_ircenable){
        String Endmessage="\r\n";
        int ind=Andmessage.indexOf(Endmessage);                  
        while (ind>-1){                 
//          String getdata=Andmessage.substring(0, ind);           
          SIM900.ircsend(Andmessage.substring(0, ind));
          Andmessage=Andmessage.substring(ind+2); 
          ind=Andmessage.indexOf(Endmessage);
        }
      }
      Andmessage="";   
    }
    void print(String message){
      Andmessage=Andmessage+message;
    }
    void println(byte bytearray[], int len){
      Serial2.write(bytearray,len);
      if (flags&FLAG_ircenable){
        Andmessage="Ard:base64 " + base64encode(bytearray,len);
        SIM900.ircsend(Andmessage);
      }
      Andmessage="";   
    }
  private:

};


And And(2); 

void setup() { 
   analogReference(INTERNAL1V1);
   Wire.begin();  
   LCD.init();
   RTC.begin; 
   Loaddata();
               
   Serial.begin(serialspeed); //PC
   Serial1.begin(19200);      //SIM900                                               
   Serial3.begin(8192);       //ECU                                                     
   Serial2.begin(9600);        //Bluetooth   
   pinMode(Btpin, INPUT);                                                  
   pinMode(ledPin, OUTPUT); 
   pinMode(Pinstart, OUTPUT);                                         

   TV.clear_screen();
   TV.select_font(font4x6);
   if (!SD.begin(53)) {
     Serial2.println("initialization failed!");
     LCD.sendString("SD error",0,0);
     TV.println(0,4,"SD error");
   }
   else{
     Serial2.println("initialization done."); 
     LCD.sendString("SD ready",0,0); 
     TV.println(0,4,"SD ready");                                               
   }

    if(flags&FLAG_SIM900enable){
//     Runcommand("And:ircenable");
       if(SIM900.poweron()){
        LCD.sendString("SIM900 ready",0,1);
       }
       else{
        LCD.sendString("SIM900 not answer",0,1);
       }
    }
    else{
//      Runcommand("And:ircdisable");
      LCD.sendString("SIM900 disable",0,1);
    }
                             
   Volumebeforesrart=Volumelong;                  
   Distancebeforestart=Distancelong;
   Worktimeinginebeforestart=Worktimeinginelong;
   Cashbeforestart=Cashlong;  

//   Runcommand("And:printdata");
      
   SIM900.setircparam("irc.swiftirc.net", 6667, "Lemard", "Lemard", "Carlem", "260181");

   Mytask.add(0,"And:ircreconnect",30000,30000,-1);
   Mytask.add(1,"And:SIM900.poweroffon",130000,130000,-1);   
   Mytask.add(2,"And:oledrefresh",1000,500,-1);
   Mytask.add(3,"And:sendquery",5000,queryperiod,0);
//   Mytask.add(4,"And:lcdonoff",3000,3000,-1);
   Mytask.add(5,"And:btpush",3000,500,-1);Mytask.add(6,"And:TVrefresh",1000,500,-1);

   if(flags&FLAG_autoquery){
      Mytask.taskcount[3]=-1;
   }
   
   if(flags&FLAG_ircenable){
      LCD.sendString("IRC enable",10,0);
   }
   else{
      LCD.sendString("IRC disable",10,0);
      Runcommand("And:ircdisable");
   }    
   delay(2000);
   LCD.clearLcd();
   TV.clear_screen();
}

void loop() { 
  SIM900.zikl();
}

void Sim900power()                                                          
{
  digitalWrite(2, HIGH);
  delay(1500);
  digitalWrite(2, LOW);
                
}

void serialEvent(){      //PC
  while (Serial.available()) {
      byte inChar = Serial.read();
  if (flags&FLAG_bridgePCSIM900){Serial1.write(inChar);}    
  if (flags&FLAG_bridgePCECU){Serial3.write(inChar);}                                            
  }
}

void serialEvent1(){         //SIM900
  while (Serial1.available()) {    
      byte  inChar = Serial1.read();                                           
      if (flags&FLAG_bridgePCSIM900){Serial.write(inChar);} 
      if (inChar>0){messagesim900=messagesim900 + char(inChar); }
      if ((inChar>0)&SIM900.busy){SIM900.answer=SIM900.answer + char(inChar); }                            
  }
}

void serialEvent2(){           //Bluetooth 
  while (Serial2.available()) {
    byte  inChar = Serial2.read();
                                          
    if (inChar>0){incomingdata=incomingdata + char(inChar); }
    endbuffer2++;
    buffer2[endbuffer2]=inChar;
    getbluetoothpacket();
  }
}

void serialEvent3(){       //ECU
    while (Serial3.available()) {
      byte inChar=Serial3.read();
      if (flags&FLAG_bridgePCECU){Serial.write(inChar);}                                                   
                                                                                                                                  
      endbuffer3++;
      buffer3[endbuffer3]=inChar;
      getecupacket();
    }
}

void getecupacket(){
                                          
                                
  while (indpacket3!=endbuffer3){
    indpacket3++;
    if(leghtpacket3==0&buffer3[byte(indpacket3-1)]==244&buffer3[indpacket3]>85){
      beginpacket3=indpacket3-1;
      leghtpacket3=buffer3[indpacket3]-82;
      verifypacket3=244;                                                                                     
    }
    if(leghtpacket3!=0){
                                                             
      verifypacket3=verifypacket3+buffer3[indpacket3];  
                                                                                                  
                                                         
      if(byte(beginpacket3+leghtpacket3-1)==indpacket3){
        if(verifypacket3!=0){
          indpacket3=beginpacket3+1;
                                       
        }        
        if(verifypacket3==0){
                                         
          processpacket();
          indpacket3=beginpacket3+leghtpacket3-1;
        }
        leghtpacket3=0;
      }
    }
  }
}  
  
void getbluetoothpacket(){                           
  while (indpacket2!=endbuffer2){
    indpacket2++;                                             
    if(leghtpacket2==0&buffer2[byte(indpacket2-2)]==1&buffer2[byte(indpacket2-1)]==253){
      beginpacket2=indpacket2-2;
      leghtpacket2=buffer2[indpacket2];
      verifypacket2=254;     
    }
    if(leghtpacket2!=0){
      verifypacket2=verifypacket2+buffer2[indpacket2];  
      if(byte(beginpacket2+leghtpacket2-1)==indpacket2){
        if(verifypacket2!=0){
          indpacket2=beginpacket2+2;                                                 
        }        
        if(verifypacket2==0){
          processbluetoothpacket();
        }
        leghtpacket2=0;
      }
    }
  }
}   
  
void processpacket(){
     MPbegin(byte(1),byte(253));
     MPadd(byte(2));
                                                  
     for(byte ibyte=beginpacket3;ibyte!=byte(beginpacket3+leghtpacket3);ibyte++){
       MPadd(buffer3[ibyte]);                           
     }
     MPsend();
                                          
                                                                                 
  if(buffer3[byte(beginpacket3+2)]==1&buffer3[byte(beginpacket3+1)]==146){
    Mytask.taskstart[3]=millis() + querydelay;
    calcpacket();
//    if(Prm()==0){
//      datatooled(2);  
//    }
//    else{
      packetfind=true;
      datatooled(datatooledmode[getmode()]);
      datatotv(1);
      packetfind=false;
      Mytask.restarttask(2,1000);
//    }
    
  }
}

void processbluetoothpacket(){
  byte type=buffer2[byte(beginpacket2+3)];
  byte len=buffer2[byte(beginpacket2+2)];
  switch (type) {
    case 3:
      for(byte ibyte=byte(beginpacket2+4);ibyte!=byte(beginpacket2+leghtpacket2-1);ibyte++){
        Serial3.write(buffer2[ibyte]);
      } 
      break;
    case 11:
      Tankfuel=0;
      type=10;
    case 10:
      Fuelevent();
      break;
    case 6:
      unsigned long Syncdata;
      Syncdata=bytearraytolong(buffer2,beginpacket2+4);
      RTC.adjust(DateTime(Syncdata));
      type=7;
    case 7:
      DateTime now0=RTC.now();
      unsigned long Currenttime=now0.unixtime();
      MPbegin(1,253);
      MPadd(byte(7));
      MPadd(Currenttime);   
      MPsend();
      break;         
  }
}

unsigned long bytearraytolong(byte bytearray[], byte beginlong){
      unsigned long otvet;
      for(byte ibyte=byte(beginlong+3);ibyte!=byte(beginlong-1);ibyte--){
        otvet=otvet << 8;
        otvet=otvet+bytearray[ibyte];
      }  
      return otvet;
}

String Stringalling(String st,int alling, int len){
  st.trim();
  if (st.length()>len){
    st.remove(len);
  }
  else{
    for(int i=st.length();i<len;i++){
      if(alling){st=" " + st;}else{st=st + " ";}
    }
  }
  return st;
}

String Stringalling(double d,int alling, int len){
  return Stringalling(String(d),alling,len);
}

void datatotv(int mode){
  DateTime now0=RTC.now();
  DateTime now1 (now0.unixtime() + 18000);  
  char Stringtochar2[21];
  String stroka1; 
  String stroka2;

  int curmode=getmode();
  if (curmode<2&savemode>1){TV.clear_screen();}
  savemode=curmode;

  switch (mode) {
    case 0:
      stroka1=String(dayweek[now1.dayOfWeek()]) + " "
        + String(now1.year()) + "/"
        + String(now1.month()) + "/"
        + String(now1.day()) + " " 
        + String(now1.hour()) + ":"
        + String(now1.minute()) + ":"
        + String(now1.second()) + "";
      stroka1=Stringalling(stroka1,0,30);   
      TV.select_font(font4x6);                          
      TV.println(0,4,stroka1.c_str());

      stroka1="Tank " + String(Tankfuel/1000.0) + " L  " + String(TankDist()) + " km";
      stroka1=Stringalling(stroka1,0,30); 
      TV.println(stroka1.c_str()); 
                
    break;
    case 1:
      stroka1 = "TEMP AIR/WATER " + String(TAir(),0) + "/" + String(TWat(),0) + " C";
      stroka1=Stringalling(stroka1,0,30); 
      TV.select_font(font4x6);
      TV.println(0,16,stroka1.c_str()); 

      stroka1=Stringalling(String(Speed(),0),1,3);
      TV.select_font(font8x8);
      TV.print(76,33,stroka1.c_str());
      TV.select_font(font4x6);
      TV.print(100,36," km/h");

      stroka1=Stringalling(String(Prm(),0),1,4);
      TV.select_font(font8x8);
      TV.print(68,43,stroka1.c_str());
      TV.select_font(font4x6);
      TV.print(100,46," prm");      
//      datenow=String(Speed(),0) + " km/h";
//      TV.println(60,40,datenow.c_str()); 

      if (L100()>0){
        stroka1=Stringalling(String(L100(),2),1,4); 
        stroka2=" L/km";        
      }
      else{
        stroka1=Stringalling(String(Lh(),2),1,4); 
        stroka2=" L/h ";         
      }
      TV.select_font(font8x8);
      TV.print(68,53,stroka1.c_str());
      TV.select_font(font4x6);
      TV.print(100,56,stroka2.c_str());

      TV.print(0,26,"TRAVEL");
      TV.print(68,26,"CURRENT");

      
      stroka1= String(DistTr(),1) + " km";
      stroka1=Stringalling(stroka1,0,11);
      TV.print(0,34,stroka1.c_str());
      
      stroka1= String(VolTr(),2) + " L";
      stroka1=Stringalling(stroka1,0,11);
      TV.print(0,40,stroka1.c_str());     

      stroka1= String(CashTr(),2) + " RUB";
      stroka1=Stringalling(stroka1,0,11);
      TV.print(0,46,stroka1.c_str());

      stroka1= String(LhAvrTr(),2) + " L/h";
      stroka1=Stringalling(stroka1,0,11);
      TV.print(0,52,stroka1.c_str());   

      stroka1= String(L100AvrTr(),2) + " L/km";
      stroka1=Stringalling(stroka1,0,15);
      TV.print(0,58,stroka1.c_str());        

      stroka1= String(gear(),0);// + " prmkm/h";
      stroka1=Stringalling(stroka1,0,4);
      TV.print(44,34,stroka1.c_str());        

 //     TV.print(120,56,"ffff");

      
      
    break;
    default:
    break;
  }
}


void datatooled(int mode){
  int prm;
  double l100;
  int speed;
  String datenow;
//  String timenow;
  
  DateTime now0=RTC.now();
  DateTime now1 (now0.unixtime() + 18000);
  char Stringtochar2[20];  
  
  if (mode>0){
//    Serial.println("lcdon-");
    LCD.lcdOn();
//    Serial.println("-lcdon");
  } 
  
  switch (mode) {
    case 0:
            LCD.lcdOff();
             break;      
    case 1:
            datenow="   " + String(dayweek[now1.dayOfWeek()]) + "  "
                              + String(now1.year()) + "/"
                              + String(now1.month()) + "/"
                              + String(now1.day()) + "       "; 
            datenow.toCharArray(Stringtochar2, datenow.length()+1);
            LCD.sendString(Stringtochar2,0,0);   
                                       
            datenow="       " + String(now1.hour()) + ":"
                              + String(now1.minute()) + ":"
                              + String(now1.second()) + "         ";
             datenow.toCharArray(Stringtochar2, datenow.length()+1);
             LCD.sendString(Stringtochar2,0,1);
             
//             LCDsenddouble(double(now1.dayOfWeek()),1,0,1,0);
//             LCD.sendString(dayweek[now1.dayOfWeek()],0,0);

             break;    
    case 2:
           LCD.sendString("AIR ",0,0); 
           LCDsenddouble(TAir(),4,0,3,0);
           LCD.sendString(" FUEL ",7,0); 
           LCDsenddouble(Tankfuel/1000.0,4,1,13,0);
           LCD.sendString(" L ",17,0); 
           LCD.sendString("WAT ",0,1); 
           LCDsenddouble(TWat(),4,0,3,1);           
           LCD.sendString(" DIST ",7,1);
           LCDsenddouble(TankDist(),4,1,13,1);
           LCD.sendString(" km",17,1);           
            break;
    case 3:
        LCD.sendString(" ",3,0);
        LCD.sendString(" ",8,0);
        LCD.sendString(" ",13,0); 
        LCD.sendString(" ",18,0);
        LCD.sendString(" ",19,0);  
        LCD.sendString(" ",3,1);
        LCD.sendString(" ",8,1);
        LCD.sendString(" ",13,1); 
        LCD.sendString(" ",18,1);
        LCD.sendString(" ",19,1); 
        LCDsenddouble(Prm(),4,0,9,0);
        LCDsenddouble(VolTr(),4,2,4,0);
        LCDsenddouble(CashTr(),4,2,4,1);
        LCDsenddouble(TWat(),3,0,0,1);  
        LCDsenddouble(TAir(),3,0,0,0);
        LCDsenddouble(L100(),4,2,14,0);//удельный расход мгновенный
        LCDsenddouble(L100AvrTr(),4,2,14,1);//удельный расход средний
        LCDsenddouble(DistTr(),4,2,9,1);
        if (countmeasure==20){
      //    LCD.clearLcd();
          LCD.sendString("AIR ",0,0);
          LCD.sendString("WAT ",0,1);
          LCD.sendString("LITR ",4,0);
          LCD.sendString("RUB  ",4,1);
          LCD.sendString("PRM  ",9,0);
          LCD.sendString("Km   ",9,1);
          LCD.sendString("L/Km ",14,0);
          LCD.sendString("L/Km ",14,1);    
          countmeasure=0;
        }
        countmeasure++;

      break;
    case 4:
           LCD.sendString("Reserve fuel ",0,0); 
           LCDsenddouble(Tankfuel/1000.0,4,1,13,0);
           LCD.sendString(" L ",17,0); 
           LCD.sendString("    Distance ",0,1);
           LCDsenddouble(TankDist(),4,1,13,1);
           LCD.sendString(" km",17,1);
           
           
            break;    
    case 5:
           LCD.sendString("L/h   ",0,0); 
           LCDsenddouble(Lh(),5,2,5,0);
           LCDsenddouble(LhAvrTr(),5,2,10,0);
           LCDsenddouble(LhAvr(),5,2,15,0);
           LCD.sendString("L/km  ",0,1); 
           LCD.sendString(" ",5,1);
           LCD.sendString(" ",10,1);
           LCD.sendString(" ",15,1);
           LCDsenddouble(L100(),4,2,6,1);
           LCDsenddouble(L100AvrTr(),4,2,11,1);
           LCDsenddouble(L100Avr(),4,2,16,1);           
            break; 
    case 6:
           LCDsenddouble(Prm(),4,0,0,0);
           LCD.sendString("prm ",4,0); 
           LCDsenddouble(Speed(),3,0,8,0);
           LCD.sendString("kh ",11,0);
           if (L100()>0){
           LCDsenddouble(L100(),4,2,14,0); 
           LCD.sendString("Lk",18,0);           
           }
           else{
           LCDsenddouble(Lh(),4,2,14,0); 
           LCD.sendString("Lh",18,0);            
           }
 

           LCDsenddouble(CashTr(),4,2,0,1);
           LCD.sendString("rub ",4,1); 
           LCDsenddouble(VolTr(),4,2,8,1);
           LCD.sendString("L ",12,1); 
           if (L100AvrTr()>0){
           LCDsenddouble(L100AvrTr(),4,2,14,1); 
           LCD.sendString("Lk",18,1);           
           }
           else{
           LCDsenddouble(LhAvrTr(),4,2,14,1); 
           LCD.sendString("Lh",18,1);            
           }                                                  
            break;
    case 7:
            LCD.sendString("Sensor Fuel=",0,0);
            LCDsenddouble(SFuel(),4,0,12,0);
            LCD.sendString("                    ",0,1);
            break;                                                 
    default:
//            LCD.sendString("       MODE         ",0,0);
//            LCD.sendString("                    ",0,1);
//            LCDsenddouble(double(mode),0,0,13,0);
            ;
  }
}

void calcpacket(){
    moment1=millis();                        
    interval=moment1-moment;                                                             
    flow=0;
    flow=buffer3[byte(beginpacket3+39)];
    flow=flow*256.0;
    flow=flow+buffer3[byte(beginpacket3+40)];
    flow=flow/131.07;                                                               
    flow=flow*flowinjector;
    flow=flow/60000000;                                                                     
    flow=flow*buffer3[byte(beginpacket3+15)]*1500;                              
    flow=flow*4;     // расход мл/мсек                               
    if(interval<2000){                                          
        Volume=Volume+flow*interval/3600;                                   
        Cash=Cash+flow*interval*Pricefuel/36000;         
        
    if(flow<0){                              
        for(byte ibyte=beginpacket3;ibyte!=byte(beginpacket3+leghtpacket3);ibyte++){          
        }                   
    }
        Distance=Distance+buffer3[byte(beginpacket3+17)]*(double)interval/60/60;                         
        if(buffer3[byte(beginpacket3+15)]>0){
          Worktimeingine=Worktimeingine+interval;                                         
        };
      };
      Refreshlongdata();
      Startevent();
      Stopevent();
      lastprm=buffer3[byte(beginpacket3+15)];                               
      moment = moment1;
      Sendmydata();      
      Blinkled();                        
    if ((Volumelong-Volumesave)>=1000){Savedata();};                                     
}

void Sendmydata() {
  unsigned int Flowintmlh=int(flow*1000+0.5);
  MPbegin(1,253);
  MPadd(byte(1));
  MPadd(moment1);                  
  MPadd(Volumelong);              
  MPadd(Distancelong);            
  MPadd(Worktimeinginelong);      
  MPadd(Cashlong);                
  MPadd(Countstartengine);         
  MPadd(Volumelong-Volumebeforesrart);
  MPadd(Distancelong-Distancebeforestart);     
  MPadd(Worktimeinginelong-Worktimeinginebeforestart);
                       
                                                
                                                              
                                                                
  MPadd(Cashlong-Cashbeforestart);
  MPadd(Tankfuel);                 
  MPadd(Flowintmlh);             
                   
                     
  MPsend();
}

  void MPbegin(byte MPbyte){
      MPdata[0]=MPbyte;
      MPleightbyte=1;
      MPleight=2;    
  }
  void MPbegin(byte MPbyte, byte MPbyte1){
      MPdata[0]=MPbyte;
      MPdata[1]=MPbyte1;
      MPleightbyte=2;
      MPleight=3;    
  }
  void MPadd(unsigned int MPint){
      unsigned long MPlong;
      MPlong=MPint;
      MPaddall(MPlong,2);
  };
  void MPadd(double MPdouble){
      unsigned long MPlong;
      MPlong=MPdouble;
      MPaddall(MPlong,4);
  };
  void MPadd(byte MPbyte){
      unsigned long MPlong;
      MPlong=MPbyte;
      MPaddall(MPlong,1);
  };
  void MPadd(unsigned long MPlong){
      MPaddall(MPlong,4);
  };  
  void MPadd(String MPString){
      for(int ibyte=0;ibyte<MPString.length();ibyte++){
        MPdata[MPleight]=(byte)(MPString.charAt(ibyte));
                        
        MPleight++;
      };    
  };
  void MPaddall(unsigned long MPlong,byte MPcountbit){
      for(byte ibyte=0;ibyte<MPcountbit;ibyte++){
        MPdata[MPleight]=(byte)((MPlong >> (ibyte*8)) & 0xff);
        MPleight++;
      };    
  };
  void MPsend(){
    byte MPsum=0;
    MPdata[MPleightbyte]=MPleight+1;
    for(byte ibyte=0;ibyte<(MPleight);ibyte++){
//        Serial2.write(MPdata[ibyte]);
        MPsum=MPsum-MPdata[ibyte];
    }; 
    MPdata[MPleight]=MPsum;
    MPleight++;
    Serial2.write(MPdata,MPleight);
  };

  void MPsend(byte bytearray[],int start, int len){ 
    for(int ibyte=0;ibyte<len;ibyte++){
      MPdata[ibyte]=bytearray[start+ibyte];                  
    };   
    Serial2.write(MPdata,len);
  }

  void MPwrite(String filename){
    char Stringtochar[filename.length()+ 1];
    filename.toCharArray(Stringtochar, filename.length()+1);
    myFile = SD.open(Stringtochar, FILE_WRITE);
      if (myFile) {
        Serial2.println("");
        Serial2.println("opened " + filename);                   
        myFile.write(MPdata,MPleight);                    
        myFile.close();
      } else {  
        Serial2.println("");
        Serial2.println("error open " + filename);                                        
      }  
  };

char* Stringtochar111(String St){
    Serial2.println("St="+St);
    char Stringtochar1[St.length()+ 1];
    St.toCharArray(Stringtochar1, St.length()+1);
    Serial2.println("Stringtochar1="+String(Stringtochar1));
    return Stringtochar1;
}

void Resetdata() {
  Volume=0;
  Distance=0;
  Worktimeingine=0;
  Volumelong=0;
  Tankfuel=0;
  Distancelong=0;
  Worktimeinginelong=0;  
  Volumebeforesrart=0;
  Distancebeforestart=0;
  Worktimeinginebeforestart=0;
  Countstartengine=0; 
  Eventcount=0;  
  Cash=0;
  Cashlong=0;
  Cashbeforestart;
                                
  Pricefuel=30;        
  EEPROM_write(22, Pricefuel); 
  EEPROM_write(26, flowinjector);  
  Savedata();
  And.println("Ard:Data is reset");  
                

}  

void Loaddata() {
  EEPROM_read(0, Volumelong);        
  EEPROM_read(4, Distancelong);
  EEPROM_read(8, Worktimeinginelong); 
  EEPROM_read(12, Countstartengine);
  EEPROM_read(14, Eventcount);
  EEPROM_read(18, Tankfuel);                                              
  EEPROM_read(20, Cashlong); 
  EEPROM_read(24, Pricefuel);
  EEPROM_read(28, flowinjector);  
  EEPROM_read(32, flags);
  EEPROM_read(33, serialspeed);
  EEPROM_read(35, queryperiod);
  EEPROM_read(37, querydelay);  
  if (flowinjector==0) flowinjector=219;    
  if (serialspeed==0) serialspeed=9600;                            
  Volumesave=Volumelong;
                         
}

void Savedata() {
                         
    EEPROM_write(0, Volumelong);
    EEPROM_write(4, Distancelong); 
    EEPROM_write(8, Worktimeinginelong); 
    EEPROM_write(12, Countstartengine);
    EEPROM_write(14, Eventcount);
    EEPROM_write(18, Tankfuel); 
    EEPROM_write(20, Cashlong); 
    EEPROM_write(24, Pricefuel); 
    EEPROM_write(28, flowinjector);
    EEPROM_write(32, flags);
//    serialspeed=9600;
    EEPROM_write(33, serialspeed);
    EEPROM_write(35, queryperiod);
    EEPROM_write(37, querydelay);
//    Sendsavedata();
}

String sectotime(unsigned long seccount){
  unsigned int hourcount=int(seccount/3600.0);
  unsigned int minutcount=int((seccount-hourcount*3600)/60.0);
  unsigned int secondcount=int(seccount-hourcount*3600-minutcount*60);
  return String(hourcount) + ":" + String(minutcount) + ":" + String(secondcount);
}

void Senddata() {
  And.println("Ard:data Volumelong=" + String(Volumelong/1000.0)+" L"); 
  And.println("Ard:data Distancelong=" + String(Distancelong/1000.0)+" km");  
  And.println("Ard:data Worktimeinginelong=" + sectotime(Worktimeinginelong)); 
  And.println("Ard:data Countstartengine=" + String(Countstartengine) + " runs"); 
  And.println("Ard:data Eventcount=" + String(Eventcount)+" event"); 
  And.println("Ard:data Tankfuel=" + String(Tankfuel/1000.0)+" L"); 
  And.println("Ard:data Cashlong=" + String(Cashlong/100.0)+" RUB"); 
  And.println("Ard:data Pricefuel=" + String(Pricefuel,2)+" RUB/L");
  And.println("Ard:data flowinjector=" + String(flowinjector,2) + " cc/s"); 
  And.println("Ard:data Volumelongtravel=" + String((Volumelong-Volumebeforesrart)/1000.0)+" L"); 
  And.println("Ard:data Distancelongtravel=" + String((Distancelong-Distancebeforestart)/1000.0)+" km");  
  And.println("Ard:data Worktimeinginelongtravel=" + sectotime(Worktimeinginelong-Worktimeinginebeforestart)); 
  And.println("Ard:data Cashlongtravel=" + String((Cashlong-Cashbeforestart)/100.0)+" RUB");   
}

void Sendsavedata() {
  byte Bytevalue;
  unsigned long Longvalue;
  unsigned int Intvalue;
  double Doublevalue;
  EEPROM_read(0, Longvalue); 
  And.println("Ard:savedata Volumelong=" + String(Longvalue/1000.0)+" L"); 
  EEPROM_read(4, Longvalue);
  And.println("Ard:savedata Distancelong=" + String(Longvalue/1000.0)+" km"); 
  EEPROM_read(8, Longvalue); 
  And.println("Ard:savedata Worktimeinginelong=" + sectotime(Longvalue)); 
  EEPROM_read(12, Intvalue);
  And.println("Ard:savedata Countstartengine=" + String(Intvalue) + " runs"); 
  EEPROM_read(14, Intvalue);
  And.println("Ard:savedata Eventcount=" + String(Intvalue)+" event");
  EEPROM_read(18, Intvalue);  
  And.println("Ard:savedata Tankfuel=" + String(Intvalue/1000.0)+" L"); 
  EEPROM_read(20, Longvalue); 
  And.println("Ard:savedata Cashlong=" + String(Longvalue/100.0)+" RUB"); 
  EEPROM_read(24, Doublevalue); 
//  Serial2.print("Ard:savedata Pricefuel=");
  And.println("Ard:savedata Pricefuel=" + String(Doublevalue,2)+" RUB/L");
  EEPROM_read(28, Doublevalue); 
//  Serial2.print("Ard:savedata flowinjector=");
  And.println("Ard:savedata flowinjector=" + String(Doublevalue,2) + " cc/s");  
//  Volumesave=Volumelong;
  EEPROM_read(32, Bytevalue); 
  And.println("Ard:savedata flags=" + String(Bytevalue) + " B" + String(Bytevalue, BIN));
  if (flags&FLAG_autoquery){And.println("Ard:savedata FLAG_autoquery = true");}
  if (flags&FLAG_SIM900enable){And.println("Ard:savedata FLAG_SIM900enable = true");}
  if (flags&FLAG_ircenable){And.println("Ard:savedata FLAG_ircenable = true");}
  if (flags&FLAG_bridgePCECU){And.println("Ard:savedata FLAG_bridgePCECU = true");}
  if (flags&FLAG_bridgePCSIM900){And.println("Ard:savedata FLAG_bridgePCSIM900 = true");}  
  
  EEPROM_read(33, Intvalue);
  And.println("Ard:savedata Serialspeed=" + String(Intvalue));
  EEPROM_read(35, Intvalue);
  And.println("Ard:savedata queryperiod=" + String(Intvalue) + "ms");  
  EEPROM_read(37, Intvalue);
  And.println("Ard:savedata querydelay=" + String(Intvalue) + "ms");                         
}


void resetbluetooth() {
    digitalWrite(ResetPin, false);
    delay(200);
    digitalWrite(ResetPin, true);
                                  
}

void Blinkled() {
  if(led==false){
    led=true;}
  else {
    led=false;
  }
  digitalWrite(ledPin, led);
}

void SAVELOG(String Typedata, byte data) {
                      
  logFile = SD.open("LEMLOG.TXT", FILE_WRITE);
  if (!(Typedata.equals(saveTypedata))){
    logFile.println("");
    logFile.print(Typedata);
    saveTypedata=Typedata;
  }
  logFile.write(data);
  logFile.close();
}

void SAVELOG1(String dataString) {
                    
  File logFile = SD.open("carlemlog", FILE_WRITE);
                                                               
                    
                                           
  if (logFile) {
    logFile.println(dataString);
                                                                
                      
    logFile.close();
                                                              
                                    
                                  
  }  
                                             
  else {
    Serial2.println("error opening carlemlog.txt");
  }
}

void readlog(){           
  File dataFile = SD.open("LEMLOG.TXT");                                     
  if (dataFile) {
    while (dataFile.available()) {
      Serial2.write(dataFile.read());
    }
    dataFile.close();
  }                                        
  else {
    Serial2.println("error opening LEMLOG.txt");
  }                                               
}

void Processingcommand() {
  if (incomingdata.length()>256) {incomingdata=incomingdata.substring(64);}
  String Endmessage="\r";
  Endmessage=Endmessage + "\n";
  int ind=incomingdata.indexOf(Endmessage);                  
  while (ind>-1){
//    if (1>0){Serial.println("Processingcommand");};
//    if (1>0){Serial.println("incomingdata="+incomingdata);};
    String getdata=incomingdata.substring(0, ind);
    incomingdata=incomingdata.substring(ind+2);                           
    Runcommand(getdata);
    ind=incomingdata.indexOf(Endmessage);
  }
}

void Processingsim900() {                                                                                                                   
  String Endmessage="\r";
  Endmessage=Endmessage + "\n";
  int ind=messagesim900.indexOf(Endmessage);                  
  while (ind>-1){
//    Serial2.println("ind="+String(ind));
//    Serial2.println("messagesim900="+messagesim900);
//    Serial2.println("lenmessagesim900="+String(messagesim900.length()));
    String getdata=messagesim900.substring(0, ind);
    messagesim900=messagesim900.substring(ind+2);                               
    MPbegin(1,253);
    MPadd(byte(4));
    MPadd(getdata);
    MPadd(byte(10));
    MPadd(byte(13));
    MPsend();                               
    Runsim900command(getdata);
    ind=messagesim900.indexOf(Endmessage);
  }
}

String getdatatime(){
  DateTime now0=RTC.now();
  return (String(now0.year()) + "/"
                    + String(now0.month()) + "/"
                    + String(now0.day()) + ","
                    + String(now0.hour()) + ":"
                    + String(now0.minute()) + ":"
                    + String(now0.second()));
}

void GPRSon(){
  Serial1.println("AT+SAPBR=3,1," + String(char(34)) + "CONTYPE" + String(char(34)) + "," + String(char(34)) + "GPRS" + String(char(34)));
  Serial1.println("AT+SAPBR=3,1," + String(char(34)) + "APN" + String(char(34)) + "," + String(char(34)) + "inet.ycc.ru" + String(char(34)));
  Serial1.println("AT+SAPBR=3,1," + String(char(34)) + "USER" + String(char(34)) + "," + String(char(34)) + "motiv" + String(char(34)));
  Serial1.println("AT+SAPBR=3,1," + String(char(34)) + "PWD" + String(char(34)) + "," + String(char(34)) + "motiv" + String(char(34)));
  Serial1.println("AT+SAPBR=1,1");
}

void Runsim900command(String query) { 
//  SIM900.on=true;
  int iof=query.indexOf(SIM900.incomingprefix);
  if (iof>-1){
    query=query.substring(iof);
    Runcommand(query);
  }
  if (query.equals("RDY")){        
    SIM900.on=true;                    
  }
  if (query.equals("NORMAL POWER DOWN")){
    SIM900.on=false;
  }
  if (query.startsWith("PING")){
    if (flags&FLAG_ircenable){
      SIM900.tcpsend("PONG"+query.substring(4)+"\r\n");
      Mytask.restarttask(0);
      Mytask.restarttask(1);
    }
  } 
//  if ((query.indexOf("+CGREG: 1")>-1)||(query.indexOf("CLOSED")>-1)){
//    if(SIM900.ircstart("irc.swiftirc.net", 6667, "Lemard", "Lemard", "Carlem", "260181")){Serial2.println("Irc is start");}else{Serial2.println("Irc not start");} ;
//    if(ircenable)Mytask.restarttask(0,5000);
//  } 
  
  if (query.startsWith("CONNECT FAIL")){
    SIM900.poweroff();
    SIM900.poweron();
  } 
  
}

void(* resetFunc) (void) = 0;

void Runcommand(String query) {
  do{
    if (query.startsWith("And:")){
      if (query=="And:sendquery"){
        Serial3.write(244); Serial3.write(87); Serial3.write(1); Serial3.write(0); Serial3.write(180);
        break;
      }
      if (query=="And:TVrefresh"){
        datatotv(0);
        break;
      }             
      if (query=="And:oledrefresh"){
        datatooled(datatooledmode[getmode()]);
        break;
      } 
      if (query=="And:btpush"){
        boolean Btstate=digitalRead(Btpin);
        if(Btstate){
          int mod=getmode();
          Serial2.println("Button push");
          datatooledmode[mod]++;
          if (datatooledmode[mod]>10){
            datatooledmode[mod]=0;
          }          
          LCD.sendString("       MODE         ",0,0);
//          LCD.sendString("                    ",0,1);
          LCDsenddouble(datatooledmode[mod],0,0,13,0);
            switch (datatooledmode[mod]) {
              case 0:
                LCD.sendString("     Screen off     ",0,1);
                break;              
              case 1:
                LCD.sendString("     Date, Time     ",0,1);
                break;
              case 2:
                LCD.sendString("    Before start    ",0,1);
                break;                
              case 3:
                LCD.sendString("     Favorite1      ",0,1);
                break;
              case 4:
                LCD.sendString("       Reserve      ",0,1);
                break;     
              case 5:
                LCD.sendString("         Flow       ",0,1);
                break;  
              case 6:
                LCD.sendString("     Favorite2      ",0,1);
                break;   
              case 7:
                LCD.sendString("   Accumulation     ",0,1);
                break;    
              case 8:
                LCD.sendString(" Accumulation travel",0,1);
                break; 
              case 9:
                LCD.sendString("      Average       ",0,1);
                break;  
              case 10:
                LCD.sendString("   Average travel   ",0,1);
                break;                                                                                                                 
            }
          
        }
//        Btpush=Btstate;
        break;
      }      
      if (query=="And:lcdonoff"){
        
        if (analogRead(ACCPin)>70){
          LCD.lcdOn();  
        }
        else{
          LCD.lcdOff();  
        }
        break;
      }           
      if (query=="And:ircping"){
        if (flags&FLAG_ircenable)SIM900.ircping();
        break;
      }
      if (query=="And:ircreconnect"){
        if (flags&FLAG_ircenable) SIM900.ircreconnect();
        break;
      }   
      if (query.startsWith("And:sendfile ")){
        Serial2.println(query.substring(13));
        sendfile(query.substring(13));
        break;
      }  
      if (query=="And:sendsavedata"){
        Sendsavedata();
        break;
      }
      if (query=="And:senddata"){
        Senddata();
        break;
      }
      if (query=="And:savedata"){
        Savedata();
        break;
      } 
      if (query.startsWith("And:deletefile ")){
     //   Serial2.println(query.substring(15));
        deletefile(query.substring(15));
        break;
      }
      if (query=="And:sendfile"){
        sendfile();
        break;
      }  
      if (query=="And:list"){
          And.println("Ard:list ");
          root = SD.open("/");
          printDirectory(root, 0); 
          And.println("");
          break;
      } 
      if (query=="And:readlog"){
        readlog();
        break;
      }
      if (query=="And:resetdata"){
        Resetdata();
        break;
      } 
      if (query=="And:SIM900.power"){
        SIM900.power();
        break;
      }
      if (query=="And:SIM900.poweron"){
        SIM900.poweron();
        break;
      }  
      if (query=="And:SIM900.poweroff"){
        SIM900.poweroff();
        break;
      }  
      if (query=="And:SIM900.poweroffon"){
        Serial2.println("poweroffon");
        SIM900.power();
        SIM900.wait(4000);
        SIM900.poweron();
        Mytask.restarttask(0);
        Mytask.restarttask(1);
        break;
      }   
      if (query=="And:autoqueryon"){
//        flagautoquery=true;
        flags=flags|FLAG_autoquery;
        Mytask.taskcount[3]=-1;
        Savedata();
        And.println("autoqueryon");
        break;
      }  
      if (query=="And:autoqueryoff"){
//        flagautoquery=false;
        flags=flags&(FLAG_autoquery^B11111111);
        Mytask.taskcount[3]=0;
        Savedata();
        And.println("autoqueryoff");
        break;
      } 
      if (query=="And:ircenable"){
//        ircenable=true;
        flags=flags|FLAG_ircenable;
        And.println("ircenable");
        Runcommand("And:SIM900enable");
        Savedata();
        Mytask.taskcount[0]=-1;
        Mytask.taskcount[1]=-1;
        break;
      }       
      if (query=="And:ircdisable"){
//        ircenable=false;
        flags=flags&(FLAG_ircenable^B11111111);
        Savedata();
        Mytask.taskcount[0]=0;
        Mytask.taskcount[1]=0;
        And.println("ircdisable");
        break;
      }
      if (query=="And:SIM900enable"){
        flags=flags|FLAG_SIM900enable;
        SIM900.poweron();
        Savedata();
        And.println("SIM900enable");
        break;
      }  
      if (query=="And:SIM900disable"){
        flags=flags&(FLAG_SIM900enable^B11111111);
        SIM900.poweroff();
        Savedata();
        And.println("SIM900disable");
        break;
      }     
      if (query=="And:bridgePCECUenable"){
//        bridgePCECU=true;
        flags=flags|FLAG_bridgePCECU;
        Savedata();
        And.println("bridgePCECUenable");
        break;
      }  
      if (query=="And:bridgePCECUdisable"){
//        bridgePCECU=false;
        flags=flags&(FLAG_bridgePCECU^B11111111);
        Savedata();
        And.println("bridgePCECUdisable");
        break;
      } 
      if (query=="And:bridgePCSIM900enable"){
//        bridgePCSIM900=true;
        flags=flags|FLAG_bridgePCSIM900;
        Savedata();
        And.println("bridgePCSIM900enable");
        break;
      }  
      if (query=="And:bridgePCSIM900disable"){
//        bridgePCSIM900=false;
        flags=flags&(FLAG_bridgePCSIM900^B11111111);
        Savedata();
        And.println("bridgePCSIM900disable");
        break;
      }                           
      if (query=="And:proba"){
    //    delay(1000);
    //    Serial2.println("proba run");
        sendfile();
        break;
      }  
      if (query.startsWith("And:send;")){
        Senddata(query.substring(9));
        break;
      }  
      if (query.startsWith("And:SIM900;")){
        Serial1.println(query.substring(11));
        break;
      }  
      if (query.startsWith("And:nextbyte;")){                                          
          byteinpacket++;
          Serial2.println("byte="+String(byteinpacket));
          break;
      }  
      if (query.startsWith("And:tank;")){
        Tank(query);
        break;
      }
      if (query.startsWith("And:periodautoquery;")){
        Serial2.println("query="+query);
        Serial2.println("split="+(split(query ,";",3)));                                                 
        periodautoquery = stringtoint(split(query ,";",3));
        break;
      }    
       if (query=="And:debugon"){
        D=true;
        break;
      } 
      if (query=="And:debugoff"){
        D=false;
        break;
      } 
      if (query.startsWith("And:base64 ")){
        endbuffer2=base64decode(query.substring(11),buffer2);
        indpacket3=0;
        Serial2.println("base64=" + bytetostring(buffer2,endbuffer2));
        getbluetoothpacket();
        break;
      }
      if (query=="And:proverka"){
        break;
      } 
      if (query=="And:getdatatime"){
          And.println(getdatatime());
          break;
      } 
      if (query=="And:gprson"){
          GPRSon();
          break;
      }
      if (query=="And:reset"){
          Savedata();
          resetFunc();
          break;
      }
      if (query.startsWith("And:serialspeed ")){
        And.println(query.substring(16));
        String serspeed=query.substring(16);
        char charint[6];
        serspeed.toCharArray(charint,6);
        serialspeed=atoi(charint);
//        And.println(String(val));
        Serial.println("serialspeed=" + String (serialspeed));
        And.println("seri=" + String (serialspeed));
        Savedata();
        
        resetFunc();
        break;
      }
      if (query.startsWith("And:queryperiod ")){
        String textqueryperiod=query.substring(16);
        char charint[6];
        textqueryperiod.toCharArray(charint,6);
        queryperiod=atoi(charint);
//        And.println(String(val));
//        Serial.println("queryperiod=" + String (serialspeed));
        And.println("queryperiod=" + String (queryperiod));
        Mytask.taskperiod[3]=queryperiod;
        Savedata();
        break;
      }
      if (query.startsWith("And:querydelay ")){
        String textquerydelay=query.substring(15);
        char charint[6];
        textquerydelay.toCharArray(charint,6);
        querydelay=atoi(charint);
//        And.println(String(val));
//        Serial.println("queryperiod=" + String (serialspeed));
        And.println("querydelay=" + String (querydelay));
        Savedata();
        break;
      }      
      if (query=="And:startengine"){
          digitalWrite(Pinstart, true);
          delay(500);
          digitalWrite(Pinstart, false);
          And.println("startengine");
          break;
      }      
      if (query=="And:lcdon"){
        LCD.lcdOn();
        break;
      }
      if (query=="And:lcdoff"){
        LCD.lcdOff();
        break;
      }     
      if (query=="And:lcdonoffmode"){
        if (Mytask.taskcount[4]==0){
          Mytask.taskcount[4]=-1;
          Serial2.print ("Ard:lcdonoffmode enable");
        }
        else {
          Mytask.taskcount[4]=0;
          Serial2.print ("Ard:lcdonoffmode disable");
        }
        break;
      }         
    }
  }while(0);
  if (query.startsWith("AT")||query.startsWith("at")){
    Serial1.println(query);
  } 
}

void printDirectory(File dir, int numTabs) {
  dir.rewindDirectory();
   while(true){
     File entry =  dir.openNextFile();
     if (! entry) {                                  
       break;
     }
       And.print("Ard:list ");
     for (uint8_t i=0; i<numTabs; i++) {
       And.print(" ");
     }
//     Serial2.println(entry.name());
     And.print(entry.name());
     if (entry.isDirectory()) {
       And.print("/");
       printDirectory(entry, numTabs+1);
     } else {                                              
       And.print("  ");
       And.println(String(entry.size(), DEC));
//       And.print("\r\n");
     }
     entry.close();
   }
}

int stringtoint(String intstring){
                                            
  char buf[intstring.length()];
  intstring.toCharArray(buf,intstring.length()+1);
  int val=atoi(buf);
  return val;
}

void Tank(String query){
                                                          
                                                          
                                                          
  String znak = split(query ,";",3);
  String floatstring = split(query ,";",4);
  char buf[floatstring.length()];
  floatstring.toCharArray(buf,floatstring.length()+1);
  double val=atof(buf);
  Serial2.println(val,2);   
  Serial2.println("ard;Tankfuel="+String(Tankfuel));
  if (val>0){
    Serial2.println("ard;split3=" + znak); 
    if(znak.equals("=")){Tankfuel=int(val*1000);};
    if(znak.equals("+")){Tankfuel=Tankfuel+int(val*1000);};
    if(znak.equals("-")){Tankfuel=Tankfuel-int(val*1000);};
  }
  Serial2.println("ard;Tankfuel="+String(Tankfuel)); 
  Savedata(); 
                              
                           
}

String split(String stroka, String deliver, byte ind){
  int index=1;
  String otvet="";
  for(int n=0; n<stroka.length(); n++){
    if (stroka.substring(n,n+1).equals(deliver)){
      index=index+1;
    }
    else{
      if (ind==index){
      otvet=otvet+stroka.charAt(n);
      }
    }
  }
return otvet;
}

void Eventlog(int event) {

  Eventcount=Eventcount+1;
  EEPROM_write(14, Eventcount);  
  int ind=Eventcount % logsize;
  logind[ind]=Eventcount;
  logtime[ind]=millis();
  logevent[ind]=event;
  logvolume[ind]=Volumelong;
  logdistance[ind]=Distancelong;
  logtimeengine[ind]= Worktimeinginelong;
  logstartcount[ind]= Countstartengine;

}   

void Senddata(String message) {
  Serial3.print(message);
}

void Proba(){                                 
   Eventlog(1);
   incomingdata="And:log";
   incomingdata+="\r\n";
}

void Refreshlongdata(){
Volumelong=Volumelong+int(Volume);                                
if((int(Volume))>=Tankfuel){
  Tankfuel=0;}
else{
  Tankfuel=Tankfuel-int(Volume);
}
Volume=Volume-int(Volume);
Distancelong=Distancelong+int(Distance);
Distance=Distance-int(Distance);
Worktimeinginelong=Worktimeinginelong+Worktimeingine/1000;
Worktimeingine=Worktimeingine%1000;
Cashlong=Cashlong+int(Cash);                                             
Cash=Cash-int(Cash);
}

boolean deletefile(String filename){ 
  char Stringtochar[filename.length()+ 1];
  filename.toCharArray(Stringtochar, filename.length()+1);  
  if (!SD.exists(Stringtochar)) {
    And.println("Ard:file " + String(Stringtochar) + " no delete, file doesn't exist");
    return false;
  }
//  else {
//    Serial2.println("no delete " + filename + ", file doesn't exist");
//    return false;
//  }

//  Serial2.println("delete file ..." + String(Stringtochar));
  SD.remove(Stringtochar);

  if (SD.exists(Stringtochar)) {
    And.println("Ard:file " + String(Stringtochar) + " delete error");
    return false;
  }
  else {
    And.println("Ard:file " + String(Stringtochar) + " deleted");
    return true;
  }
}

void sendfile(){
  sendfile(namefile());
}

void sendfile(String filename){ 
  char Stringtochar[filename.length()+ 1];
  filename.toCharArray(Stringtochar, filename.length()+1);                           
  myFile = SD.open(Stringtochar);
  if (myFile) {
            Serial2.println("opened " + filename);
    while (myFile.available()) {
      byte b=myFile.read();
        Serial2.write(b);                     
    }                
    myFile.close();
  } else {     
        Serial2.println("error open " + filename);                                 
  }  
}

String namefile(){      
    String  namefile1;                  
    DateTime now = RTC.now();
    if (now.month()<10){
      namefile1=String(now.year())+"0"+String(now.month());}
    else{
      namefile1=String(now.year())+String(now.month());}
    if (now.day()<10){
      namefile1=namefile1+"0"+String(now.day())+".LOG";}
    else{
      namefile1=namefile1+String(now.day())+".LOG";} 
    return  namefile1;                                    
}  

void Fuelevent(){
      byte type=buffer2[byte(beginpacket2+3)];
      byte len=buffer2[byte(beginpacket2+2)];
      unsigned long fuel;
      unsigned long price=0;
      fuel=bytearraytolong(buffer2,beginpacket2+4);
      Tankfuel=Tankfuel+fuel;
      if (len==13){
        price=bytearraytolong(buffer2,beginpacket2+8);
        Pricefuel=double(price)/100;
      }
       Eventcount=Eventcount+1;       
       Savedata();       
  //     MPwrite(namefile()); 
       Sendmydata();
       MPwrite(namefile()); 
       MPbegin(byte(1),byte(253));
       MPadd(type);
       MPadd(Eventcount);
       DateTime now0=RTC.now();
       unsigned long Currenttime=now0.unixtime();
       MPadd(Currenttime);
       MPadd(fuel);
       if (price>0){MPadd(price);}
       MPsend();
       MPwrite(namefile());
}

void Startevent(){
  if(lastprm==0&&buffer3[byte(beginpacket3+15)]>0){
     Countstartengine=Countstartengine+1;
     Eventcount=Eventcount+1;                     
     MPwrite(namefile()); 
     Sendmydata();
     MPwrite(namefile()); 
     MPbegin(byte(1),byte(253));
     MPadd(byte(8));
     MPadd(Eventcount);
      DateTime now0=RTC.now();
      unsigned long Currenttime=now0.unixtime();
      MPadd(Currenttime);
     MPsend();
     MPwrite(namefile()); 
     Volumebeforesrart=Volumelong;
     Distancebeforestart=Distancelong;
     Worktimeinginebeforestart=Worktimeinginelong;
     Cashbeforestart=Cashlong;
     And.println("Ard:start:" + getdatatime());
     TV.clear_screen();
 } 
}

void Stopevent(){
  if(lastprm>0&&buffer3[byte(beginpacket3+15)]==0){                                                                
        
    Eventcount=Eventcount+1;
    MPwrite(namefile()); 
    Sendmydata();
    MPwrite(namefile()); 
    MPbegin(byte(1),byte(253));
    MPadd(byte(9));
    MPadd(Eventcount);

    DateTime now0=RTC.now();
    unsigned long Currenttime=now0.unixtime();
    MPadd(Currenttime);
    
    MPsend();
    MPwrite(namefile());    
    Savedata();
    Volumebeforesrart=Volumelong;
    Distancebeforestart=Distancelong;
    Worktimeinginebeforestart=Worktimeinginelong;
    Cashbeforestart=Cashlong;    
    And.println("Ard:stop:" + getdatatime());
  };  
}

String base64encode(byte bytearray[], int len){
  int encodedLen = base64_enc_len(len);
  Serial.println("encodedLen="+String(encodedLen));
  char encoded[encodedLen];
  base64_encode(encoded, (char*)bytearray, len);
  return String(encoded);
}

int base64decode(String encoded,byte bytearray[]){
  int input2Len = encoded.length()+1; //не ясно нужно ли окончание строки нулевой байт считать
  Serial.println("input2Len="+String(input2Len));
  char input2[input2Len];
  Serial.println("input2Len="+String(input2Len));
  encoded.toCharArray(input2,input2Len);
  int decodedLen = base64_dec_len(input2, input2Len);
  Serial.println("decodedLen="+String(decodedLen));
  char decoded[decodedLen];
  base64_decode((char*)bytearray, input2, input2Len);
  int reallenencode=input2Len-1;
  if (encoded.indexOf('=')>0){reallenencode=encoded.indexOf('=');}
  Serial.println("reallenencode="+String(reallenencode));  
  int reallendecode=reallenencode*0.75;
  Serial.println("reallendecode="+String(reallendecode));
  return reallendecode;
}

String bytetostring(byte bytearray[], int len){
  String St="";
   for(int i=0;i<len;i++){
    St=St+String(bytearray[i]);
    St=St+',';
   }
  return St;
}

void LCDsenddouble(double number,unsigned int lennumber,unsigned int prezision,uint8_t col, uint8_t row){
  if (D)Serial2.println("Debug:" + String(number));
//  number=13.18;
  char buffer1[10];
  PString linenumber0(buffer1, 10, number,0); 
  if ((lennumber-linenumber0.length())<2){prezision=0;}
  if (lennumber<linenumber0.length()){lennumber=linenumber0.length();} 

  PString linenumber(buffer1, lennumber+1, number,prezision);
//  Serial.print(linenumber);
//  Serial.println("-");
//  Serial.println(linenumber.length());

  char buffer2[lennumber+1];
  PString linenumber1(buffer2, lennumber+1);
  unsigned int spacecount=lennumber-linenumber.length();
  linenumber1.begin();
  while(spacecount>0){
    linenumber1.print(" ");
    spacecount--;
  }
    linenumber1.println(linenumber);
  if (D)Serial2.print("LCD-");
  if (D)Serial2.println(linenumber1);
  LCD.sendString(linenumber1,col,row);
  if (D)Serial2.println("-LCD");

}

double Prm(){
  if(!packetfind){return 0;}
  return buffer3[byte(beginpacket3+15)]*25;
}
double VolTr(){return(Volumelong-Volumebeforesrart)/1000.0;}//л
double CashTr(){return(Cashlong-Cashbeforestart)/100.0;}//руб
double DistTr(){return(Distancelong-Distancebeforestart)/1000.0;}//км
double WorktimeTr(){return (Worktimeinginelong-Worktimeinginebeforestart)/3600.0;}//мч
double TAir(){
  if(!packetfind){return 0;}
  return(buffer3[byte(beginpacket3+10)]*0.75-40);
}
double TWat(){
  if(!packetfind){return 0;}
  return(buffer3[byte(beginpacket3+9)]*0.75-40);
}    
double Speed(){
  if(!packetfind){return 0;}
  return(buffer3[byte(beginpacket3+17)]);
} //км/ч
double Lh(){//л/ч
  if(!packetfind){return 0;}
  if (Prm()>0){
    return(flow);
  }
  else{
    return 0;
  }
}
double LhAvrTr(){//л/ч
  if(WorktimeTr()>0){
    return (VolTr()/WorktimeTr());
  }
  return 0;  
}
double LhAvr(){//л/ч
  if(Worktimeinginelong>0){
    double db=double(Volumelong)/double(Worktimeinginelong)*3.6;
    return db;
  }
  return 0;  
}
double L100(){//л/100км
  if(!packetfind){return 0;}
  if (Speed()>0){
    double db=flow/Speed()*100.0;
    if(db<100){return db;}
  }
  else{
    return 0;
  }
}
double L100AvrTr(){  //л/100км
  if(DistTr()>0){
    double db=VolTr()/DistTr()*100.0;
    if(db<100){return db;}
  }
  return 0;  
}
double L100Avr(){  //л/100км
  if (Distancelong>0){
//    Serial.println(Volumelong);
//    Serial.println(Distancelong);
//    Serial.println(100.0*Volumelong/Distancelong);
    double db=100.0*Volumelong/Distancelong;
    if(db<100){return db;}    
  }
  return 0;
}
double TankDist(){
  if(L100Avr()>0){
    return (0.1*Tankfuel/L100Avr());
  }
  return 0;
}

double SFuel(){
  return double(analogRead(SFuelPin));
}

int getmode(){
  if (analogRead(ACCPin)<300){return 0;}
  if ((millis()-moment)>2000){return 1;}
  if (lastprm==0){return 2;}
  return 3;
}

double gear(){
  if (Speed()==0){return 0;}
  return double(Prm())/double(Speed()) ;
}

