// ---> Wifi Arduino
bool withWifi = true; //Zet withWifi op 'true' om met wifi te testen en 'false' om met de seriële monitor te testen.
// NIET AANPASSEN, startcommando's voor de wifimodule en servomotoren.
String startsequence = "CMDS/";
String stopsequence = "/CMDEND/";

String startsequenceSetup = "SETUPB/";
String stopsequenceSetup = "/SETUPE/";

// ---> Servo's
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver MijnServo = Adafruit_PWMServoDriver();

#define SERVO_FREQ 50 // Analoge servo's werken op updates van ~ 50 Hz
// Schrijf hier het servonummer dat je wilt gebruiken.
int servo_wagen_nummer = 12; 
int servo_bak_nummer = 11;
int servo_afstandssensor_nummer = 9;
int interval_afstand_servo = 1;

///  SERVOMIN en SERVOMAX hangen af van het type servomotor! Deze waardes zijn voor de MG90's die 180° bereik heeft.
// Afhankelijk van het type servo dat je gebruikt zal je hier zelf je eigen waarden voor SERVOMIN en SERVOMAX moeten definiëren.
#define SERVOMIN  80 // Dit is de 'minimale' pulslengtetelling (van 4096)
#define SERVOMAX  409 // Dit is de 'maximale' pulslengtetelling (van 4096)

// --> Pin numbers

// --> Gewichtssensor pins
#include "HX711.h"
long calibration_factor = -7050.0; // Deze waarde wordt verkregen met behulp van de SparkFun_HX711_Calibration-schets
const int DOUT = 3;
const int CLK = 2;
HX711 scale;
long gewicht_bak = 0;
long vroeger_gewicht = 0;
int witte_bonen_gewicht = 50;
int zwarte_bonen_gewicht = 50;
int rode_bonen_gewicht = 100;

// --> Stel de kleursensorpennen in en constant
const int S0 = 45;
const int S1 = 4;
const int S2 = 53;
const int S3 = 46;
const int OUT = 28;
const int OE = 52;
// Definieer de kleuren en hun bijbehorende frequentiebereiken
const int red_min_redbean = 56;
const int red_max_redbean = 70;
const int green_min_redbean = 94;
const int green_max_redbean = 118;
const int blue_min_redbean = 83;
const int blue_max_redbean = 109;
const int red_min_whitebean = 10;
const int red_max_whitebean = 170;
const int green_min_whitebean = 10;
const int green_max_whitebean = 93;
const int blue_min_whitebean = 10;
const int blue_max_whitebean = 82;
const int red_min_blackbean = 71;
const int red_max_blackbean = 130;
const int green_min_blackbean = 220;
const int green_max_blackbean = 160;
const int blue_min_blackbean = 110;
const int blue_max_blackbean = 160;
int frequency_red = 0;
int frequency_green = 0;
int frequency_blue = 0;

// ---> DC motor pins
const int dc_motor_aan_uit = 9 ;
const int dc_motor_omgekeerd = 8 ;

// ---> Stappenmotor pins
const int stepPin = 12;
const int dirPin = 13;
const int enPin = 11;

// --> Boolean values
bool start_metingen = false;
bool start_bot = false;
bool rode_boon = false;
bool zwarte_boon = false;
bool witte_boon = false;
bool rode_bonen_nodig = false;
bool witte_bonen_nodig = false;
bool zwarte_bonen_nodig = false;
bool kleursensor_aan = false;
bool servo_wagen_wijzerszin = true;
bool dc_motor_aan = false;
bool dc_motor_wijzerszin = false;
bool servo_bak_wijzerszin = false;
bool servo_afstandssensor_terug = false;
int gemeten_bakken = 0;
int doorlopen_bakken = 0;
bool eerste_afstand = true;
bool code_nog_niet_doorlopen = true;

// --> Afstandssensor pins en constanten
const int pingPin = 44; // Trigger Pin van Ultrasonische Sensor
const int echoPin = 48; // Echo Pin van Ultrasonische Sensor
const long h_max = 4.72;
const long basis = 18.29;
const long alfa = 17;
const long l = 12.98;
const long RPM = 71.43 ;
const long h0 = 10;
const long r = 2.4;
long afstand = 0;
long cm_eind = 0;
long V = 0;

long gewicht_een_witte_en_rode_boon = 1/3;
long gewicht_een_zwarte_boon = 1/6;
long volume_een_witte_en_rode_boon = 0.75;
long volume_een_zwarte_boon = 0.50;

// --> LCD
#include <LiquidCrystal.h>
// De Code werkt adhv 6 variabelen. 3 die de positie van de bonen geven en 3 die de inhoud van het reservoir geven.

// Pin waarop de RS-pin van LCD is aangesloten
const int rs = 21;
// Pin waarop de EN-pin van LCD is aangesloten
const int en = 14;
// Pin waarop de D4-pin van LCD is aangesloten
const int d4 = 19;
// Pin waarop de D5-pin van LCD is aangesloten
const int d5 = 7;
// Pin waarop de D6-pin van LCD is aangesloten
const int d6 = 6;
// Pin waarop de D7-pin van LCD is aangesloten
const int d7 = 18;

// Initialiseer de bibliotheek met de nummers van de interfacepennen
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Variabelen om gewicht op te slaan voor elke kleur in de opslag
int gewicht_rood = 0;
int gewicht_wit = 0;
int gewicht_zwart = 0;

// Beginpositie voor elke kleur (1, 2 of 3)
int begin_kleur_rood = 2;
int begin_kleur_wit = 3;
int begin_kleur_zwart = 1;


void setup() {
  // --> Wifi setup
  Serial2.begin(115200); // Serial2 is de communicatie met de ESP32 (wifi-module).
  pinMode(25, OUTPUT);
  digitalWrite(25,HIGH);

  // ---> Colorsensor setup
  // Stel de pin-modi voor de kleursensor in
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);

  pinMode(OE, OUTPUT);
  digitalWrite(OE, LOW);

  // Stel de frequentieschaling in op 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // ---> DC motor setup
  pinMode(dc_motor_aan_uit,OUTPUT) ; //Logische pinnen worden ook ingesteld als uitvoer
  pinMode(dc_motor_omgekeerd,OUTPUT) ;

  // ---> Servo setup
  // -----------------NIET AANPASSEN ------------------------
  MijnServo.begin();
  MijnServo.setOscillatorFrequency(27000000);
  MijnServo.setPWMFreq(SERVO_FREQ);

  // ---> Stappenmotor setup
  // Stelt drie twee pinnen in als Uitgangen
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(enPin,OUTPUT);
  digitalWrite(enPin,LOW); // Schakelt de motor in

  // ---> Gewichtssensor setup
  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor); // Deze waarde wordt verkregen met behulp van de SparkFun_HX711_Calibration-schets
  scale.tare(); // Ervan uitgaande dat er bij het opstarten geen gewicht op de weegschaal staat, zet u de weegschaal terug op 0

  // ---> LCD setup
  // Stel het aantal kolommen en rijen van het LCD-scherm in
  lcd.begin(16, 2);
}


// ---------------------------------- Functies die gebruik maken van de wifi om het process in gang te steken ----------------------------------

void start_zonder_wifi() {
  withWifi = false;
  vroeger_gewicht = 0;
  gewicht_bak = 0;
  code_nog_niet_doorlopen = true;

  // Kies hier het nodig gewicht van bonen
  witte_bonen_gewicht = 100;
  zwarte_bonen_gewicht = 150;
  rode_bonen_gewicht = 200;

  if (rode_bonen_gewicht > 0) {
    rode_bonen_nodig = true;
  }
  if (witte_bonen_gewicht > 0) {
    witte_bonen_nodig = true;
  }
  if (zwarte_bonen_gewicht > 0) {
    zwarte_bonen_nodig = true;
  }
  stappen_motor_grote_stap_links();
  start_metingen = true;
  kleursensor_aan = true;
}


void wifi() {
  // Kijk steeds opnieuw of er iets werd verzonden over wifi.
  String command = checkwifi(); //Hier voer je de functie checkwifi() uit en sla je het resultaat op in de string command. De functie checkwifi() wordt lager in dit document gecodeerd.
  // Voer het commando uit als er iets werd ontvangen
  if (command.length()>0){
    //Splits het command op in commando/parameters
    String witte = command.substring(0,command.indexOf(':'));
    String zwarte = command.substring(command.indexOf(':')+1,command.lastIndexOf(':'));
    String rode = command.substring(command.lastIndexOf(':')+1,command.length());

    witte_bonen_gewicht = atol(witte.c_str());
    zwarte_bonen_gewicht = atol(zwarte.c_str());
    rode_bonen_gewicht = atol(rode.c_str());

    withWifi = false;
    code_nog_niet_doorlopen = true;
    vroeger_gewicht = 0;
    gewicht_bak = 0;
    cm_eind = 0;

    if (rode_bonen_gewicht > 0) {
      rode_bonen_nodig = true;
    }
    if (witte_bonen_gewicht > 0) {
      witte_bonen_nodig = true;
    }
    if (zwarte_bonen_gewicht > 0) {
      zwarte_bonen_nodig = true;
    }
    stappen_motor_grote_stap_links();
    start_metingen = true;
    kleursensor_aan = true;
  }
  if(withWifi){ // Als je wifi aanstaat, maakt dit if-statement het mogelijk om van de seriële monitor naar de app te communiceren. Je typt iets willekeurigs in de seriële monitor, en de app toont deze boodschap.
    if(Serial.available()){ // Voer deze actie uit als er iets verscheen in de seriële monitor.
      String boodschap=Serial.readString(); // Lees de boodschap in die je typte en sla deze op als een string.
      fromMonitorToApp(boodschap); // De void fromMonitorToApp() wordt onderaan dit bestand gedefinieerd.
    }
  }
}


// ---------------------------------------------------- Functies voor de connectie met ESP32; NIET AANPASSEN AUB ---------------------------------------------------------

String checkwifi() {
  //Kijkt of er iets werd verzonden over wifi, ontvangt het en decodeert het ook.
  String command = "";
  
  if (withWifi){
    // Je werkt met wifi, dus je leest de input van de ESP32.
    while (Serial2.available())
    {
      command = Serial2.readString();
    }
    // Haal de start- en stopsequence van het command
    if (command.indexOf(startsequence)>0){
      command = command.substring(command.indexOf(startsequence)+startsequence.length());
      command = command.substring(0,command.indexOf(stopsequence));
    }
    // Is het een setup commando? 
    else if (command.indexOf(startsequenceSetup)>0){
      displayESP32Setup(command);
      command = "";
    }
    else {
      command = "";
    }
  }
  else{
  // Je werkt zonder wifi, dus je leest de input van de seriële monitor.
     while (Serial.available())
    {
      command = Serial.readString();
    }
  }
  return command;
}

void sendWifi(String message){
  if (withWifi) {
    Serial2.println(message);
  }
  else {
    Serial.print("Message for Wifi: ");
    Serial.println(message);
  }
}

void displayESP32Setup(String command){
  command = command.substring(command.indexOf(startsequenceSetup)+startsequenceSetup.length());
  command = command.substring(0,command.indexOf(stopsequenceSetup));
  Serial.println(command);
}

void fromMonitorToApp(String message){
  sendWifi(message);
}

// ---------------------------------- PAS HIERONDER AAN WELKE COMMANDO'S JE ZELF WILT ONTVANGEN --------------------------------------------------------

// ---------------------------------- Functies voor servo's ----------------------------------

// Functie om de wagen heen en weer te laten gaan
void servo_wagen() {
  if (servo_wagen_wijzerszin) {
    // naar voor: 0 en na achteren 180
    int servoPWM = 0;
    servoPWM = map(servoPWM, 0, 180, SERVOMIN, SERVOMAX);
    MijnServo.setPWM(15-servo_wagen_nummer, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
    servo_wagen_wijzerszin = false;
  } else {
    int servoPWM = 180;
    servoPWM = map(servoPWM, 0, 180, SERVOMIN, SERVOMAX);
    MijnServo.setPWM(15-servo_wagen_nummer, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
    servo_wagen_wijzerszin = true;
  }
    delay(10000);
}


// Functie om de bak met de bonen derin te openen
void servo_bak() {
  // toe = 145, open = 65
  if (servo_bak_wijzerszin) {
    int servoPWM = 145;
    servoPWM = map(servoPWM, 0, 180, SERVOMIN, SERVOMAX);
    MijnServo.setPWM(15-servo_bak_nummer, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
    servo_bak_wijzerszin = false;
  } else {
    int servoPWM = 45;
    servoPWM = map(servoPWM, 0, 180, SERVOMIN, SERVOMAX);
    MijnServo.setPWM(15-servo_bak_nummer, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
    servo_bak_wijzerszin = true;
  }
    delay(10000);
}


// Functie om de afstandsensor naar beneden en naar boven te laten bewegen
void servo_afstandssensor() {
  // afstand boven:210 en beneden:0
  if (servo_afstandssensor_terug) {
    int servoPWM = 270;
    servoPWM = map(servoPWM, 0, 270, SERVOMIN, SERVOMAX);
    MijnServo.setPWM(15-servo_afstandssensor_nummer, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
  } else {
    int servoPWM = 270 - interval_afstand_servo;
    servoPWM = map(servoPWM, 0, 270, SERVOMIN, SERVOMAX);
    MijnServo.setPWM(15-servo_afstandssensor_nummer, 0, servoPWM); // de "15-" is een interne correctie, niet verwijderen!!
  }

  interval_afstand_servo += 1;
  delayMicroseconds(2.5);;
}


// ---------------------------------- Functies voor de stappenmotor ----------------------------------

// Functie draait de constructie naar rechts volgens de kleinere hoek, hier 15°
void stappen_motor_kleine_stap_rechts() {
  digitalWrite(dirPin,HIGH); // Hiermee kan de motor in een bepaalde richting bewegen
  // Maakt pulsen met elke puls gelijk aan 1.8° zodat 200 pulsen gelijk is aan één volledige cyclusomwenteling, 360°
  for(int x = 0; x < 8; x++) {
    digitalWrite(stepPin,HIGH); 
    delay(100);    // door deze tijdsvertraging tussen de stappen te wijzigen, kunnen we de rotatiesnelheid wijzigen
    digitalWrite(stepPin,LOW); 
    delay(100); 
  }
  delay(5000);
}


// Functie draait de constructie naar rechts volgens de grotere hoek, hier 30°
void stappen_motor_grote_stap_rechts() {
  digitalWrite(dirPin,HIGH); 
  for(int x = 0; x < 17; x++) {
    digitalWrite(stepPin,HIGH); 
    delay(100);
    digitalWrite(stepPin,LOW); 
    delay(100); 
  }
  delay(5000);
}


// Functie draait de constructie naar links volgens de grotere hoek, hier 30°
void stappen_motor_grote_stap_links() {
  digitalWrite(dirPin,LOW);
  for(int x = 0; x < 17; x++) {
    digitalWrite(stepPin,HIGH); 
    delay(100);
    digitalWrite(stepPin,LOW); 
    delay(100); 
  }
  delay(5000);
}


// Functie draait de constructie naar links volgens de kleinere hoek, hier 15°
void stappen_motor_kleine_stap_links() {
  digitalWrite(dirPin,LOW);
  for(int x = 0; x < 8; x++) {
    digitalWrite(stepPin,HIGH); 
    delay(100);
    digitalWrite(stepPin,LOW); 
    delay(100); 
  }

  delay(5000);
}


// // ---------------------------------- Functies het berekenen van het aantal bonen met behulp van de afstandsensor ----------------------------------

// Functie om de afstand te verkrijgen van de afstandssensor
void afstandssensor() {
  long duration, cm;
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
  cm = microsecondsToCentimeters(duration);
  cm_eind = cm;

  if (eerste_afstand && cm != 0 ) {
    afstand = cm;
    eerste_afstand = false;
  }
}


long microsecondsToCentimeters(long microseconds) {
   return microseconds / 29 / 2;
}


// Functie om de opslag bonen te bereken en wat er moet gebeuren na de berekening
void afstandssensor_berekeningen() {
  long tijd = 0;

  while (interval_afstand_servo <= 120) {
    afstandssensor();
    servo_afstandssensor();
    tijd += 2.5 *(10^(-3));

    if (cm_eind <= afstand) {
      long h = h0 - (60*RPM*r*tijd)/(2*3.1415);
      if (h <= h_max) {
        long V = (basis*(h^2)*2)/(3*tan(alfa));
      }
      if (h > h_max) {
        long V = (basis*(h^2)*2)/(3*tan(alfa)) + basis*l*(h - h_max) + (6+ 2*((h - h_max)^2)*3.1415*(h - h_max))/6;
      }
      if (rode_boon) {
        gewicht_rood = (V/volume_een_witte_en_rode_boon)*gewicht_een_witte_en_rode_boon;
        rode_boon = false;
      }
      if (witte_boon) {
        gewicht_wit = (V/volume_een_witte_en_rode_boon)*gewicht_een_witte_en_rode_boon;
        witte_boon = false;
      }
      if (zwarte_boon) {
        gewicht_zwart = (V/volume_een_zwarte_boon)*gewicht_een_zwarte_boon;
        zwarte_boon = false;
      }
      servo_afstandssensor_terug = true;
      interval_afstand_servo = 1;
      break;
    }
  }

  servo_afstandssensor();
  servo_afstandssensor_terug = false;
  gemeten_bakken += 1;

  if (gemeten_bakken >= 3) {
    start_metingen = false;
    gemeten_bakken = 0;
    stappen_motor_kleine_stap_links();
    stappen_motor_grote_stap_links();
    stappen_motor_grote_stap_links();
    start_bot = true;
  } else {
    stappen_motor_kleine_stap_rechts();
  } 
  kleursensor_aan = true;
}


// ---------------------------------- Functie voor LCD ----------------------------------

void lcd_scherm() {
  // Clear the LCD
  lcd.clear();
  
  // Print the colors and their weights in the desired order
  if (begin_kleur_rood == 1) {
    lcd.setCursor(0, 0);
    lcd.print("ROOD ");
    lcd.setCursor(0, 1);
    lcd.print(gewicht_rood);
    lcd.print("g");
  }
  else if (begin_kleur_rood == 2) {
    lcd.setCursor(6, 0);
    lcd.print("ROOD ");
    lcd.setCursor(6, 1);
    lcd.print(gewicht_rood);
    lcd.print("g");
  }
  else if (begin_kleur_rood == 3) {
    lcd.setCursor(12, 0);
    lcd.print("ROOD ");
    lcd.setCursor(12, 1);
    lcd.print(gewicht_rood);
    lcd.print("g");
  }
  
  if (begin_kleur_wit == 1) {
    lcd.setCursor(0, 0);
    lcd.print("WIT ");
    lcd.setCursor(0, 1);
    lcd.print(gewicht_wit);
    lcd.print("g");
  }
  else if (begin_kleur_wit == 2) {
    lcd.setCursor(6, 0);
    lcd.print("WIT ");
    lcd.setCursor(6, 1);
    lcd.print(gewicht_wit);
    lcd.print("g");
  }
  else if (begin_kleur_wit == 3) {
    lcd.setCursor(13, 0);
    lcd.print("WIT ");
    lcd.setCursor(12, 1);
    lcd.print(gewicht_wit);
    lcd.print("g");
  }
  
  if (begin_kleur_zwart == 1) {
    lcd.setCursor(0, 0);
    lcd.print("ZWART ");
    lcd.setCursor(0, 1);
    lcd.print(gewicht_zwart);
    lcd.print("g");
  }
  else if (begin_kleur_zwart == 2) {
    lcd.setCursor(6, 0);
    lcd.print("ZWART ");
    lcd.setCursor(6, 1);
    lcd.print(gewicht_zwart);
    lcd.print("g");
  }
  else if (begin_kleur_zwart == 3) {
    lcd.setCursor(11, 0);
    lcd.print("ZWART ");
    lcd.setCursor(12, 1);
    lcd.print(gewicht_zwart);
    lcd.print("g");
  }
}


// ---------------------------------- Functie voor kleursensor en acties die doorgevoerd moeten worden bij bepaalde kleuren ----------------------------------

void kleursensor(){
  if (kleursensor_aan) {
    // Instellen van rood gefilterde fotodiodes om te lezen
    digitalWrite(S2,LOW);
    digitalWrite(S3,LOW);
    // Uitlezen van de uitgangsfrequentie
    frequency_red = pulseIn(OUT, LOW);
    delay(100);

    // Instellen van groen gefilterde fotodiodes om te lezen
    digitalWrite(S2,HIGH);
    digitalWrite(S3,HIGH);
    // Uitlezen van de uitgangsfrequentie
    frequency_green = pulseIn(OUT, LOW);
    delay(100);

    // Instellen van blauw gefilterde fotodiodes om te lezen
    digitalWrite(S2,LOW);
    digitalWrite(S3,HIGH);
    // Uitlezen van de uitgangsfrequentie
    frequency_blue = pulseIn(OUT, LOW);
    delay(100);

    // Aanpassaen van frequentie bereiken zodat de correcte bonen worden gededecteerd
    // Alle acties worden doorgevoerd na detectie van bonen
    if (red_max_whitebean > frequency_red) {
      kleursensor_aan = false;
      witte_boon = true;
      if (start_metingen) {
        stappen_motor_kleine_stap_rechts();
        eerste_afstand = true;
        afstandssensor_berekeningen();
      } else {
        if (rode_bonen_nodig) {
          servo_wagen();
          dc_motor_aan = true;
        } else {
          check_bakken();
        }
      }
    } else if (green_min_blackbean < frequency_green) {
      kleursensor_aan = false;
      zwarte_boon = true;
      if (start_metingen) {
        stappen_motor_kleine_stap_rechts();
        eerste_afstand = true;
        afstandssensor_berekeningen();
      } else {
        if (rode_bonen_nodig) {
          servo_wagen();
          dc_motor_aan = true;
        } else {
          check_bakken();
        }
      } 
    } else {
      kleursensor_aan = false;
      rode_boon = true;
      if (start_metingen) {
        stappen_motor_kleine_stap_rechts();
        eerste_afstand = true;
        afstandssensor_berekeningen();
      } else {
        if (rode_bonen_nodig) {
          servo_wagen();
          dc_motor_aan = true;
        } else {
          check_bakken();
        }
      }
    }
  }
}


// ---------------------------------- Functies voor DC-motor, gewichtssensor en checken van bakken ----------------------------------

// --> DC motor functie
void dc_motor() {
  if (dc_motor_aan) {
    if(dc_motor_wijzerszin) {
      digitalWrite(dc_motor_aan_uit,HIGH);
      gewichtssensor();
    } else {
      digitalWrite(dc_motor_omgekeerd,HIGH);
      digitalWrite(dc_motor_aan_uit,HIGH);
      dc_motor_aan = false;
      dc_motor_wijzerszin = true;
      delay(10000);
      servo_wagen();
      check_bakken();
    }
  } else {
    digitalWrite(dc_motor_aan_uit,LOW);
  }
}


// Meet het gewicht en checkt wanneer de DC_motor moet omkere 
void gewichtssensor() {
  gewicht_bak = scale.get_units(); //scale.get_units() returns a float
 
  if (zwarte_boon) {
    if (gewicht_bak - vroeger_gewicht >= zwarte_bonen_gewicht ) {
      dc_motor_wijzerszin = false;
      vroeger_gewicht = gewicht_bak;
      zwarte_boon = false;
    }
  }
  if (rode_boon) {
    if (gewicht_bak - vroeger_gewicht >= rode_bonen_gewicht ) {
      dc_motor_wijzerszin = false;
      vroeger_gewicht = gewicht_bak;
      rode_boon = false;
    }
  }
  if (witte_boon) {
    if (gewicht_bak - vroeger_gewicht >= witte_bonen_gewicht ) {
      dc_motor_wijzerszin = false;
      vroeger_gewicht = gewicht_bak;
      witte_boon = false;
    }
  }
  String boodschap = String(gewicht_back, 0);
  fromMonitorToApp(boodschap);
}


void check_bakken() {
  doorlopen_bakken += 1;
  if (doorlopen_bakken >= 3) {
    start_bot = false;
    doorlopen_bakken = 0;
    stappen_motor_grote_stap_links();
    code_nog_niet_doorlopen = false;
    servo_bak();
    delay(10000);
    servo_bak();
    withWifi = true;
  } else {
    stappen_motor_grote_stap_rechts();
    kleursensor_aan = true;
  }
}


// ---------------------------------- De loop ----------------------------------

void loop() {
  if (withWifi) {
    wifi();
  } else {
    start_zonder_wifi();
  }

  if (start_bot || start_metingen){
    kleursensor();
    dc_motor();
  }
  
  lcd_scherm();

  // Blijft data sturen naar MIT app zodat de app aan blijft
  if (code_nog_niet_doorlopen) {
    String boodschap = " ";
    fromMonitorToApp(boodschap);
}