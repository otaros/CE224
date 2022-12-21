/*-------------------------------- Includes --------------------------------*/
#include <MQUnifiedsensor.h>
#include <Wire.h>
#include <SoftwareSerial.h>

/*--------------------------------- Define ---------------------------------*/
#define Board ("Arduino UNO")
#define MQ3_Pin (A0)
#define MOISTURE_PIN (A1)
#define SOFT_SERIAL_RX (2)
#define SOFT_SERIAL_TX (3)
/***********************Software Related Macros************************************/
#define Type ("MQ-3") // MQ3
#define Voltage_Resolution (5)
#define ADC_Bit_Resolution (10) // For arduino UNO/MEGA/NANO
#define RatioMQ3CleanAir (60)   // RS / R0 = 60 ppm
/*-------------------------------- Typedef --------------------------------*/
typedef struct
{
    float alcohol = 0.0, methane = 0.0, carbon_monoxide = 0.0, moisture = 0.0;
} Package;
/*------------------------------- Instances -------------------------------*/
MQUnifiedsensor MQ3(Board, Voltage_Resolution, ADC_Bit_Resolution, MQ3_Pin, Type);
SoftwareSerial mySerial(2, 3); // RX, TX
/*------------------------------- Variables -------------------------------*/
Package pack;
/*-------------------------- Functions prototype --------------------------*/
void sendSensorValue();

/*--------------------------------- Code ---------------------------------*/
void setup()
{
    // put your setup code here, to run once:
    Serial.begin(9600);
    mySerial.begin(9600);

    MQ3.setRegressionMethod(0);
    MQ3.init();

    Serial.print("Calibrating please wait.");
    float calcR0 = 0;
    for (int i = 1; i <= 100; i++)
    {
        MQ3.update(); // Update data, the arduino will read the voltage from the analog pin
        calcR0 += MQ3.calibrate(RatioMQ3CleanAir);
        Serial.print(".");
    }
    MQ3.setR0(calcR0 / 100);
    Serial.println("  done!.");

    if (isinf(calcR0))
    {
        Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
        while (1)
            ;
    }
    if (calcR0 == 0)
    {
        Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
        while (1)
            ;
    }

    // Wire.begin(0x03);
    // Serial.println("I2C Slave started");
    // Wire.onRequest(sendSensorValue);
}

void loop()
{
    // put your main code here, to run repeatedly:
    while (mySerial.available() == 0)
        ;
    char c = mySerial.read();
    if (c == '1')
    {
        Serial.println("Received request");
        MQ3.setA(-0.062102441);
        MQ3.setB(1.842498924);
        pack.methane = MQ3.readSensor();
        // Wire.write((byte *)&pack.methane, sizeof(float));
        mySerial.write((byte *)&pack.methane, sizeof(float));
        Serial.println(pack.methane);

        MQ3.setA(-0.884918621);
        MQ3.setB(4.574317431);
        pack.carbon_monoxide = MQ3.readSensor();
        // Wire.write((byte *)&pack.carbon_monoxide, sizeof(float));
        mySerial.write((byte *)&pack.carbon_monoxide, sizeof(float));
        Serial.println(pack.carbon_monoxide);

        MQ3.setA(-0.754896547);
        MQ3.setB(1.996459667);
        pack.alcohol = MQ3.readSensor();
        // Wire.write((byte *)&pack.alcohol, sizeof(float));
        mySerial.write((byte *)&pack.alcohol, sizeof(float));
        Serial.println(pack.alcohol);

        pack.moisture = analogRead(MOISTURE_PIN);
        // Wire.write((byte *)&pack.moisture, sizeof(float));
        mySerial.write((byte *)&pack.moisture, sizeof(float));
        Serial.println(pack.moisture);

        // Wire.endTransmission();
        // Wire.write((byte*)&pack, sizeof(Package));
        // Wire.write((byte*)&pack.alcohol, sizeof(float));
        Serial.println("Sent!");
    }
}

void sendSensorValue()
{
    // Serial.println("Received request");
    // Package pack;
    // MQ3.update();

    // MQ3.setA(-0.062102441);
    // MQ3.setB(1.842498924);
    // pack.methane = MQ3.readSensor();
    // Wire.write((byte *)&pack.methane, sizeof(float));

    // MQ3.setA(-0.884918621);
    // MQ3.setB(4.574317431);
    // pack.carbon_monoxide = MQ3.readSensor();
    // Wire.write((byte *)&pack.carbon_monoxide, sizeof(float));

    // MQ3.setA(-0.754896547);
    // MQ3.setB(1.996459667);
    // pack.alcohol = MQ3.readSensor();
    // Wire.write((byte *)&pack.alcohol, sizeof(float));

    // pack.moisture = analogRead(MOISTURE_PIN);
    // Wire.write((byte *)&pack.moisture, sizeof(float));

    // // Wire.endTransmission();
    // // Wire.write((byte*)&pack, sizeof(Package));
    // // Wire.write((byte*)&pack.alcohol, sizeof(float));

    // Serial.print(pack.alcohol);
    // Serial.print(" | ");
    // Serial.print(pack.methane);
    // Serial.print(" | ");
    // Serial.println(pack.carbon_monoxide);
    // Serial.println("Sent!");

    // delay(1000);
}