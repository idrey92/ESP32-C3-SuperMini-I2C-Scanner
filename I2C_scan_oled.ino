#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define I2C_SDA 3
#define I2C_SCL 4

#define BUTTON_PIN 10
#define LED_PIN 8

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool autoScan = false;
bool buttonPressed = false;
unsigned long pressTime = 0;
unsigned long lastAutoScan = 0;
const unsigned long autoScanInterval = 3000; // Увеличено до 3 сек для комфортного чтения

// Расширенная база данных популярных I2C устройств
String getDeviceName(uint8_t address) {
  switch (address) {
    // 0x00 - 0x1F: Системные, акселерометры и магнитометры
    case 0x00: return "I2C General Call"; // Системный общий вызов
    case 0x0E: return "MAG3110 Magneto"; // Компас micro:bit
    case 0x0D: return "HMC5883L Magneto"; // Компас HMC5883L
    case 0x11: return "SI1145 UV/IR";    // Датчик освещенности и УФ Adafruit
    case 0x19: return "LSM303AGR Accel"; // Акселерометр
    case 0x1D: return "ADXL345 Accel";   // Популярный акселерометр (ALT-адрес)
    case 0x1E: return "HMC5883 Magneto"; // Магнитометр (компас)
    
    // 0x20 - 0x2F: Расширители портов и датчики света
    case 0x20: return "PCF8574 IO Exp";  // Расширитель портов GPIO
    case 0x23: return "BH1750 Light";    // Датчик освещенности (люксметр)
    case 0x27: return "LCD1602/2004";    // Символьный дисплей через PCF8574
    case 0x29: return "VL53L0X ToF /TCS34725"; // Лазерный дальномер или RGB датчик
    
    // 0x30 - 0x3F: Дисплеи и датчики среды
    case 0x38: return "AHT10/AHT20/SHT30"; // Климатические датчики
    case 0x39: return "TSL2561 Light";   // Датчик освещенности
    case 0x3C: return "OLED SSD1306 (Ours)"; // Наш собственный дисплей
    case 0x3D: return "OLED SSD1306 Alt"; // Альтернативный адрес OLED дисплея
    case 0x3F: return "LCD PCF8574 Alt"; // Альтернативный адрес LCD 1602
    
    // 0x40 - 0x4F: Датчики тока, АЦП, ШИМ-драйверы
    case 0x40: return "SHT21/HTU21D/INA219"; // Влажность или датчик тока
    case 0x44: return "SHT3X Temp/Hum";  // Прецизионный датчик температуры Sensirion
    case 0x48: return "ADS1115 / LM75";  // 16-битный АЦП или датчик температуры
    case 0x4A: return "TCA9548A Multiplex"; // Мультиплексор шины I2C
    
    // 0x50 - 0x5F: Энергонезависимая память (EEPROM)
    case 0x50: return "AT24C32 EEPROM";  // Чип памяти на модулях часов RTC
    case 0x53: return "ADXL345 / EEPROM"; // Акселерометр ADXL345 (Default)
    case 0x57: return "AT24C32 EEPROM Alt"; // Альтернативный адрес памяти
    case 0x5A: return "MPR121 Touch /MLX90614"; // Сенсорная клавиатура или ИК термометр
    case 0x5C: return "BH1750 Light Alt"; // Люксметр (ADDR пин поднят в HIGH)
    
    // 0x60 - 0x6F: Таймеры (RTC), гироскопы, сервоприводы
    case 0x60: return "MCP4725 DAC";     // ЦАП (Цифро-аналоговый преобразователь)
    case 0x68: return "DS3231 RTC/MPU6050"; // Часы реального времени или гироскоп 6-DOF
    case 0x69: return "MPU6050 Gyro Alt"; // Гироскоп MPU6050 (пин AD0 в HIGH)
    case 0x6A: return "LSM9DS1 IMU";     // Мощный 9-осевой инерциальный датчик
    
    // 0x70 - 0x7F: Барометры и драйверы моторов
    case 0x70: return "PCA9685 PWM/Servo"; // 16-канальный драйвер сервоприводов
    case 0x76: return "BMP280/BME280/MS5611"; // Датчик давления/температуры/влажности Bosch
    case 0x77: return "BMP180/BME280 Alt"; // Старый барометр BMP180 или BME280
    
    default:   return "Unknown Device";  // Если устройство кастомное или редкое
  }
}


void showMessage(String msg, int textSize = 1)
{
  display.clearDisplay();
  display.setTextSize(textSize);
  display.setCursor(0, 0);
  display.println(msg);
  display.display();
}

void scanBus()
{
  digitalWrite(LED_PIN, HIGH);
  showMessage("Scanning...", 2); // Пишем статус крупно

  int count = 0;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  for (uint8_t addr = 1; addr < 127; addr++)
  {
    if (addr == OLED_ADDRESS) continue; 

    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0)
    {
      char buf[12];
      sprintf(buf, "0x%02X:", addr);

      // Если нашли устройство и на экране еще есть место (максимум 5 строк для имен)
      if (count < 5) {
        display.print(buf);
        display.println(getDeviceName(addr)); // Выводим имя из базы данных
      }
      
      count++;
      Serial.print("Found: ");
      Serial.print(buf);
      Serial.println(getDeviceName(addr));
    }
    delay(5);
  }

  digitalWrite(LED_PIN, LOW);

  // Отрисовка подвала (последняя строка экрана)
  display.setCursor(0, 45);
  display.print("Total found: ");
  display.println(count);
  display.print(autoScan ? "AUTO: ON" : "AUTO: OFF");
  
  display.display();
  Serial.println("Scan complete");
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL, 400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
  {
    Serial.println("OLED not found");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Крупное стартовое меню
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("I2C SCAN");
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.println("Short: Scan");
  display.setCursor(0, 40);
  display.println("Long: Auto");
  display.display();
}

void loop()
{
  bool btn = !digitalRead(BUTTON_PIN);

  if (btn && !buttonPressed)
  {
    buttonPressed = true;
    pressTime = millis();
  }

  if (!btn && buttonPressed)
  {
    unsigned long duration = millis() - pressTime;
    buttonPressed = false;

    if (duration < 1000)
    {
      scanBus();
    }
  }

  if (btn && buttonPressed)
  {
    if ((millis() - pressTime > 1000))
    {
      autoScan = !autoScan;

      while (!digitalRead(BUTTON_PIN)) { delay(10); }
      buttonPressed = false;

      showMessage(String("AUTO SCAN\n\n") + (autoScan ? "ENABLED" : "DISABLED"), 2);
      delay(1000);
      
      if(autoScan) scanBus(); // Сразу запускаем первый круг автосканирования
    }
  }

  if (autoScan)
  {
    if (millis() - lastAutoScan > autoScanInterval)
    {
      lastAutoScan = millis();
      scanBus();
    }
  }
}
