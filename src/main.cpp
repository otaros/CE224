
/*-------------------------------- Includes --------------------------------*/
#include <mbed.h>
#include "BH1750.h"
#include "Adafruit_AHTX0.h"
#include "Adafruit_SSD1306.h"
// #include "Adafruit_ST7735.h"
// #include "RTClib.h"
// #include "SD.h"

/*--------------------------------- Define ---------------------------------*/
// begin
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define DISPLAY_ADDRESS 0x3C
#define LIGHT_METTER_ADDRESS 0x23
#define REFRESH_TIME 5
#define MOISTURE_PIN AIN0
// #define MOSI_PIN SPI_PSELMOSI0
// #define MISO_PIN SPI_PSELMISO0
// #define SCK_PIN SPI_PSELSCK0
// #define CS_PIN SPI_PSELSS0
// #define TRIGGER_PIN P1_11 // D2
// Flags
#define NEW_SENSING_CYCLE_FLAG (1UL << 0)
#define DONE_SENSING_FLAG (1UL << 1)
// end
/*------------------------------- Namespace -------------------------------*/
using namespace std;
using namespace mbed;
using namespace rtos;
/*-------------------------------- Typedef --------------------------------*/
typedef struct Package
{
  float lux = 0.0, moist = 0.0;
  sensors_event_t humidity, temp;
};
/*------------------------------- Instances--------------------------------*/
BH1750 lightMeter(LIGHT_METTER_ADDRESS);                                  // Light meter sensor
Adafruit_AHTX0 humTemp;                                                   // Humidity and temperature sensor
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // OLED I2C display
// Adafruit_ST7735 tft = Adafruit_ST7735(MOSI_PIN, MISO_PIN, SCK_PIN, CS_PIN, OLED_RESET); // TFT SPI display
EventFlags flags; // Event flags
Thread mainThread;
Thread sensorThread;
Thread displayThread;
Thread checkFlags;
Thread memMgrThread;
Ticker senseTicker;
// InterruptIn trigger(TRIGGER_PIN);
Queue<Package, 16> Data; // Queue for raw input data
MemoryPool<Package, 16> memPool;

/*-------------------------- Function prototypes --------------------------*/
void App_Task();
void processInput_Task();
void display_Task();
void memMgr_Task();
void startNewSensingCycle();
float toPercent(int, float, float, float, float);

/*---------------------------------- Code ----------------------------------*/
void setup()
{
  Serial.begin(9600); // Initialize serial port

  display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS);                              // Initialize display
  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE_2, LIGHT_METTER_ADDRESS, nullptr); // Initialize light meter
  humTemp.begin();                                                                   // Initialize humidity and temperature sensor

  senseTicker.attach(&startNewSensingCycle, chrono::seconds(REFRESH_TIME)); // Auto set Sensing flag every 5 seconds
  mainThread.start(App_Task);
  sensorThread.start(processInput_Task);
  displayThread.start(display_Task);
  checkFlags.start(startNewSensingCycle);
}

void loop()
{
}

void App_Task()
{
  while (1)
  {
  }
}

void processInput_Task()
{
  // sensors_event_t humidityRaw, tempeRaw;
  // float *luxRaw = new float(0.0);
  while (1)
  {
    flags.wait_any(NEW_SENSING_CYCLE_FLAG); // Wait for sensing signal

    Serial.println("Sensing..."); // Start sensing

    Package *pack = memPool.try_alloc(); // Allocate memory for package
    pack->lux = lightMeter.readLightLevel();
    humTemp.getEvent(&pack->humidity, &pack->temp);
    pack->moist = analogRead(MOISTURE_PIN);
    pack->moist = toPercent(pack->moist, 0.0, 1024.0, 0.0, 100.0);

    // use for testing purpose
    // pack->lux = rand() % 100 * 0.976;
    // pack->temp.temperature = random(0, 50) * 0.983;
    // pack->humidity.relative_humidity = random(0, 100) * 0.982;
    // pack->moist = rand() % 100 * 0.967;

    Data.try_put(pack); // Push data to rawData

    flags.set(DONE_SENSING_FLAG);

    Serial.println("Done sensing"); // End sensing
  }
}

void display_Task()
{
  float displayLux = 0.0, displayTemp = 0.0, displayHum = 0.0, displayMoist = 0.0;
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  while (1)
  {
    if (flags.wait_any(DONE_SENSING_FLAG) == DONE_SENSING_FLAG) // If new data processed
    {
      Package *pack = nullptr;
      Data.try_get(&pack);
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
    ThisThread::sleep_for(chrono::seconds(1));
  }
}

void memMgr_Task()
{
  while (1)
  {
  }
}

float toPercent(int x, float in_min = 0.0, float in_max = 1024.0, float out_min = 0.0, float out_max = 100.0)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void startNewSensingCycle()
{
  flags.set(NEW_SENSING_CYCLE_FLAG); // Set sensing flag every 5 seconds
}
