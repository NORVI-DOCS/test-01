/*
  ESP32-S2-MINI-1 basic hardware test
  Board : ESP32S2 Dev Module (USB CDC On Boot = Enabled)
  Output: Serial monitor over the native USB port

  Tests:
    1. Chip / flash / PSRAM / heap information
    2. Internal temperature sensor
    3. GPIO output (LED blink)
    4. ADC read
    5. Wi-Fi scan
*/

#include <WiFi.h>

// ---- Configuration ----------------------------------------------------
// Change TEST_LED_PIN to the LED / GPIO wired on your board.
#ifdef LED_BUILTIN
  #define TEST_LED_PIN LED_BUILTIN
#else
  #define TEST_LED_PIN 2
#endif

#define TEST_ADC_PIN 1            // GPIO1 = ADC1_CH0 on ESP32-S2
#define REPORT_INTERVAL_MS 5000   // How often to print the status report
#define BLINK_INTERVAL_MS 500     // LED toggle period

unsigned long lastReport = 0;
unsigned long lastBlink  = 0;
bool ledState = false;
uint32_t cycleCount = 0;

// ---- Helpers ----------------------------------------------------------
void printChipInfo() {
  Serial.println("---- Chip Info ----");
  Serial.printf("Model         : %s (rev %d)\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("CPU cores     : %d\n", ESP.getChipCores());
  Serial.printf("CPU freq      : %u MHz\n", getCpuFrequencyMhz());
  Serial.printf("Flash size    : %u KB\n", ESP.getFlashChipSize() / 1024);
  Serial.printf("Flash speed   : %u MHz\n", ESP.getFlashChipSpeed() / 1000000);
  Serial.printf("Sketch size   : %u KB\n", ESP.getSketchSize() / 1024);
  Serial.printf("Free sketch   : %u KB\n", ESP.getFreeSketchSpace() / 1024);
  Serial.printf("PSRAM size    : %u KB\n", ESP.getPsramSize() / 1024);
  Serial.printf("SDK version   : %s\n", ESP.getSdkVersion());
  Serial.printf("MAC (Wi-Fi)   : %s\n", WiFi.macAddress().c_str());
  Serial.println();
}

void printHeap() {
  Serial.printf("Heap free     : %u bytes (min ever %u, largest block %u)\n",
                ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
}

void testTemperature() {
  float t = temperatureRead();  // Internal sensor, approximate die temperature
  Serial.printf("Die temp      : %.1f C\n", t);
}

void testAdc() {
  int raw = analogRead(TEST_ADC_PIN);
  int mv  = analogReadMilliVolts(TEST_ADC_PIN);
  Serial.printf("ADC GPIO%d     : raw=%d, %d mV\n", TEST_ADC_PIN, raw, mv);
}

void testWifiScan() {
  Serial.println("---- Wi-Fi Scan ----");
  int n = WiFi.scanNetworks();
  if (n < 0) {
    Serial.println("Wi-Fi scan FAILED");
    return;
  }
  if (n == 0) {
    Serial.println("Scan OK, no networks found");
    return;
  }
  Serial.printf("Scan OK, %d network(s) found:\n", n);
  for (int i = 0; i < n && i < 10; i++) {
    Serial.printf("  %2d: %-24s ch%-2d %4d dBm %s\n",
                  i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i),
                  WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "secured");
  }
  WiFi.scanDelete();
  Serial.println();
}

// ---- Arduino entry points ---------------------------------------------
void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 3000) {
    delay(10);  // Wait for USB CDC host, but do not block forever
  }
  delay(200);

  Serial.println();
  Serial.println("==============================");
  Serial.println(" ESP32-S2-MINI-1 Test Started");
  Serial.println("==============================");

  pinMode(TEST_LED_PIN, OUTPUT);
  digitalWrite(TEST_LED_PIN, LOW);

  analogReadResolution(12);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  printChipInfo();
  printHeap();
  testTemperature();
  testAdc();
  Serial.println();
  testWifiScan();

  Serial.println("Entering loop: LED blinking, status report every 5 s");
  Serial.println();
}

void loop() {
  unsigned long now = millis();

  // Non-blocking LED blink
  if (now - lastBlink >= BLINK_INTERVAL_MS) {
    lastBlink = now;
    ledState = !ledState;
    digitalWrite(TEST_LED_PIN, ledState);
  }

  // Periodic status report
  if (now - lastReport >= REPORT_INTERVAL_MS) {
    lastReport = now;
    cycleCount++;
    Serial.printf("[#%lu] uptime=%lu s\n", (unsigned long)cycleCount, now / 1000);
    printHeap();
    testTemperature();
    testAdc();
    Serial.println();
  }
}
