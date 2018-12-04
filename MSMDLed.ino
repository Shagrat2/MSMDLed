#define MY_NODE_ID AUTO
#define MY_PARENT_NODE_ID AUTO

//#define MY_NODE_ID 115
//#define MY_PASSIVE_NODE
//#define MY_RF24_CHANNEL 78

#define SKETCH_NAME "MSMD RGB"
#define SKETCH_MAJOR_VER "0"
#define SKETCH_MINOR_VER "1"

const int HeartBitInterval = 60000;
const int cSaveDelayed = 5000;

//====
#define MY_DEBUG 
#define DEBUG

//int mtwr;
//#define MY_TRANSPORT_WAIT_READY_MS (mtwr)
#define MY_RADIO_RF24
#define MY_RF24_PA_LEVEL RF24_PA_MAX
#ifndef MY_RF24_CE_PIN
  #define MY_RF24_CE_PIN 8
#endif
#include <MySensors.h>
#include <SPI.h>
#include <avr/wdt.h>
#include <IRremote.h>

//==== NODE_SENSORS
#define SensOn         2    // V_Status
#define SensRGB        5    // V_VAR1
#define SensProg       3    // V_VAR1
#define SensLevel      4    // V_LEVEL
#define SensProfile    6    // V_VAR1

#define SensUID      200
#define SensNewID    201  //V_VAR1

#define BUTTON   2  // номер вывода кнопки 1 равен 12
#define CH1_PIN  6
#define CH2_PIN  5
#define CH3_PIN  9

//=== Memory adress
#define COnState    0
#define CPrgPeriod  1
#define CPrgNum     2
#define CBrightness 3

#define CRVal       10
#define CGVal       11
#define CBVal       12

//#define MaxSmallTry 5 //Количество попыток отправки пакета.
//#define PauseTX 50

#define EPNONE      0xFF
#define cGateAdr    0x00

MyMessage msgState(SensOn, V_STATUS);
MyMessage msgRGB(SensRGB, V_RGB);
MyMessage msgProfile(SensProfile, V_VAR1);

MyMessage msgPrg(SensProg, V_VAR1);
MyMessage msgPrgStepTime(SensProg, V_VAR2);

MyMessage msgPrgBrightness(SensLevel, V_LIGHT_LEVEL);

int RECV_PIN = 4;
IRrecv irrecv(RECV_PIN);
decode_results results;
 
void(* resetFunc) (void) = 0;

unsigned long previsionTime; // HeartBit period
unsigned long saveDelayed = 0;  // Отложенное сохранение
byte Node_ID;                // ID of Node
bool isON = 0;               // IsON
byte Profile = 1;            // Profile: 0 - RGB, 1 - GRB
byte RCol, GCol, BCol = 0;   // Current colors
int8_t brightness = 100;     // Brightness Max

// Programs
byte PROG = 1;                        // Program
unsigned long previsionTimeStep = 0;  // Previrus time step
byte prgStep = 0;                     // Program step
byte index = 0;                       // Prg sub step
int prgStepTime = 100;                // Time next step

String Flag_type;
int8_t slumber;
String A;

struct cTItm {
  long Key;
  byte R;
  byte G;
  byte B;
};

struct cTItm ColorKey[16] = {
  {0xF720DF, 0xFF, 0x00, 0x00}, // Red
  {0xF7A05F, 0x00, 0xFF, 0x00}, // Green
  {0xF7609F, 0x00, 0x00, 0xFF}, // Blue
  {0xF7E01F, 0xFF, 0xFF, 0xFF}, // White

  {0xF710EF, 0xFF, 0xA5, 0x00}, // Orange       - colorhexa.com
  {0xF7906F, 0x90, 0xee, 0x90}, // Light green  - colorhexa.com
  {0xF750AF, 0x00, 0x00, 0x8b}, // Dark blue    - colorhexa.com

  {0xF730CF, 0x9b, 0x87, 0x0c}, // Dark yellow  - encycolorpedia.com
  {0xF7B04F, 0x00, 0xFF, 0xFF}, // Cyan         - colorhexa.com
  {0xF7708F, 0xa5, 0x2a, 0x2a}, // Brown        - colorhexa.com

  {0xF708F7, 0xFF, 0xFF, 0x00}, // Yellow       - encycolorpedia.com
  {0xF78877, 0xad, 0xd8, 0xe6}, // Light blue   - colorhexa.com
  {0xF748B7, 0x80, 0x00, 0x80}, // Purple       - colorhexa.com
  
  {0xF728D7, 0xFF, 0xFF, 0xED}, // Light yellow - colorhexa.com
  {0xF7A857, 0x87, 0xce, 0xeb}, // Sky blue     - colorhexa.com
  {0xF76897, 0xff, 0xc0, 0xcb}, // Pink         - colorhexa.com
};

void before() {
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, LOW);
  pinMode(CH1_PIN, OUTPUT);
  pinMode(CH2_PIN, OUTPUT);
  pinMode(CH3_PIN, OUTPUT);  

  //=== Laod last state ===
  uint8_t val;

  // Programm
  val = loadState(CPrgNum);
  if (val != 0xFF) {
    SetProg(val, false, false);
  }

  // Prg period
  val = loadState(CPrgPeriod);
  if (val != 0xFF) {
    SetStepTime(val*100, false, false);
  }

  // Brightness
  val = loadState(CBrightness);
  if (val != 0xFF) {
    SetBrightness(val, false, false);
  }

  // Color  
  SetColor(loadState(CRVal), loadState(CGVal), loadState(CBVal), false, false);

  // On
  val = loadState(COnState);
  if (val != 0xFF) {
    SetOn(val, false, false);
  }
}

 void setup() {
  // Request
  request(SensProfile, V_VAR1);      // Profile  
  request(SensRGB, V_RGB);           // Color
  request(SensLevel, V_LIGHT_LEVEL); // Light level
  request(SensProg, V_VAR1);         // Program
  request(SensProg, V_VAR2);         // Step time  
  request(SensOn, V_STATUS);         // On/Off
  
  wdt_enable(WDTO_8S);  
  irrecv.enableIRIn(); // Start the receiver 
}

void presentation(){
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);

  present(SensUID,      S_CUSTOM,   "2c3a692526a78bda");
  present(SensNewID,    S_CUSTOM,   "Node new ID");

  present(SensOn,       S_LIGHT,        "ON/OFF");
  present(SensRGB,      S_RGB_LIGHT,    "RGB");
  present(SensProg,     S_CUSTOM,       "Programe");
  present(SensLevel,    S_LIGHT_LEVEL,  "Brightness (0-100)");
  present(SensProfile,  S_CUSTOM,       "Profile 0=RGB;1=GRB");
}

void PrgTableStep() {  
  const char Data[11][3] = {
    { 255, 255, 255 },
    { 255, 0,   0   },
    { 0,   255, 0   },
    { 0,   0,   255 },
    { 255, 69,  0   },
    { 255, 255, 0   },
    { 106, 90,  205 },
    { 238, 130, 238 },
    { 188, 143, 143 },
    { 0,   255, 255 },
    { 255, 165, 0 }
  };
  
  const int DataLen = (sizeof(Data) / 3);

  // ???
  if (prgStep > DataLen)
    prgStep = 0;
  
  UpdColor(Data[prgStep][0], Data[prgStep][1], Data[prgStep][2]);
  prgStep++;
}

void Prog2_Step() {  
  if (prgStep > 2) prgStep = 0;
 
  switch (prgStep) {
  case 0:
    UpdColor(255, 0, 0);
    break;

  case 1:
    UpdColor(0, 255, 0);
    break;
     
  case 2:
    UpdColor(0, 0, 255);
    break;     
  }        
  prgStep++;   
}

void Prog3_Step() {
   if (prgStep > 2) {
     prgStep = 0;
     index = 0;
   }

   switch (prgStep) {
   case 0:     
     BCol = index;
     GCol = 255 - index;
     break;
     
   case 1:
     RCol = index;
     BCol = 255 - index;
     break;
     
   case 2:
     GCol = index;
     RCol = 255 - index;
     break;     
   }

   UpdColor(RCol, GCol, BCol);
   
   if (index >= 255){
      prgStep++;
      index = 0;
   } else
      index += 5;
}

void loop() {
  wdt_reset();

  int i;
  bool isOk = true;
  unsigned long tick = millis();
  
  if (irrecv.decode(&results)) {
    
    #ifdef DEBUG
      //Serial.println(results.value, HEX);
    #endif

    long BtnCode = results.value;
    irrecv.resume(); // Получаем следующее значение
    
    switch (BtnCode) {
      case 0xF740BF:
        SetOn(false, true, true);
        break;
        
      // On/Off
      case 0xF7C03F:
        SetOn(true, true, true);        
        break;

      // Next programm  
      case 0xF7D02F:
        SetProg(PROG+1, true, true);
        break;
      
      // Decriase brightness
      case 0xF7807F:
        SetBrightness(brightness-5, true, true);
        break;

      // Increate brightness
      case 0xF700FF:
        SetBrightness(brightness+5, true, true);
        break;

      // Incrise prg step
      case 0xF7C837:
        SetStepTime(prgStepTime+100, true, true);
        break;

      // Decrise prg step
      case 0xF7E817:
        SetStepTime(prgStepTime-100, true, true);
        break;
          
      default:
        isOk = false;
        
        for (i=0; i < (sizeof(ColorKey)/sizeof(cTItm)); i++){
          if (ColorKey[i].Key == results.value) {
            isOk = true;            
            SetProg(0, true, true);
            SetColor(ColorKey[i].R, ColorKey[i].G, ColorKey[i].B, true, true);
            break;
          }
        }
        break;
    }

    if (isOk) {
      #ifdef DEBUG      
        Serial.print(BtnCode, HEX); Serial.println(" - ok");
      #endif

      return;
    }    
  }

  if (tick - previsionTime > HeartBitInterval) {
    previsionTime = tick;
    sendHeartbeat(); 
  }

  if (!isON)
    return;

  if (tick - previsionTimeStep < prgStepTime) {
    return;
  }
  if ((saveDelayed != 0) && (tick - saveDelayed > cSaveDelayed)) {
    //! save to flash

    saveState(COnState, isON);
    saveState(CPrgNum, PROG);
    saveState(CPrgPeriod, prgStepTime/100);
    saveState(CBrightness, brightness);
    saveState(CRVal, RCol);
    saveState(CGVal, GCol);
    saveState(CBVal, BCol);

     #ifdef DEBUG      
       Serial.println("#Save state - ok");
     #endif
      
    saveDelayed = 0;
  }
  previsionTimeStep = millis();
         
  switch (PROG) {
  case 1:    
    PrgTableStep();
    break;

  case 2:  
    Prog2_Step();
    break;
    
  case 3:
    Prog3_Step();
    break;
    
  default:    
    break;
  }
}

void receive(const MyMessage &message) {
    if (message.isAck()) {
      return;
    }

    long fLong;    
    byte fByte;
    int fInt;
    char myStr[7]; 

    uint8_t Dest = message.sender;

    // Request
    if (mGetCommand(message) == C_REQ) {
      #ifdef DEBUG
        Serial.print("Incoming request from:");
        Serial.println(Dest);
        Serial.print(", for sensor: ");
        Serial.println(message.sensor);
      #endif

      switch (message.sensor) {
      case SensOn:
        send(msgState.set(isON).setDestination(Dest));
        break;
        
      case SensProg:
        switch (message.type) {
        case V_VAR1:
          send(msgPrg.set(PROG).setDestination(Dest));
          break;
        case V_VAR2:
          send(msgPrgStepTime.set(prgStepTime).setDestination(Dest));
          break;        
        }
        break;
      
      case SensRGB:        
        sprintf( myStr, "%02X%02X%02X", RCol, GCol, BCol);
        send(msgRGB.set(myStr).setDestination(Dest));
        break;

      case SensLevel:
        send(msgPrgBrightness.set(brightness).setDestination(Dest));
        break;       

      case SensProfile:
        send(msgProfile.set(Profile).setDestination(Dest));
        break;
      }

      return;
    }

    // Set NULL date (if empty sensors)
    if (strlen(message.getString()) == 0) {
      return;
    }
  
    switch (message.sensor) {
    // ON/OF; type == V_STATUS
    case SensOn:      
      SetOn(message.getBool(), true, false);
      break;        
      
    // Program
    case SensProg:
      switch (message.type) {
      case V_VAR1:
        SetProg(message.getByte(), true, false);        
        SetOn(true, true, true);
              
        break;
        
      case V_VAR2:      
        SetStepTime(message.getInt(), true, false);
        break;
      
      default:
        return;
      }
      break;    

    case SensLevel:
      SetBrightness(message.getInt(), true, false);
      break;
        
    // RGB Data; type == V_VAR1
    case SensRGB:      
      fLong = strtol( message.data, NULL, 16);
      SetColor(byte(fLong >> 16), byte(fLong >> 8), byte(fLong), true, false);
      break;  

    // Profile
    case SensProfile:
      Profile = message.getByte();

      #ifdef DEBUG
        Serial.print("Profile: ");
        Serial.println(Profile);
      #endif

      break;
    
    // NODE_NEW_ID; type == V_VAR1
    case SensNewID:
      Node_ID = message.getByte();
      
      #ifdef DEBUG
        Serial.print("NEW NodeID: ");
        Serial.println(Node_ID);
      #endif
            
      hwWriteConfig(EEPROM_NODE_ID_ADDRESS, Node_ID);
      resetFunc();

      return;
    }    
}

void SetOn(bool Val, bool SaveState, bool SendState){  
  if (Val != isON) {
    isON = Val;
    UpdColor(RCol, GCol, BCol);

    if (SaveState) {
      saveDelayed = millis();
      
      #ifdef DEBUG
        Serial.print("### Changed - ");
      #endif
    }
  }  
  
  if (SendState) {
    send(msgState.set(isON).setDestination(cGateAdr));
  }

  #ifdef DEBUG
    Serial.print("ON: ");
    Serial.println(isON);
  #endif
}

void SetProg(byte Val, bool SaveState, bool SendState){
  if (Val != PROG){
    if (Val > 3) Val = 0;
    PROG = Val;
  
    if (SaveState) {
      saveDelayed = millis();

      #ifdef DEBUG
        Serial.print("### Changed - ");
      #endif
    }  
  }  
  
  if (SendState) {
    send(msgPrg.set(PROG).setDestination(cGateAdr));
  }

  #ifdef DEBUG
    Serial.print("Change prog: "); 
    Serial.println(PROG);  
  #endif  
}

void SetBrightness(int Val, bool SaveState, bool SendState){
  if (Val != brightness){
    if (Val < 1) Val = 1;
    if (Val > 100) Val = 100;          
    brightness = Val;
        
    UpdColor(RCol, GCol, BCol);
  
    if (SaveState) {
      saveDelayed = millis();

      #ifdef DEBUG
        Serial.print("### Changed - ");
      #endif
    }  
  }  

  if (SendState) {
    send(msgPrgBrightness.set(brightness).setDestination(cGateAdr));
  }

  #ifdef DEBUG
    Serial.print("Brightness: "); 
    Serial.println(brightness);  
  #endif  
}

void SetStepTime(int Val, bool SaveState, bool SendState){
  if (Val != prgStepTime){
    if (Val < 1) Val = 1;
    if (Val > 5000) Val = 5000;
    prgStepTime = Val;
  
    if (SaveState) {
      saveDelayed = millis();

      #ifdef DEBUG
        Serial.print("### Changed - ");
      #endif
    }    
  }  
 
  if (SendState) {
    send(msgPrgStepTime.set(prgStepTime).setDestination(cGateAdr));
  }

  #ifdef DEBUG
    Serial.print("Step time: "); 
    Serial.println(prgStepTime);  
  #endif  
}

void SetColor(byte R, byte G, byte B, bool SaveState, bool SendState){
  RCol = R;
  GCol = G;
  BCol = B;

  UpdColor(R, G, B);

  if (SaveState) {
    saveDelayed = millis();

    #ifdef DEBUG
      Serial.print("### Changed - ");
    #endif
  }

  if (SendState) {
    char myStr[7];
    sprintf( myStr, "%02X%02X%02X", RCol, GCol, BCol);
    send(msgRGB.set(myStr).setDestination(cGateAdr));
  }

  #ifdef DEBUG
    Serial.print("Color: "); 
    Serial.print(R, HEX); 
    Serial.print(G, HEX); 
    Serial.println(B, HEX);
  #endif  
}

void UpdColor(byte R, byte G, byte B){
  
  R = brightness*R/100;
  G = brightness*G/100;
  B = brightness*B/100;

  if (!isON) {
    analogWrite(CH1_PIN, 0);
    analogWrite(CH2_PIN, 0);
    analogWrite(CH3_PIN, 0);
    return;
  }
  
  switch (Profile) {   
  case 1: 
    // GRB
    analogWrite(CH1_PIN, G);
    analogWrite(CH2_PIN, R);    
    analogWrite(CH3_PIN, B);
    break;
    
  default:
    // RGB
    analogWrite(CH1_PIN, R);
    analogWrite(CH2_PIN, G);
    analogWrite(CH3_PIN, B);
    break;
  }
}
