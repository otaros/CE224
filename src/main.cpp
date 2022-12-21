/*-------------------------------- Includes --------------------------------*/
#include <mbed.h>
#include "BH1750.h"
#include "Adafruit_AHTX0.h"
#include "Adafruit_SSD1306.h"
// #include "Adafruit_ST7735.h"
#include "SD.h"

/*--------------------------------- Define ---------------------------------*/
// begin
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define DISPLAY_ADDRESS 0x3C
#define LIGHT_METTER_ADDRESS 0x23
#define MQ_UNO_ADDRESS 0x03
#define REFRESH_TIME 5
#define MOISTURE_PIN A0
#define MOSI_PIN SPI_PSELMOSI0
#define MISO_PIN SPI_PSELMISO0
#define SCK_PIN SPI_PSELSCK0
// #define DISPLAY_CS_PIN P0_27
#define SD_CS_PIN D9
// #define TRIGGER_PIN P1_11 // D2
// Flags signal for flag1
#define NEW_SENSING_CYCLE_FLAG (1UL << 0)
#define DONE_SENSING_FLAG (1UL << 1)
#define READY_TO_DISPLAY_FLAG (1UL << 2)
// Flags signal for flag2
#define WRITE_SD_CARD_FLAG (1UL << 0)
#define DATA_READY_TO_WRITE_FLAG (1UL << 1)
/*------------------------------- Namespace -------------------------------*/
using namespace std;
using namespace mbed;
using namespace rtos;
/*-------------------------------- Typedef --------------------------------*/
typedef struct
{
  float lux = 0.0, moist = 0.0, alcohol = 0.0, methane = 0.0, carbon_monoxide = 0.0;
  sensors_event_t humidity, temp;
} Package;
/*------------------------------- Variables -------------------------------*/

/*------------------------------- Instances--------------------------------*/
BH1750 lightMeter(LIGHT_METTER_ADDRESS);                                  // Light meter sensor
Adafruit_AHTX0 humTemp;                                                   // Humidity and temperature sensor
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // OLED I2C display
// Adafruit_ST7735 tft = Adafruit_ST7735(MOSI_PIN, MISO_PIN, SCK_PIN, CS_PIN, OLED_RESET); // TFT SPI display
EventFlags flag1; // Event flag1
EventFlags flag2; // Event flag2
Thread mainThread;
Thread sensorThread;
Thread displayThread;
Thread memMgrThread;
Ticker senseTicker;
Ticker writeSDCard;
// InterruptIn trigger(TRIGGER_PIN);
Queue<Package, 1> dataFromSensor; // Queue for raw input data
Queue<Package, 1> dataForDisplay; // Queue for data to be displayed
Queue<Package, 1> dataForMemMgr;  // Queue for data to be stored in SD card
MemoryPool<Package, 3> memPool;

/*-------------------------- Function prototypes --------------------------*/
void App_Task();
void processInput_Task();
void display_Task();
void memMgr_Task();
void startNewSensingCycle();
void writetoSDCard();
void initPeripherals();
float toPercent(int, float, float, float, float);

/*---------------------------------- Code ----------------------------------*/
void setup()
{
  Serial.begin(9600); // Initialize serial port
  Serial1.begin(9600);
  initPeripherals(); // Initialize peripherals

  senseTicker.attach(&startNewSensingCycle, chrono::seconds(REFRESH_TIME)); // Auto set Sensing flag every 5 seconds
  writeSDCard.attach(&writetoSDCard, chrono::seconds(20));                  // Auto write to SD card every 20 seconds
  mainThread.start(App_Task);
  sensorThread.start(processInput_Task);
  displayThread.start(display_Task);
  memMgrThread.start(memMgr_Task);
}

void loop()
{
}

void App_Task()
{
  while (1)
  {
    flag1.wait_any(DONE_SENSING_FLAG);
    Package *pack = nullptr;
    dataFromSensor.try_get(&pack);
    Package *pack2 = memPool.try_alloc();
    *pack2 = *pack;
    dataForDisplay.try_put(pack2);
    flag1.set(READY_TO_DISPLAY_FLAG);
    if (flag2.wait_any(WRITE_SD_CARD_FLAG, 50) == WRITE_SD_CARD_FLAG)
    {
      Package *pack3 = memPool.try_alloc();
      *pack3 = *pack;
      dataForMemMgr.try_put(pack);
      flag2.set(DATA_READY_TO_WRITE_FLAG);
    }
    memPool.free(pack);
  }
}

void processInput_Task()
{
  // sensors_event_t humidityRaw, tempeRaw;
  // float *luxRaw = new float(0.0);
  while (1)
  {
    flag1.wait_any(NEW_SENSING_CYCLE_FLAG); // Wait for sensing signal
    Serial.println("Sensing...");           // Start sensing

    Package *pack = memPool.try_alloc(); // Allocate memory for package
    // Wire.requestFrom(MQ_UNO_ADDRESS, sizeof(float) * 4);
    // Wire.readBytes((byte *)&pack->methane, sizeof(float));
    // Wire.readBytes((byte *)&pack->carbon_monoxide, sizeof(float));
    // Wire.readBytes((byte *)&pack->alcohol, sizeof(float));
    // Wire.readBytes((byte *)&pack->moist, sizeof(float));
    Serial1.write('1');
    // while (Serial1.available() == 0)
    ;
    Serial1.readBytes((byte *)&pack->methane, sizeof(float));
    Serial.println(pack->methane);
    // while (Serial1.available() == 0)
    ;
    Serial1.readBytes((byte *)&pack->carbon_monoxide, sizeof(float));
    Serial.println(pack->carbon_monoxide);
    // while (Serial1.available() == 0)
    ;
    Serial1.readBytes((byte *)&pack->alcohol, sizeof(float));
    Serial.println(pack->alcohol);
    // while (Serial1.available() == 0)
    ;
    Serial1.readBytes((byte *)&pack->moist, sizeof(float));
    Serial.println(pack->moist);

    pack->lux = lightMeter.readLightLevel();
    humTemp.getEvent(&pack->humidity, &pack->temp);
    pack->moist = toPercent(pack->moist, 0.0, 1024.0, 0.0, 100.0);

    // use for testing purpose
    // pack->lux = rand() % 100 * 0.976;
    // pack->temp.temperature = random(0, 50) * 0.983;
    // pack->humidity.relative_humidity = random(0, 100) * 0.982;
    // pack->moist = rand() % 100 * 0.967;

    dataFromSensor.try_put(pack); // Push data to rawData

    flag1.set(DONE_SENSING_FLAG);

    Serial.println("Done sensing"); // End sensing
  }
}

void display_Task()
{
  float displayLux = 0.0, displayTemp = 0.0, displayHum = 0.0, displayMoist = 0.0, displayMethane = 0.0, displayCO = 0.0, displayAlcohol = 0.0;
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  while (1)
  {
    if (flag1.wait_any(READY_TO_DISPLAY_FLAG) == READY_TO_DISPLAY_FLAG) // If new data processed
    {
      Package *pack = nullptr;
      dataForDisplay.try_get(&pack);
      displayMethane = pack->methane;
      displayCO = pack->carbon_monoxide;
      displayAlcohol = pack->alcohol;
      displayLux = pack->lux;
      displayTemp = pack->temp.temperature;
      displayHum = pack->humidity.relative_humidity;
      displayMoist = pack->moist;
      memPool.free(pack);

      Serial.println("Done reading");
    }

    Serial.println("Displaying...");
    Serial.println(displayLux);
    Serial.println(displayTemp);
    Serial.println(displayHum);
    Serial.println(displayMoist);

    display.setCursor(0, 0);
    display.clearDisplay();
    display.print("Light: ");
    display.print(displayLux);
    display.println(" lux");
    display.print("Temp: ");
    display.print(displayTemp);
    display.println(" C");
    display.print("Hum: ");
    display.print(displayHum);
    display.println(" %");
    display.print("Soil: ");
    display.print(displayMoist);
    display.println(" %");
    display.display();

    ThisThread::sleep_for(chrono::seconds(1) + chrono::milliseconds(250));

    display.setCursor(0, 0);
    display.clearDisplay();
    display.print("Alcohol: ");
    display.print(displayAlcohol);
    display.println(" ppm");
    display.print("CH4: ");
    display.print(displayMethane);
    display.println(" ppm");
    display.print("CO: ");
    display.print(displayCO);
    display.println(" ppm");
    display.display();

    ThisThread::sleep_for(chrono::seconds(1) + chrono::milliseconds(250));
  }
}

void memMgr_Task()
{
  SDLib::File dataFile;
  while (1)
  {
    flag2.wait_any(DATA_READY_TO_WRITE_FLAG);
    dataFile = SD.open("data.txt", FILE_WRITE);
    Package *pack = nullptr;
    dataForMemMgr.try_get(&pack);

    dataFile.print(pack->lux);
    dataFile.print(',');
    dataFile.print(pack->temp.temperature);
    dataFile.print(',');
    dataFile.print(pack->humidity.relative_humidity);
    dataFile.print(',');
    dataFile.print(pack->moist);
    dataFile.print(',');
    dataFile.print(pack->methane);
    dataFile.print(',');
    dataFile.print(pack->carbon_monoxide);
    dataFile.print(',');
    dataFile.println(pack->alcohol);

    dataFile.close();

    Serial.println("Done writing to SD card");
    memPool.free(pack);
  }
}

float toPercent(int x, float in_min = 0.0, float in_max = 1024.0, float out_min = 0.0, float out_max = 100.0)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void startNewSensingCycle()
{
  flag1.set(NEW_SENSING_CYCLE_FLAG); // Set sensing flag every 5 seconds
}

void writetoSDCard()
{
  flag2.set(WRITE_SD_CARD_FLAG);
}

void initPeripherals()
{
  Wire.begin();                                         // Initialize I2C
  display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS); // Initialize display
  lightMeter.begin();                                   // Initialize light meter
  humTemp.begin();                                      // Initialize humidity and temperature sensor
  SD.begin(9);                                          // Initialize SD card
}
