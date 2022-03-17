// -------------------
// Diplomarbeit: iWalk
// -----------------------------------------
// Erstellt von Georg Kaufmann u. Paul Resch
// -----------------------------------------

#include "BluetoothSerial.h"
#include "math.h"
#include "Wire.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

char buf[25];  // Mit diesem String getestet: "180#"
char* ptr = buf;
byte j;
int Steps = 0;
int Falls = 0;
int Weight = 0;
const long interval = 30000;
unsigned long ptime = 0;

Adafruit_MPU6050 mpu;

int val;

void setup() {
  Serial.begin(9600);
  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("Bluetooth Modul kann verbunden werden!");

  // MPU:
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);

  pinMode(25, INPUT);
}
  
// the loop routine runs over and over again forever:
void loop() {
  // ---------
  // Schritte:
  // ---------
  steps();
  
  // -------------
  // Kraftmessung:
  // -------------
  weight();

  // -------
  // Stürze:
  // -------
  fall(); 

  delay(500);
  
  // ---------------------
  // Bluetooth Connection:
  // ---------------------
  bluetooth();
  
}

void steps() {
  
  // Get new sensor events with the readings 
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  double gyro_x = g.gyro.x;
  double gx = 0;
  if(gyro_x > 0.1 && gyro_x < 0.7) {
    gx = 1; 
  }
  double gyro_y = g.gyro.y; 
  double gy = 0;
  if(gyro_y > 0.1 && gyro_y < 0.7) {
    gy = 1; 
  }
  double gyro_z = g.gyro.z;
  double gz = 0;
  if(gyro_z > 0.1 && gyro_z < 0.6) {
    gz = 1; 
  }
  
  double accel_x = a.acceleration.x;
  double ax = 0; 
  if(accel_x > 0.8 && accel_x < 1.5) {
    ax = 1; 
  }
  double accel_y = a.acceleration.y;
  double ay = 0; 
  if(accel_y > 1.4 && accel_y < 5) {
    ay = 1; 
  }
  double accel_z = a.acceleration.z;
  double az = 0; 
  if(accel_z > 6 && accel_z < 9) {
    az = 1; 
  }
  
  if(/*gx == 1 && gy == 1 && gz == 1 &&*/ ax == 1 && ay == 1) {
    Steps++;
  }
  Serial.println("--------------------");
  Serial.println("Schritte:");
  Serial.println(Steps);
}

void weight() {
  val = analogRead(25); // Auslesen des Sensorwertes (zwischen 0 und 4059)
  Weight = map(val,0,4095,0,20); // "mappen", also ins Verhältnis setzten des Sensorwertes (0-4059) zu 0-20kg
  
  Serial.println("gemessene Kraft:");
  Serial.println(val);
  Serial.println(Weight);
}

void fall() {
  // -------
  // Stürze:
  // -------

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  unsigned long t = millis();  

  if(t - ptime >= interval) {
    if((a.acceleration.x < -9.5 || a.acceleration.x > 9.5 || a.acceleration.y < -9.5 || a.acceleration.y > 9.5) && Weight < 1) {
       Falls++; 
       SerialBT.println("HELP ME");
     }
     ptime = t;
  } 

  Serial.println("Stürze:");
  Serial.println(Falls);
  Serial.println("--------------------");
}

void bluetooth() {
  // Eingang vom Bluetooth Modul auslesen
  if (SerialBT.available()) {
    buf[j] = SerialBT.read();
    j++;
  }

  
  // Bei # in der Anfrage, TestString zurück senden
  //if (buf[j-1]=='#') {
  //  SerialBT.println("SR70;KR50;ST0;#"); // Format des Strings: Schritte 70 Trennzeichen Maximale Kraft 50kg ;
                                              // . . . . . . . . . . 0 Stürze ; # zur Prüfung ob ganzer String eingeganen ist
  //} 

  // Bei # in der Anfrage, Werte zurück senden
  if (buf[j-1]=='#') {
    // SerialBT.println("SR" + Steps + ";" + "KR" Krafteinwirkung + ";" + "ST" + Falls + ";" + "#"); 
    SerialBT.print("SR");
    SerialBT.print(Steps);
    SerialBT.print(";");
    SerialBT.print("KR");
    SerialBT.print(Weight);
    SerialBT.print(";");
    SerialBT.print("ST");
    SerialBT.print(Falls);
    SerialBT.print(";");
    SerialBT.print("#");

    Steps = 0; 
    Falls = 0;
  } 
}
