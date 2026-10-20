#include "Max30003.h"
#include "SPI.h"
#define MAX30003_SPI_SPEED 2000000

SPISettings SPI_SETTINGS(MAX30003_SPI_SPEED, MSBFIRST, SPI_MODE0);

MAX30003::MAX30003(int cs_pin) {
  _cs_pin = cs_pin;
  pinMode(_cs_pin, OUTPUT);
  digitalWrite(_cs_pin, HIGH);
}

void MAX30003::max30003RegWrite(unsigned char WRITE_ADDRESS, unsigned long data) {
  // now combine the register address and the command into one byte:
  byte dataToSend = (WRITE_ADDRESS << 1) | WREG;

  SPI.beginTransaction(SPI_SETTINGS);
  // take the chip select low to select the device:
  digitalWrite(_cs_pin, LOW);

  delay(2);
  SPI.transfer(dataToSend);
  SPI.transfer(data >> 16);
  SPI.transfer(data >> 8);
  SPI.transfer(data);
  delay(2);

  // take the chip select high to de-select:
  digitalWrite(_cs_pin, HIGH);
  SPI.endTransaction();
}


void MAX30003::max30003SwReset(void) {
  max30003RegWrite(SW_RST, 0x000000);
  delay(100);
}

void MAX30003::max30003Synch(void) {
  max30003RegWrite(SYNCH, 0x000000);
}

void MAX30003::max30003RegRead(uint8_t Reg_address, uint8_t* buff) {
  uint8_t spiTxBuff;

  SPI.beginTransaction(SPI_SETTINGS);
  digitalWrite(_cs_pin, LOW);

  spiTxBuff = (Reg_address << 1) | RREG;
  SPI.transfer(spiTxBuff);  //Send register location

  for (int i = 0; i < 3; i++) {
    buff[i] = SPI.transfer(0xff);
  }

  digitalWrite(_cs_pin, HIGH);
  SPI.endTransaction();
}


bool MAX30003::max30003ReadInfo (void) {// Vô hiệu hóa tín hiệu hiệu chuẩn để đo ECG thực tế
  delay(100);
 
  uint8_t spiTxBuff;
  uint8_t readBuff[4];

  SPI.beginTransaction(SPI_SETTINGS);
  digitalWrite(_cs_pin, LOW);

  spiTxBuff = (INFO << 1) | RREG;
  SPI.transfer(spiTxBuff);  //Send register location

  for (int i = 0; i < 3; i++) {
    readBuff[i] = SPI.transfer(0xff);
  }

  digitalWrite(_cs_pin, HIGH);
  SPI.endTransaction();

  if ((readBuff[0] & 0xf0) == 0x50) {

    Serial.println("max30003 is ready");
    Serial.print("Rev ID:  ");
    Serial.println((readBuff[0] & 0xf0));

    return true;
  } else {

    Serial.println("max30003 read info error\n");
    return false;
  }

  return false;
}

void MAX30003::max30003ReadData(int num_samples, uint8_t* readBuffer) {
  uint8_t spiTxBuff;

  SPI.beginTransaction(SPI_SETTINGS);
  digitalWrite(_cs_pin, LOW);

  spiTxBuff = (ECG_FIFO_BURST << 1) | RREG;
  SPI.transfer(spiTxBuff);  //Send register location

  for (int i = 0; i < num_samples * 3; ++i) {
    readBuffer[i] = SPI.transfer(0x00);
  }

  digitalWrite(_cs_pin, HIGH);
  SPI.endTransaction();
}

void MAX30003::max30003Begin() {
  max30003SwReset();  // Khởi động lại thiết bị
  delay(100);
  
  // Cấu hình thanh ghi CNFG_GEN
  // Bật chức năng phát hiện lead-on và cài đặt ngưỡng phát hiện điện cực 0x802CB 0x8141B 0x8151B 0x8155B
  max30003RegWrite(CNFG_GEN, 0x8051B);  // Dòng điện DC Lead-Off: 100nA, ngưỡng phát hiện: VMID ± 300mV Bia 200M
  delay(100);

  // Cấu hình hiệu chuẩn (Calibration) CNFG_CAL
  // Cài đặt tín hiệu chuẩn để kiểm tra đường tín hiệu
  max30003RegWrite(CNFG_CAL, 0x700800);        //   Calibration Source Magnitude Selection (VMAG)  = 0.50mV  || Calibration Source Frequency 256Hz
  // Cấu hình MUX đầu vào CNFG_EMUX 0x304800 0x700800
  // Kết nối đường tín hiệu ECG vào ngõ vào MUX
  delay(100);
  max30003RegWrite(CNFG_EMUX, 0x0B0000);  // Chọn ngõ vào ECGP và ECGN
  delay(100);

  // Cấu hình ECG CNFG_ECG 
  // Cài đặt tốc độ lấy mẫu và các bộ lọc để tối ưu hóa chất lượng tín hiệu 0x805000 , 837000 512 Hz, GAIN=160, DLPF=150 Hz , 0x836002 512 Hz, GAIN=160, DLPF = 40 Hz
  max30003RegWrite(CNFG_ECG, 0x815000); //
  delay(100);
  // 512Hz, Gain=40, DLPF=40Hz, DHPF=0.5Hz //0x815000
  // Cấu hình CNFG_RTOR1
  max30003RegWrite(CNFG_RTOR1, 0x3fc600);  //0x5FC600 0x3fc600 0x3FA300
  delay(100);
  // Đồng bộ hóa cài đặt
  max30003Synch();
  delay(100);

}


void MAX30003::max30003BeginRtorMode() {
  max30003SwReset();
  delay(100);
  max30003RegWrite(CNFG_GEN, 0x080004);
  delay(100);
  max30003RegWrite(CNFG_CAL, 0x720000);  // 0x700000
  delay(100);
  max30003RegWrite(CNFG_EMUX, 0x0B0000);
  delay(100);
  max30003RegWrite(CNFG_ECG, 0x800400);  // d23 - d22 : 10 for 250sps , 00:500 sps
  delay(100);
  max30003RegWrite(CNFG_RTOR1, 0x3fc600);
  delay(100);
  max30003RegWrite(EN_INT, 0x000401);
  delay(100);
  max30003Synch();
  delay(100);
}

//not tested
void MAX30003::max30003SetsamplingRate(uint16_t samplingRate) {
  uint8_t regBuff[4] = { 0 };
  max30003RegRead(CNFG_ECG, regBuff);

  switch (samplingRate) {
    case SAMPLINGRATE_128:
      regBuff[0] = (regBuff[0] | 0x80);
      break;

    case SAMPLINGRATE_256:
      regBuff[0] = (regBuff[0] | 0x40);
      break;

    case SAMPLINGRATE_512:
      regBuff[0] = (regBuff[0] | 0x00);
      break;

    default:
      Serial.println("Wrong samplingRate, please choose between 128, 256 or 512");
      break;
  }

  unsigned long cnfgEcg;
  memcpy(&cnfgEcg, regBuff, 4);

  Serial.print(" cnfg ECG ");
  Serial.println((cnfgEcg));
  max30003RegWrite(CNFG_ECG, (cnfgEcg >> 8));
}

void MAX30003::getEcgSamples(void) {
  uint8_t regReadBuff[4];
  max30003RegRead(ECG_FIFO, regReadBuff);

  unsigned long data0 = (unsigned long)(regReadBuff[0]);
  data0 = data0 << 24;
  unsigned long data1 = (unsigned long)(regReadBuff[1]);
  data1 = data1 << 16;
  unsigned long data2 = (unsigned long)(regReadBuff[2]);
  data2 = data2 >> 6;
  data2 = data2 & 0x03;

  unsigned long data = (unsigned long)(data0 | data1 | data2);
  ecgdata = (signed long)(data);
#ifndef debug
  Serial.println(ecgdata);
#endif 
}


void MAX30003::getHRandRR(void) {
  uint8_t regReadBuff[4];
  max30003RegRead(RTOR, regReadBuff);

  unsigned long RTOR_msb = (unsigned long)(regReadBuff[0]);
  unsigned char RTOR_lsb = (unsigned char)(regReadBuff[1]);
  unsigned long rtor = (RTOR_msb << 8 | RTOR_lsb);
  rtor = ((rtor >> 2) & 0x3fff);

  float hr = 60 / ((float)rtor * 0.0078125);
  heartRate = (unsigned int)hr;

  unsigned int RR = (unsigned int)rtor * (7.8125);  //8ms
  RRinterval = RR;
// #ifndef DEBUG
//  Serial.println(heartRate);
// #endif
}

void MAX30003::setECGGain(uint8_t gainSetting) {
  uint8_t regBuff[4] = { 0 };
  max30003RegRead(CNFG_ECG, regBuff);

  // Xóa các bit hiện tại cho gain (bit 6-5)
  regBuff[0] &= ~(0x60);

  // Đặt lại giá trị gain mới
  switch (gainSetting) {
    case 1:  // Gain = 20
      regBuff[0] |= (0x00);
      break;
    case 2:  // Gain = 40
      regBuff[0] |= (0x20);
      break;
    case 3:  // Gain = 80
      regBuff[0] |= (0x40);
      break;
    case 4:  // Gain = 160
      regBuff[0] |= (0x60);
      break;
    default:
      regBuff[0] |= (0x00);  // Mặc định gain = 20
  }

  unsigned long cnfgEcg;
  memcpy(&cnfgEcg, regBuff, 4);

  // Ghi lại giá trị mới vào thanh ghi CNFG_ECG
  max30003RegWrite(CNFG_ECG, (cnfgEcg >> 8));
}
