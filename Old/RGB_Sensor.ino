//#################################### хитрая штуковина которая выковыривает тип переменных #####################################################
template <typename T_ty> struct TypeInfo {
  static const char * name;
};
#define TYPE_NAME(var) TypeInfo< typeof(var) >::name
#define MAKE_TYPE_INFO(type)  template <> const char * TypeInfo<type>::name = #type;
MAKE_TYPE_INFO( int )
MAKE_TYPE_INFO( float )
MAKE_TYPE_INFO( bool )
MAKE_TYPE_INFO( byte )
MAKE_TYPE_INFO( String )

//#################################### хитрая штуковина которая выковыривает тип переменных #####################################################
#define MY_DEBUG 
//int mtwr;
//#define MY_TRANSPORT_WAIT_READY_MS (mtwr)
#define MY_RADIO_RF24
#define MY_RF24_PA_LEVEL RF24_PA_MAX
#ifndef MY_RF24_CE_PIN
#define MY_RF24_CE_PIN 8
#endif
#include <MySensors.h>
#include <SPI.h>
#include <IRremote.h>
#include <avr/wdt.h>

//==============================================================================================
////////NODE_SENSORS////////////////////////////////////////////////////////////////////////////
//==============================================================================================
#define Data_Sensors         1    //V_VAR1
#define ON_Sensor            2    //V_Status
#define Prog_Sensor          3    //V_VAR1
#define Step_Sensor          4    //V_VAR1
#define RGB_Sensors          5
#define Node_id              6    //V_VAR1
#define Period_send_node     7    //V_VAR1
//==============================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////
//==============================================================================================
#define BUTTON   2  // номер вывода кнопки 1 равен 12
#define RED      6
#define GREEN     5
#define BLUE     9

#define MaxSmallTry 5 //Количество попыток отправки пакета.
#define PauseTX 50

//MyMessage msgRGB(RGB_Sensors, V_TEXT);
//MyMessage msgID(Node_id, V_VAR1);
//MyMessage msgTime(Period_send_node, V_VAR1);

int RECV_PIN = 4;
IRrecv irrecv(RECV_PIN);
decode_results results;
 
void(* resetFunc) (void) = 0;
//==============================================================================================
//////////<< presentation >>////////////////////////////////////////////////////////////////////
//==============================================================================================
void presentation(){
  sendSketchInfo("RGB_node", "2.0", "15.09.2018");
  present(Data_Sensors,        S_CUSTOM,   "RGB data");
  present(ON_Sensor   ,        S_LIGHT,    "ON/OFF");
  present(Prog_Sensor,         S_CUSTOM,   "Prpg_data");
  present(Step_Sensor,         S_CUSTOM,   "Step_period");
  present(RGB_Sensors,         S_CUSTOM,   "RGB_data");
  present(Node_id,             S_CUSTOM,   "Node_new_ID");
  present(Period_send_node,    S_CUSTOM,   "Period_send_node");
}
//==============================================================================================
//////////<< VARIABLES >>///////////////////////////////////////////////////////////////////////
//==============================================================================================
unsigned long previsionTime, previsionTimeStep;
int Period_send = 60000;
byte Node_ID;
int value_int;
float value_float;
bool value_bool;
byte value_byte;
String value_String;
String Flag_type;
int8_t slumber;
String A;
byte RED_PWM, GREEN_PWM, BLUE_PWM, threshold = 255;
bool ON_OFF = 0, flagStart = 0, k, autoProg;
int STEP = 100;
byte PROG = 0, state = 0, index, color;

//==============================================================================================
//////////<< SETUP >>///////////////////////////////////////////////////////////////////////////
//==============================================================================================
void setup() {
  wdt_enable(WDTO_8S);
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, LOW);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  analogWrite(RED, 255);
  analogWrite(GREEN, 255);
  analogWrite(BLUE, 255);
  wait(2000);
  analogWrite(RED, 0);
  analogWrite(GREEN, 0);
  analogWrite(BLUE, 0);  
    irrecv.enableIRIn(); // Start the receiver
  int L = loadState(1);
  if((L) > 0 && (L)< 255)
  Period_send=L*60000;
  value_int = Period_send;                            // ЗАПИСЫВАЕТ В VALUE_BOOL ЗНАЧЕНИЕ ДЛЯ ОТПРАВКИ
  Flag_type = TYPE_NAME(value_int);         // ЗАПИСЫВАЕТ В ПЕРЕМЕННУЮ FLAG_TYPE ТИП ПЕРЕМЕННОЙ (VALUE_BOOL
  int z = TX(0, Period_send_node, V_VAR1);            // УХОД В ПП ОТПРАВКИ 
  A = (String(0) + "," + String(0) + "," + String(0));
  value_String = A;                      // ЗАПИСЫВАЕТ В VALUE_BOOL ЗНАЧЕНИЕ ДЛЯ ОТПРАВКИ
  Flag_type = TYPE_NAME(value_String);        // ЗАПИСЫВАЕТ В ПЕРЕМЕННУЮ FLAG_TYPE ТИП ПЕРЕМЕННОЙ (VALUE_BOOL
  z = TX(0, RGB_Sensors, V_VAR1);        // УХОД В ПП ОТПРАВКИ  
  value_bool = ON_OFF;                            // ЗАПИСЫВАЕТ В VALUE_BOOL ЗНАЧЕНИЕ ДЛЯ ОТПРАВКИ
  Flag_type = TYPE_NAME(value_bool);         // ЗАПИСЫВАЕТ В ПЕРЕМЕННУЮ FLAG_TYPE ТИП ПЕРЕМЕННОЙ (VALUE_BOOL
  z = TX(0, ON_Sensor, V_STATUS);            // УХОД В ПП ОТПРАВКИ 
  value_byte = PROG;                            // ЗАПИСЫВАЕТ В VALUE_BOOL ЗНАЧЕНИЕ ДЛЯ ОТПРАВКИ
  Flag_type = TYPE_NAME(value_byte);         // ЗАПИСЫВАЕТ В ПЕРЕМЕННУЮ FLAG_TYPE ТИП ПЕРЕМЕННОЙ (VALUE_BOOL
  z = TX(0, Prog_Sensor, V_VAR1);            // УХОД В ПП ОТПРАВКИ 
  value_int = STEP;                            // ЗАПИСЫВАЕТ В VALUE_BOOL ЗНАЧЕНИЕ ДЛЯ ОТПРАВКИ
  Flag_type = TYPE_NAME(value_int);         // ЗАПИСЫВАЕТ В ПЕРЕМЕННУЮ FLAG_TYPE ТИП ПЕРЕМЕННОЙ (VALUE_BOOL
  z = TX(0, Step_Sensor, V_VAR1);            // УХОД В ПП ОТПРАВКИ 
  }
//==============================================================================================
//////////<< LOOP >>////////////////////////////////////////////////////////////////////////////
//==============================================================================================
void loop() {
  wdt_reset();
 //  if (millis() - previousMillis > interval) {
  //  previousMillis = millis();
 //   sendHeartbeat(); 
 // }
     
  if (irrecv.decode(&results)) {
     Serial.println(results.value, HEX);
    if (results.value == (0x91EEC837)) // ON_OFF
      {
      Serial.println("IR - ok");
      ON_OFF = !ON_OFF;
      Serial.print("ON_OFF:");
      Serial.println(ON_OFF);
      Serial.print("STEP:");
      Serial.println(STEP);             
      if(ON_OFF == 1)
      PROG = 0;     
      if(ON_OFF == 0)
      {
      k = 0;
      autoProg = 0;
      color = 0;
      RED_PWM = 0;
      GREEN_PWM = 0;
      BLUE_PWM = 0;
      PWM();        
      }
   }
    if (results.value == (0x91EE08F7)){ //Код кнопки
      Serial.println("IR - ok");
      PROG += 1;
      if(PROG > 10)
      PROG = 0;
      Serial.print("PROG:");
      Serial.println(PROG);      
    }    

    if (results.value == (0x91EE18E7)){ //Код кнопки
      Serial.println("IR - ok");
      k = 0;
      autoProg = 1;
      Serial.print("autoProg:");
      Serial.println(autoProg);          
    }      
      if (results.value == (0x91EEE817)){ //Код кнопки
      Serial.println("IR - ok");
       k = 0;
      color += 1;
      if(color > 10)
      color = 0;
      Serial.print("color:");
      Serial.println(color);          
    }      

    if (results.value == (0x91EE9A65)){ //Код кнопки
      Serial.println("IR - ok");
      k = 0;
      if(threshold > 0)
      threshold -= 5;    
      Serial.print("threshold:");
      Serial.println(threshold);           
    }        

    if (results.value == (0x91EE1AE5)){ //Код кнопки
      Serial.println("IR - ok");
      k = 0;
      if(threshold < 255)
      threshold += 5;
      Serial.print("threshold:");
      Serial.println(threshold);           
    }         
    if (results.value == (0x91EEB04F)){ //Код кнопки
      Serial.println("IR - ok");    
      if(STEP < 5000)
      STEP += 100; 
      Serial.print("STEP:");
      Serial.println(STEP);       
    }         
    if (results.value == (0x91EE708F)){ //Код кнопки
      Serial.println("IR - ok");      
      if(STEP > 0)
      STEP -= 100;  
      Serial.print("STEP:");
      Serial.println(STEP);        
    }             
    irrecv.resume(); // Получаем следующее значение
  }
  
 if(ON_OFF == 1){
  if(PROG == 0 && flagStart == 1 && (k == 0 || autoProg == 1)){
    flagStart = 0;
     k = 1;
     if(color == 0){
      RED_PWM = 255;
      GREEN_PWM = 255;
      BLUE_PWM = 255;
      PWM();
     }
      if(color == 1){
      RED_PWM = 255;
      GREEN_PWM = 0;
      BLUE_PWM = 0;
      PWM();
     } 
      if(color == 2){
      RED_PWM = 0;
      GREEN_PWM = 255;
      BLUE_PWM = 0;
      PWM();
     }
      if(color == 3){
      RED_PWM = 0;
      GREEN_PWM = 0;
      BLUE_PWM = 255;
      PWM();
     }
     if(color == 4){
      RED_PWM = 255;
      GREEN_PWM = 69;
      BLUE_PWM = 0;
      PWM();
     }     
     if(color == 5){
      RED_PWM = 255;
      GREEN_PWM = 255;
      BLUE_PWM = 0;
      PWM();
     } 
      if(color == 6){
      RED_PWM = 106;
      GREEN_PWM = 90;
      BLUE_PWM = 205;
      PWM();
     }
     if(color == 7){
      RED_PWM = 238;
      GREEN_PWM = 130;
      BLUE_PWM = 238;
      PWM();
     }     
     if(color == 8){
      RED_PWM = 188;
      GREEN_PWM = 143;
      BLUE_PWM = 143;
      PWM();
     }    
     if(color == 9){
      RED_PWM = 0;
      GREEN_PWM = 255;
      BLUE_PWM = 255;
      PWM();
     }      
     if(color == 0){
      RED_PWM = 255;
      GREEN_PWM = 165;
      BLUE_PWM = 0;
      PWM();
     }     
     if(k == 1 && autoProg == 1)
     {
      if(color == 10)
      color = 0;
      else
      color++;
     }
  }   
   if(PROG == 1 && flagStart == 1){
     flagStart = 0;
     autoProg = 0;
           if (state == 0)
            {  
            analogWrite(RED, threshold);
            analogWrite(GREEN, 0);
            analogWrite(BLUE, 0);
            }
            if (state == 1)
            {  
            analogWrite(RED, 0);
            analogWrite(GREEN, threshold);
            analogWrite(BLUE, 0);
            }            
            if (state == 2)
            {  
            analogWrite(RED, 0);
            analogWrite(GREEN, 0);
            analogWrite(BLUE, threshold);
            }          
              state++;
              if (state > 2) state = 0;
   }
   if(PROG == 2 && flagStart == 1){
            autoProg = 0;
            flagStart = 0;
            if (state == 0)
            {  
               analogWrite(BLUE, index);
               analogWrite(GREEN, threshold - index);
            }
            if (state == 1)
            {  
               analogWrite(RED, index);
               analogWrite(BLUE, threshold - index);
            }            
            if (state == 2)
            {  
               analogWrite(GREEN, index);
               analogWrite(RED, threshold - index);
            }          
            if (index >= threshold)
            {
              state++;
              index = 0;
              if (state > 2) state = 0;
            }else
            index += 5;
   }
   if(PROG == 3 && k == 0){
    k = 1;
        RED_PWM = threshold;
        GREEN_PWM = threshold;
        BLUE_PWM = threshold;
        PWM();
   }
 }
   if(millis() - previsionTimeStep > STEP)
     {
      previsionTimeStep = millis();
      flagStart = 1;
     }
}
//==============================================================================================
//////////<< RECEIVE >>/////////////////////////////////////////////////////////////////////////
//==============================================================================================
void receive(const MyMessage &message) {           
    //====  NODE_RX_CMD ============    
    if (message.sender == 0){
    if (message.sensor == Data_Sensors){
    if (message.type == V_VAR1){
       A = _msg.getString();
       int str_len = A.length() + 1;
       char my_sB[str_len];
       A.toCharArray(my_sB, str_len);
       RED_PWM = atoi(strtok(my_sB, ","));
       GREEN_PWM = atoi(strtok(NULL, ","));
       BLUE_PWM = atoi(strtok(NULL, ","));      
       ON_OFF = atoi(strtok(NULL, ","));
       PROG = atoi(strtok(NULL, ","));   
       STEP = atoi(strtok(NULL, ","));           
       Serial.print("RED: ");
       Serial.println(RED_PWM);
       Serial.print("GREEN: ");
       Serial.println(GREEN_PWM);      
       Serial.print("BLUE: ");
       Serial.println(BLUE_PWM);   
       Serial.print("ON_OFF: ");
       Serial.println(ON_OFF);      
       Serial.print("PROG: ");       
       Serial.println(PROG);
       Serial.print("STEP: ");       
       Serial.println(STEP);       
       PWM();
    }
   }
  }  
       //====  NODE_RX_ON/OFF ============    
    if (message.sender == 0){
    if (message.sensor == ON_Sensor){
    if (message.type == V_STATUS){
       ON_OFF = message.getBool();
       if(ON_OFF == 1){ PROG = 0; k = 0;}
       if(ON_OFF == 0){ RED_PWM = 0; GREEN_PWM = 0; BLUE_PWM = 0; autoProg = 0; color = 0;}               
       Serial.print("RED: ");
       Serial.println(RED_PWM);
       Serial.print("GREEN: ");
       Serial.println(GREEN_PWM);      
       Serial.print("BLUE: ");
       Serial.println(BLUE_PWM);      
       PWM();
    }
   }
  }  
          //====  NODE_RX_Prog ============    
    if (message.sender == 0){
    if (message.sensor == Prog_Sensor){
    if (message.type == V_VAR1){
       PROG = message.getByte();
    }
   } 
  }  
    //====  NODE_RX_Step ============    
    if (message.sender == 0){
    if (message.sensor == Step_Sensor){
    if (message.type == V_VAR1){
       STEP = message.getInt();
    }
   } 
  } 
     //====  NODE_RX_RGB ============    
    if (message.sender == 0){
    if (message.sensor == RGB_Sensors){
    if (message.type == V_VAR1){
       A = _msg.getString();
       int str_len = A.length() + 1;
       char my_sB[str_len];
       A.toCharArray(my_sB, str_len);
       RED_PWM = atoi(strtok(my_sB, ","));
       GREEN_PWM = atoi(strtok(NULL, ","));
       BLUE_PWM = atoi(strtok(NULL, ","));               
       Serial.print("RED: ");
       Serial.println(RED_PWM);
       Serial.print("GREEN: ");
       Serial.println(GREEN_PWM);      
       Serial.print("BLUE: ");
       Serial.println(BLUE_PWM);      
       PWM();
    }
   } 
  }  
    //====  NODE_NEW_ID ============    
    if (message.sender == 0){
    if (message.sensor == Node_id){
    if (message.type == V_VAR1){
      Node_ID = message.getByte();
      hwWriteConfig(EEPROM_NODE_ID_ADDRESS, Node_ID);
      resetFunc();
    }
  }
 } 
      //====  PERIOD_SEND ============    
    if (message.sender == 0){
    if (message.sensor == Period_send_node){
    if (message.type == V_VAR1){
      Period_send = message.getByte();
      saveState(3, Period_send);
      Period_send = loadState(1) * 60000;
      Serial.print("Period_send: ");
      Serial.println(Period_send);
     } 
    }
  }    
}     
//==============================================================================================
//////////<< SEND >>////////////////////////////////////////////////////////////////////////////
//==============================================================================================
int TX(byte Master_id, byte ID_sensors, byte Type_variable)  //bool variable
{
  wdt_reset(); 
  byte count;
  bool send_data;
  MyMessage msg(ID_sensors, Type_variable);
  if (Flag_type == "int") 
    send_data = send(msg.setDestination(Master_id).setSensor(ID_sensors).set(value_int), true);  //setDestination(Master_ID).setSensor(1).set(State1)
  if (Flag_type == "float") 
    send_data = send(msg.setDestination(Master_id).setSensor(ID_sensors).set(value_float,2), true);  //setDestination(Master_ID).setSensor(1).set(State1)
  if (Flag_type == "bool") 
    send_data = send(msg.setDestination(Master_id).setSensor(ID_sensors).set(value_bool), true);  //setDestination(Master_ID).setSensor(1).set(State1)
  if (Flag_type == "byte") 
    send_data = send(msg.setDestination(Master_id).setSensor(ID_sensors).set(value_byte), true);  //setDestination(Master_ID).setSensor(1).set(State1) 
  if (Flag_type == "String") 
    send_data = send(msg.setDestination(Master_id).setSensor(ID_sensors).set(value_String.c_str()), true);  //setDestination(Master_ID).setSensor(1).set(State1)      
    
 _transportSM.failedUplinkTransmissions = 0;
wait(PauseTX, ID_sensors, Type_variable);
   if (send_data == 0)
     {
       Serial.print("failed: ");
       Serial.println(send_data);
       while (send_data == 0 && count < MaxSmallTry)
         {         
            wdt_reset(); 
            count++;  
            Serial.print("Sending a message try No.");      
            Serial.println(count);
            if (Flag_type == "int") 
              send_data = send(msg.setDestination(Master_id).setSensor(ID_sensors).set(value_int), true);  //setDestination(Master_ID).setSensor(1).set(State1)
            if (Flag_type == "float") 
              send_data = send(msg.setDestination(Master_id).setSensor(ID_sensors).set(value_float,2), true);  //setDestination(Master_ID).setSensor(1).set(State1)
            if (Flag_type == "bool") 
              send_data = send(msg.setDestination(Master_id).setSensor(ID_sensors).set(value_bool), true);  //setDestination(Master_ID).setSensor(1).set(State1)
            if (Flag_type == "byte") 
              send_data = send(msg.setDestination(Master_id).setSensor(ID_sensors).set(value_byte), true);  //setDestination(Master_ID).setSensor(1).set(State1)   
            if (Flag_type == "String") 
              send_data = send(msg.setDestination(Master_id).setSensor(ID_sensors).set(value_String.c_str()), true);  //setDestination(Master_ID).setSensor(1).set(State1)                
                  
            _transportSM.failedUplinkTransmissions = 0;
            wait(PauseTX, ID_sensors, Type_variable);   
         }  
     }       
   else
     { 
       Serial.print("send_data: ");
       Serial.println(send_data);   
     }
       
       count = 0;
       return send_data;
}
void PWM()
{
       analogWrite(RED, RED_PWM);
       analogWrite(GREEN, GREEN_PWM);
       analogWrite(BLUE, BLUE_PWM);
}

