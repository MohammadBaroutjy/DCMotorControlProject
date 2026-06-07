#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// تعريف الـ UUIDs (يجب أن تطابق تماماً المعرفات الموجودة في كود الـ C# لديك)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// تعريف دبابيس التحكم بدرايفر L298N
const int pinENA = 12; 
const int pinIN1 = 13; 
const int pinIN2 = 14; 

const int pwmFreq = 100;    // تردد 100 هرتز لعزم قوي ومنع صوت الرنين
const int pwmResolution = 8; 
int motorSpeed = 200;        
bool isMotorRunning = false;
bool deviceConnected = false;

// كلاس لمراقبة اتصال وفصل الأجهزة
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("\n>>> [BLE STATUS] Smartphone Connected Successfully! <<<");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("\n>>> [BLE STATUS] Smartphone Disconnected. Re-advertising... <<<");
      // إعادة البث فوراً عند الفصل لكي يظل الجهاز قابلاً للبحث
      BLEDevice::startAdvertising();
    }
};

// كلاس لاستقبال ومعالجة الأوامر والسرعات القادمة من التطبيق
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = String(pCharacteristic->getValue().c_str());

      if (value.length() > 0) {
        char command = value[0];
        Serial.print("[BLE RECEIVED] Character: '");
        Serial.print(command);
        Serial.println("'");
        
        processCommand(command);
      }
    }

    void processCommand(char command) {
      // 1. أوامر السرعة (من 0 إلى 9)
      if (command >= '0' && command <= '9') {
        int speedPercent = (command - '0') * 10;
        if (speedPercent == 0) {
          motorSpeed = 0;
        } else {
          // مابينج ذكي لضمان عزم الإقلاع عند السرعات المنخفضة (تبدأ من 130)
          motorSpeed = map(speedPercent, 10, 100, 130, 255); 
        }
        Serial.print("   -> Speed Optimized To PWM: "); Serial.println(motorSpeed);
        if (isMotorRunning) ledcWrite(pinENA, motorSpeed);
      }
      // السرعة القصوى
      else if (command == 'M' || command == 'm') {
        motorSpeed = 255;
        Serial.println("   -> Speed Set to 100% MAX");
        if (isMotorRunning) ledcWrite(pinENA, motorSpeed);
      }
      // 2. أوامر الاتجاه والحركة
      else if (command == 'F' || command == 'f') {
        isMotorRunning = true;
        digitalWrite(pinIN1, HIGH);
        digitalWrite(pinIN2, LOW);
        ledcWrite(pinENA, motorSpeed);
        Serial.println("   -> Execution: Moving FORWARD");
      } 
      else if (command == 'B' || command == 'b') {
        isMotorRunning = true;
        digitalWrite(pinIN1, LOW);
        digitalWrite(pinIN2, HIGH);
        ledcWrite(pinENA, motorSpeed);
        Serial.println("   -> Execution: Moving BACKWARD");
      } 
      else if (command == 'S' || command == 's') {
        isMotorRunning = false;
        digitalWrite(pinIN1, LOW);
        digitalWrite(pinIN2, LOW);
        ledcWrite(pinENA, 0);
        Serial.println("   -> Execution: Motor STOPPED");
      }
    }
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=========================================");
  Serial.println("[SYSTEM] Initializing ESP32 Advanced BLE Mode...");

  // تهيئة دبابيس المحرك
  pinMode(pinIN1, OUTPUT);
  pinMode(pinIN2, OUTPUT);
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, LOW);

  // إعداد الـ PWM بدقة
  ledcAttach(pinENA, pwmFreq, pwmResolution);
  ledcWrite(pinENA, 0);

  // 1. إنشاء جهاز الـ BLE وتعيين الاسم الصريح
  BLEDevice::init("ESP32_Motor_Controller");

  // 2. إنشاء الـ BLE Server وربط أحداث الاتصال
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. إنشاء الخدمة (Service)
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. إنشاء الخاصية (Characteristic) مع تفعيل صلاحيات الكتابة والقراءة
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ  |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

  // ربط كلاس الاستقبال بالخاصية
  pCharacteristic->setCallbacks(new MyCallbacks());

  // بدء الخدمة
  pService->start();

  // 5. الحل السحري لإظهار الاسم فوراً وتخطي البطء (Advanced Advertising Configuration)
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  
  // إجبار الشريحة على تضمين الاسم في حزمة البث وحزمة الاستجابة للـ Scan
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // تحسينات لتسريع استجابة هواتف أندرويد وويندوز
  pAdvertising->setMinPreferred(0x12);
  
  // بدء البث الفيزيائي في الجو
  BLEDevice::startAdvertising();
  Serial.println("[SUCCESS] BLE Advertising Active! Name: 'ESP32_Motor_Controller'");
  Serial.println("=========================================\n");
}

void loop() {
  // دالة الـ loop فارغة تماماً ومستقرة، المعالجة تتم فورياً عبر الـ Interrupts (الأحداث الخلفية)
  delay(2000);
}


//4
// #include "BluetoothSerial.h"

// #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
// #error Bluetooth is not enabled! Please run `make menuconfig` to enable it.
// #endif

// BluetoothSerial SerialBT;

// // تعريف دبابيس التحكم بدرايفر L298N
// const int pinENA = 12; 
// const int pinIN1 = 13; 
// const int pinIN2 = 14; 

// const int pwmFreq = 5000;    
// const int pwmResolution = 8; 
// int motorSpeed = 200;        // السرعة الافتراضية عند الإقلاع (من 0 إلى 255)
// bool isMotorRunning = false;
// char lastDirection = 'F';    // حفظ آخر اتجاه للأمام كحالة بدائية

// void setup() {
//   Serial.begin(115200);
//   delay(1000);
//   Serial.println("\n=========================================");
//   Serial.println("[SYSTEM START] Motor & Speed Diagnosis Online...");

//   pinMode(pinIN1, OUTPUT);
//   pinMode(pinIN2, OUTPUT);
  
//   digitalWrite(pinIN1, LOW);
//   digitalWrite(pinIN2, LOW);

//   ledcAttach(pinENA, pwmFreq, pwmResolution);
//   ledcWrite(pinENA, 0);

//   SerialBT.begin("ESP32_Motor_Controller"); 
//   Serial.println("[INIT] Bluetooth 'ESP32_Motor_Controller' is Ready.");
//   Serial.println("[HELP] Serial Monitor Commands:");
//   Serial.println("       Type 'F' -> Move Forward");
//   Serial.println("       Type 'B' -> Move Backward");
//   Serial.println("       Type 'S' -> Stop Motor");
//   Serial.println("       Type '0' to '9' -> Set Speed from 0% to 90%");
//   Serial.println("       Type 'M' -> Set Maximum Speed 100%");
//   Serial.println("=========================================\n");
// }

// void loop() {
//   // 1. استقبال الأوامر من كيبورد الكمبيوتر (Serial Monitor)
//   if (Serial.available()) {
//     char serialCmd = Serial.read();
//     if (serialCmd != '\n' && serialCmd != '\r') {
//       Serial.print("[SERIAL INPUT] Character: '");
//       Serial.print(serialCmd);
//       Serial.println("'");
//       processCommand(serialCmd);
//     }
//   }

//   // 2. استقبال الأوامر من تطبيق الهاتف (Bluetooth)
//   if (SerialBT.available()) {
//     char btCmd = SerialBT.read();
//     Serial.print("[BLUETOOTH INPUT] Character: '");
//     Serial.print(btCmd);
//     Serial.println("'");
//     processCommand(btCmd);
//   }
//   delay(10);
// }

// // دالة معالجة الحركة والسرعة
// void processCommand(char command) {
  
//   // أولاً: فحص أوامر تغيير السرعة (أرقام من 0 إلى 9)
//   if (command >= '0' && command <= '9') {
//     int speedPercent = (command - '0') * 10;
//     motorSpeed = map(speedPercent, 0, 100, 0, 255);
    
//     Serial.println("-----------------------------------------");
//     Serial.print(">>> [SPEED UPDATE] Target Speed Set to: ");
//     Serial.print(speedPercent);
//     Serial.print("% | PWM Value (0-255): ");
//     Serial.println(motorSpeed);
//     Serial.println("-----------------------------------------");
    
//     if (isMotorRunning) {
//       ledcWrite(pinENA, motorSpeed);
//       Serial.println("[PWM EXECUTION] Speed applied to running motor instantly.");
//     } else {
//       Serial.println("[NOTE] Motor is currently STOPPED. This speed will apply on next run command.");
//     }
//   }
//   // سرعة قصوى 100% بحرف M
//   else if (command == 'M' || command == 'm') {
//     motorSpeed = 255;
//     Serial.println("-----------------------------------------");
//     Serial.println(">>> [SPEED UPDATE] Target Speed Set to: 100% (MAXIMUM) | PWM Value: 255");
//     Serial.println("-----------------------------------------");
//     if (isMotorRunning) {
//       ledcWrite(pinENA, motorSpeed);
//     }
//   }
//   // ثانياً: أوامر الاتجاه والحركة
//   else if (command == 'F' || command == 'f') {
//     lastDirection = 'F';
//     isMotorRunning = true;
//     Serial.println("[EXECUTION] Driving IN1=HIGH, IN2=LOW");
//     digitalWrite(pinIN1, HIGH);
//     digitalWrite(pinIN2, LOW);
//     ledcWrite(pinENA, motorSpeed);
    
//     Serial.println(">>> [SUCCESS] Motor is driving FORWARD <<<");
//   } 
//   else if (command == 'B' || command == 'b') {
//     lastDirection = 'B';
//     isMotorRunning = true;
//     Serial.println("[EXECUTION] Driving IN1=LOW, IN2=HIGH");
//     digitalWrite(pinIN1, LOW);
//     digitalWrite(pinIN2, HIGH);
//     ledcWrite(pinENA, motorSpeed);
    
//     Serial.println(">>> [SUCCESS] Motor is driving BACKWARD <<<");
//   } 
//   else if (command == 'S' || command == 's') {
//     isMotorRunning = false;
//     Serial.println("[EXECUTION] Driving IN1=LOW, IN2=LOW");
//     digitalWrite(pinIN1, LOW);
//     digitalWrite(pinIN2, LOW);
//     ledcWrite(pinENA, 0);
    
//     Serial.println(">>> [SUCCESS] Motor STOPPED successfully <<<");
//   }
// }

// // #include "BluetoothSerial.h"

// // // التحقق من تفعيل خاصية البلوتوث
// // #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
// // #error Bluetooth is not enabled! Please run `make menuconfig` to enable it.
// // #endif

// // // إنشاء كائن البلوتوث السيريال
// // BluetoothSerial SerialBT;

// // // تعريف دبابيس التحكم بدرايفر L298N على الـ ESP32
// // const int pinENA = 12; // دبوس التحكم بالسرعة (PWM)
// // const int pinIN1 = 13; // دبوس الاتجاه الأول
// // const int pinIN2 = 14; // دبوس الاتجاه الثاني

// // // إعدادات الـ PWM الحديثة لـ ESP32 v3.x
// // const int pwmFreq = 5000;    // التردد 5 كيلو هرتز
// // const int pwmResolution = 8; // الدقة 8 بت (قيم من 0 إلى 255)

// // int motorSpeed = 0; // متغير لتخزين السرعة الحالية

// // void setup() {
// //   // فتح منفذ السيريال للمراقبة
// //   Serial.begin(115200);

// //   // ضبط دبابيس الاتجاه كمخارج
// //   pinMode(pinIN1, OUTPUT);
// //   pinMode(pinIN2, OUTPUT);

// //   // --- النظام الحديث للـ PWM في الـ ESP32 ---
// //   // نقوم بربط الدبوس والتردد والدقة مباشرة في سطر واحد بدون الحاجة لقنوات (Channels)
// //   ledcAttach(pinENA, pwmFreq, pwmResolution);

// //   // إيقاف المحرك عند بداية التشغيل كإجراء أمان
// //   stopMotor();

// //   // تشغيل البلوتوث المدمج
// //   SerialBT.begin("ESP32_Motor_Controller"); 
// //   Serial.println("The device started, now you can pair it with bluetooth!");
// // }

// // void loop() {
// //   // التحقق من وجود بيانات قادمة لاسلكياً من تطبيق الهاتف
// //   if (SerialBT.available()) {
// //     char command = SerialBT.read(); // قراءة الحرف المرسل
// //     Serial.print("Received Command: ");
// //     Serial.println(command); 
    
// //     // معالجة أوامر الاتجاه
// //     if (command == 'F') {       // Forward
// //       moveForward();
// //     } 
// //     else if (command == 'B') {  // Backward
// //       moveBackward();
// //     } 
// //     else if (command == 'S') {  // Stop
// //       stopMotor();
// //     }
// //     // معالجة أوامر السرعة
// //     else if (command >= '0' && command <= '9') {
// //       int speedPercent = (command - '0') * 10; 
// //       motorSpeed = map(speedPercent, 0, 100, 0, 255);
      
// //       // --- النظام الحديث للكتابة على الـ PWM ---
// //       // نكتب مباشرة على الدبوس بدلاً من رقم القناة
// //       ledcWrite(pinENA, motorSpeed); 
// //     }
// //   }
// //   delay(20); 
// // }

// // void moveForward() {
// //   digitalWrite(pinIN1, HIGH);
// //   digitalWrite(pinIN2, LOW);
// //   ledcWrite(pinENA, motorSpeed); // تعديل هنا
// // }

// // void moveBackward() {
// //   digitalWrite(pinIN1, LOW);
// //   digitalWrite(pinIN2, HIGH);
// //   ledcWrite(pinENA, motorSpeed); // تعديل هنا
// // }

// // void stopMotor() {
// //   digitalWrite(pinIN1, LOW);
// //   digitalWrite(pinIN2, LOW);
// //   ledcWrite(pinENA, 0); // تعديل هنا
// // }
// #include "BluetoothSerial.h"

// #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
// #error Bluetooth is not enabled! Please run `make menuconfig` to enable it.
// #endif

// BluetoothSerial SerialBT;

// // تعريف دبابيس التحكم بدرايفر L298N
// const int pinENA = 12; 
// const int pinIN1 = 13; 
// const int pinIN2 = 14; 

// const int pwmFreq = 5000;    
// const int pwmResolution = 8; 
// int motorSpeed = 220; // سرعة قوية (من 0 إلى 255) لضمان قومة المحرك

// void setup() {
//   Serial.begin(115200);
//   delay(1000);
//   Serial.println("\n=========================================");
//   Serial.println("[SYSTEM START] Diagnosing Motor Driver Code...");

//   // ضبط أقطاب التحكم كمخارج (تم تصحيح السطر هنا)
//   pinMode(pinIN1, OUTPUT);
//   pinMode(pinIN2, OUTPUT);
  
//   digitalWrite(pinIN1, LOW);
//   digitalWrite(pinIN2, LOW);

//   ledcAttach(pinENA, pwmFreq, pwmResolution);
//   ledcWrite(pinENA, 0);

//   SerialBT.begin("ESP32_Motor_Controller"); 
//   Serial.println("[INIT] Bluetooth 'ESP32_Motor_Controller' is Online.");
//   Serial.println("[HELP] You can type commands in this Serial Monitor:");
//   Serial.println("       Type 'F' and press Enter -> Move Forward");
//   Serial.println("       Type 'B' and press Enter -> Move Backward");
//   Serial.println("       Type 'S' and press Enter -> Stop Motor");
//   Serial.println("=========================================\n");
// }

// void loop() {
//   // 1. استقبال الأوامر من كتابة السيريال مونيتور (الكمبيوتر)
//   if (Serial.available()) {
//     char serialCmd = Serial.read();
//     // تجاهل الفراغات والسطر الجديد
//     if (serialCmd != '\n' && serialCmd != '\r') {
//       Serial.print("[SERIAL INPUT] Detected Character: '");
//       Serial.print(serialCmd);
//       Serial.println("'");
//       processCommand(serialCmd);
//     }
//   }

//   // 2. استقبال الأوامر من الهاتف (البلوتوث)
//   if (SerialBT.available()) {
//     char btCmd = SerialBT.read();
//     Serial.print("[BLUETOOTH INPUT] Detected Character: '");
//     Serial.print(btCmd);
//     Serial.println("'");
//     processCommand(btCmd);
//   }
//   delay(10);
// }

// // دالة تنفيذ الأوامر والتحقق من النجاح
// void processCommand(char command) {
//   if (command == 'F' || command == 'f') {
//     Serial.println("[EXECUTION] Sending Signals: IN1=HIGH, IN2=LOW");
//     digitalWrite(pinIN1, HIGH);
//     digitalWrite(pinIN2, LOW);
//     ledcWrite(pinENA, motorSpeed);
    
//     // رسالة تأكيد النجاح البرمجي
//     Serial.println("-----------------------------------------");
//     Serial.print(">>> [SUCCESS] Motor turning FORWARD successfully! <<<");
//     Serial.print(" (PWM Speed: "); Serial.print(motorSpeed); Serial.println(")");
//     Serial.println("-----------------------------------------");
//   } 
//   else if (command == 'B' || command == 'b') {
//     Serial.println("[EXECUTION] Sending Signals: IN1=LOW, IN2=HIGH");
//     digitalWrite(pinIN1, LOW);
//     digitalWrite(pinIN2, HIGH);
//     ledcWrite(pinENA, motorSpeed);

//     Serial.println("-----------------------------------------");
//     Serial.print(">>> [SUCCESS] Motor turning BACKWARD successfully! <<<");
//     Serial.print(" (PWM Speed: "); Serial.print(motorSpeed); Serial.println(")");
//     Serial.println("-----------------------------------------");
//   } 
//   else if (command == 'S' || command == 's') {
//     Serial.println("[EXECUTION] Sending Signals: IN1=LOW, IN2=LOW");
//     digitalWrite(pinIN1, LOW);
//     digitalWrite(pinIN2, LOW);
//     ledcWrite(pinENA, 0);
//     Serial.println(">>> [SUCCESS] Motor STOPPED successfully! <<<");
//   }
// }