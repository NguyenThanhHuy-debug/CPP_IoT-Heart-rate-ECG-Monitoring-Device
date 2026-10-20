// #include "MyWiFi.h"    // Sử dụng file tiêu đề đã sửa
// #include <WiFi.h>      // Thư viện WiFi của ESP32
#include "ChuongTrinhChinh.h"
#include <SPI.h>       // Thư viện SPI
#include "Max30003.h"  // Thư viện MAX30003


#define CS_PIN_MAX30003 10  // Chân CS của MAX30003
#define CS_PIN_ADXL345 3
#define SO 5
#define SI 7
#define SCK 6
#define INT 8
int movingAverageBuffer[5] = {0};
int bufferIndex = 0;
int applyMovingAverage(int rawSample);

MAX30003 max30003(CS_PIN_MAX30003);  // Khởi tạo đối tượng từ lớp MAX30003 với chân CS
// WIFI wifi;                           // Khởi tạo đối tượng từ lớp WIFI
// ADXL345 adxl345(CS_PIN_ADXL345);

void KhoiTao() {
  Serial.begin(115200);


  // // Kết nối WiFi
  // wifi.KetNoiWiFi(30);  // Kết nối WiFi trong vòng 30 giây

  // int signalStrength = wifi.TinhDoManhCuaWiFi();
  // if (signalStrength != -1) {
  //   Serial.print("Cường độ tín hiệu: ");
  //   Serial.println(signalStrength);
  // } else {
  //   Serial.println("Chưa kết nối WiFi");
  // }
  // Cấu hình chân CS cho MAX30003
  pinMode(CS_PIN_MAX30003, OUTPUT);
  digitalWrite(CS_PIN_MAX30003, HIGH);  // Vô hiệu hóa MAX30003 lúc đầu

  // Khởi tạo giao tiếp SPI
  SPI.begin(SCK, SO, SI, CS_PIN_MAX30003);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  // Đọc thông tin từ MAX30003 để kiểm tra kết nối
  bool ret = max30003.max30003ReadInfo();

  if (ret) {
#ifndef debug
    Serial.println("Max30003 đọc ID thành công");
#endif
  } else {
    // Nếu không đọc được ID, vòng lặp kiểm tra liên tục với timeout 10 giây
    unsigned long startMillis = millis();
    while (!ret) {
      ret = max30003.max30003ReadInfo();
      if (millis() - startMillis > 10000) {  // Timeout sau 10 giây
#ifndef debug
        Serial.println("Max30003 đọc ID thất bại, timeout sau 10 giây");
#endif
        break;
      }
#ifndef debug
      Serial.println("Không đọc được ID, kiểm tra kết nối...");
#endif
      delay(1000);
    }
  }
#ifndef debug
  // Khởi tạo MAX30003
  Serial.println("Khởi tạo MAX30003...");
#endif
  max30003.max30003Begin();
  // max30003.max30003SetsamplingRate(SAMPLINGRATE_512);

  // max30003.enableNotchFilter(true);
}

void ChayChuongTrinhChinh() {

  max30003.getEcgSamples();

#ifndef debug
  Serial.println(max30003.ecgdata);
#endif
  delay(21);
}

