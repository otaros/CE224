/*-------------------------------- Includes --------------------------------*/
#include <mbed.h>
#include "BH1750.h"
#include "Adafruit_AHTX0.h"
#include "Adafruit_SSD1306.h"

/*--------------------------------- Define ---------------------------------*/
// begin
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define DISPLAY_ADDRESS 0x78
#define LIGHT_METTER_ADDRESS 0x23
#define REFRESH_TIME 5
#define MOISTURE_PIN AIN0
// Flags
#define NEW_SENSING_CYCLE_FLAG (1UL << 0)
#define DONE_SENSING_FLAG (1UL << 1)
// end
/*------------------------------- Namespace -------------------------------*/
using namespace std;
using namespace mbed;
using namespace rtos;
/*-------------------------------- Typedef --------------------------------*/
typedef struct
{
  float lux = 0.0;
  float moist = 0.0;
  sensors_event_t humidity, temp;
} Package;
/*------------------------------- Instances--------------------------------*/
BH1750 lightMeter(LIGHT_METTER_ADDRESS);                                  // Light meter sensor
Adafruit_AHTX0 humTemp;                                                   // Humidity and temperature sensor
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // OLED display
AnalogIn moisture(MOISTURE_PIN);                                          // Moisture sensor analog input
EventFlags flags;                                                         // Event flags
Thread mainThread;
Thread sensorThread;
Thread displayThread;
Thread checkFlags;
// Thread memMgrThread;
Ticker senseTicker;
// Mutex displayMux("displayMux"); // Mutex for display
// Mutex senseMux("senseSem");     // Mutex for sensing
Queue<Package, 16> Data; // Queue for raw input data
MemoryPool<Package, 16> memPool;

/*-------------------------- Function prototypes --------------------------*/
void App_Task();
void processInput_Task();
void display_Task();
void startNewSensingCycle();
// void memMgr_Task();
float toPercent(int, float, float, float, float);
// void readDataFromQueue(Queue<float, 4>, *float);
// void putDataToQueue(Queue<float, 4>, float);

/*---------------------------------- Code ----------------------------------*/
void setup()
{
  Serial.begin(9600); // Initialize serial port

  display.begin();
  display.display();

  senseTicker.attach(&startNewSensingCycle, chrono::seconds(5)); // Auto set Sensing flag every 5 seconds
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
  // float *luxComputed = new float;
  // float *tempComputed = new float;
  // float *humComputed = new float;
  // float *moistComputed = new float;
  while (1)
  {
    // flags.wait_any(DONE_SENSING_FLAG);
    // rawData.try_get(&luxComputed);
    // rawData.try_get(&tempComputed);
    // rawData.try_get(&humComputed);
    // rawData.try_get(&moistComputed);

    // *moistComputed = toPercent(*moistComputed, 0.0, 4095.0, 0.0, 100.0);

    // displayData.try_put(luxComputed);
    // displayData.try_put(tempComputed);
    // displayData.try_put(humComputed);
    // displayData.try_put(moistComputed);
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
    Package *pack = memPool.try_alloc();
    /* pack->lux = lightMeter.readLightLevel();
    humTemp.getEvent(&pack->humidity, &pack->temp);
    pack->moist = moisture.read_u16();
    pack->moist = toPercent(pack->moist, 0.0, 4095.0, 0.0, 100.0); */
    pack->lux = 1.1;
    pack->humidity.temperature = 2.2;
    pack->humidity.relative_humidity = 3.3;
    pack->moist = 4.4;

    Data.try_put(pack); // Push data to rawData

    // rawData.try_put(luxRaw);
    // rawData.try_put(&humidityRaw.relative_humidity);
    // rawData.try_put(&tempeRaw.temperature);
    // rawData.try_put(val);

    flags.set(DONE_SENSING_FLAG);

    Serial.println("Done sensing"); // End sensing
  }
}

void display_Task()
{
  float displayLux = 0.0, displayTemp = 0.0, displayHum = 0.0, displayMoist = 0.0;
  while (1)
  {
    if (flags.wait_any(DONE_SENSING_FLAG, 50) == DONE_SENSING_FLAG) // If new data processed
    {
      // displayData.try_get(&displayLux);
      // displayData.try_get(&displayTemp);
      // displayData.try_get(&displayHum);
      // displayData.try_get(&displayMoist);
      Package *pack = nullptr;
      Data.try_get(&pack);
      displayLux = pack->lux;
      displayTemp = pack->temp.temperature;
      displayHum = pack->humidity.relative_humidity;
      displayMoist = pack->moist;
      memPool.free(pack);

      Serial.println("Done Reading");
    }
    Serial.println("Displaying...");
    Serial.println(displayLux);
    Serial.println(displayTemp);
    Serial.println(displayHum);
    Serial.println(displayMoist);

    // display.print("Lux: ");
    // display.print(*displayLux);
    // display.print(" Temp: ");
    // display.print(*displayTemp);
    // display.print(" Hum: ");
    // display.print(*displayHum);
    // display.print(" Moist: ");
    // display.print(*displayMoist);
    // display.display();
    ThisThread::sleep_for(chrono::seconds(1));
  }
}

float toPercent(int x, float in_min = 0, float in_max = 1024, float out_min = 0, float out_max = 100)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void startNewSensingCycle()
{
  flags.set(NEW_SENSING_CYCLE_FLAG); // Set sensing flag every 5 seconds
}
