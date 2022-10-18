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
#define DISPLAY_ADDRESS 0x3C
#define LIGHT_METTER_ADDRESS 0x23
#define REFRESH_TIME 5
#define MOISTURE_PIN AIN0
// Flags
#define NEW_SENSING_CYCLE_FLAG (1UL << 0)
#define DONE_SENSING_FLAG (1UL << 1)
#define DATA_PROCESSED_FLAG (1UL << 2)
#define DONE_READING_FLAG (1UL << 3)
// end
/*------------------------------- Namespace -------------------------------*/
using namespace std;
using namespace mbed;
using namespace rtos;
/*------------------------------- Variables -------------------------------*/
BH1750 lightMeter(LIGHT_METTER_ADDRESS);                                  // Light meter sensor
Adafruit_AHTX0 humTemp;                                                   // Humidity and temperature sensor
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // OLED display
AnalogIn moisture(MOISTURE_PIN);                                          // Moisture sensor analog input
EventFlags flags;                                                         // Event flags
Thread mainThread;
Thread sensorThread;
Thread displayThread;
// Thread memMgrThread;
Ticker senseTicker;
Mutex displayMux("displayMux"); // Mutex for display
Mutex senseMux("senseSem");     // Mutex for sensing
Queue<float, 4> rawData;        // Queue for raw input data
Queue<float, 4> displayData;    // Queue for display data

/*-------------------------- Function prototypes --------------------------*/
void App_Task();
void processInput_Task();
void display_Task();
void startNewSensingCycle();
// void memMgr_Task();
float toPercent(int, float, float, float, float);

/*---------------------------------- Code ----------------------------------*/
void setup()
{
  Serial.begin(115200);                                      // Initialize serial port
  if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS)) // Initialize display
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  flags.clear();
  senseTicker.attach_us(&startNewSensingCycle, REFRESH_TIME); // Auto set Sensing flag every 5 seconds
  mainThread.start(App_Task);
  sensorThread.start(processInput_Task);
  displayThread.start(display_Task);
}

void loop() {}

void App_Task()
{
  float *luxComputed = new float;
  float *tempComputed = new float;
  float *humComputed = new float;
  float *moistComputed = new float;

  uint32_t readflag;
  while (1)
  {
    readflag = flags.wait_any(DONE_READING_FLAG);
    if (readflag == DONE_READING_FLAG)
    {
      rawData.try_get(&luxComputed);
      rawData.try_get(&tempComputed);
      rawData.try_get(&humComputed);
      rawData.try_get(&moistComputed);

      *moistComputed = toPercent(*moistComputed, 0.0, 4095.0, 0.0, 100.0);

      displayData.try_put(luxComputed);
      displayData.try_put(tempComputed);
      displayData.try_put(humComputed);
      displayData.try_put(moistComputed);

      flags.set(DATA_PROCESSED_FLAG);
      flags.clear(DONE_READING_FLAG);
    }
  }
}

void processInput_Task()
{
  sensors_event_t humidityRaw, tempeRaw;
  float *luxRaw = new float(0.0);
  while (1)
  {
    flags.wait_any(NEW_SENSING_CYCLE_FLAG); // Wait for sensing signal

    Serial.println("Sensing..."); // Start sensing
    *luxRaw = lightMeter.readLightLevel();
    humTemp.getEvent(&humidityRaw, &tempeRaw);
    float *val = new float(moisture.read_u16());

    rawData.try_put(luxRaw);
    rawData.try_put(&humidityRaw.relative_humidity);
    rawData.try_put(&tempeRaw.temperature);
    rawData.try_put(val);

    flags.set(DONE_SENSING_FLAG);

    Serial.println("Done sensing"); // End sensing
  }
}

void display_Task()
{
  while (1)
  {
    flags.wait_any(DATA_PROCESSED_FLAG); // Wait for data processed signal
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