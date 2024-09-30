#include "WiFi.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"
#include "SoftwareSerial.h"
#include "string"
#include "stdint.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

typedef struct point_t {
  double longitude;  // 经度, E为正W为负
  double latitude;   // 纬度, N为正S为负
} Point;

char local_response_buffer[1024];
const char* ssid = "oplus_co_apvqii";
const char* password = "23459876";
const char* hw_key = "your_hwcloud_functiongraph_key";

hw_timer_t* htim = NULL;  // 检测wifi连接
bool wifi_connected = false;

// 软件串口
SoftwareSerial uart(19, 21);  // rx, tx
uint8_t rxbuf[128];
uint8_t rxlen = 0;
Point rx_ori_pos = {0};

// 储存json
DynamicJsonDocument doc(16384);

uint8_t AccessUrl(Point ori_pos, uint16_t* dst_name, uint16_t* city) {
  // 拼接url
  // char url[512] = "https://e953bf342ddc4f2f8129a6177546e527.apig.cn-north-4.huaweicloudapis.com/user/getroute?origin_location=113.965307,22.581946&destination=%E6%B7%B1%E5%9C%B3%E5%8C%97%E7%AB%99";
  char url[512] = "";
  sprintf(url, "https://%s.apig.cn-north-4.huaweicloudapis.com/user/getroute?origin_location=%f,%f&destination=",
          hw_key, ori_pos.longitude, ori_pos.latitude);
  for (int i = 0; i < 30; i++) {
    if (dst_name[i] < 255) {  // ascii字符
      url[strlen(url)] = (uint8_t)dst_name[i];
    } else {  // 汉字转换成3个u8的形式, 如"深圳北站" 6df1 5733 5317 7ad9->%e6%b7%b1%e5%9c%b3%e5%8c%97%e7%ab%99
      char temp[10];
      uint8_t b[3] = {
        0xe0 | ((dst_name[i] & 0xf000) >> 12),
        0x80 | ((dst_name[i] & 0xfc0) >> 6),
        0x80 | (dst_name[i] & 0x3f)
      };
      sprintf(temp, "%%%2x%%%2x%%%2x", b[0], b[1], b[2]);
      strcat(url, temp);
    }
  }

  strcat(url, "&city=");
  for (int i = 0; i < 10; i++) {
    if (city[i] < 255) {  // ascii字符
      url[strlen(url)] = (uint8_t)city[i];
    } else {  // 汉字转换成3个u8的形式, 如"深圳北站" 6df1 5733 5317 7ad9->%e6%b7%b1%e5%9c%b3%e5%8c%97%e7%ab%99
      char temp[10];
      uint8_t b[3] = {
        0xe0 | ((city[i] & 0xf000) >> 12),
        0x80 | ((city[i] & 0xfc0) >> 6),
        0x80 | (city[i] & 0x3f)
      };
      sprintf(temp, "%%%2x%%%2x%%%2x", b[0], b[1], b[2]);
      strcat(url, temp);
    }
  }
  Serial.println(url);

  // 发送HTTP GET请求
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  uint8_t success = 0;
  if (httpCode > 0) {                   // 检查响应码
    String payload = http.getString();  // 获取响应的内容
    Serial.println("Response:");
    Serial.println(payload);

    // 解析json
    deserializeJson(doc, payload);
    success = 1;
  } else {
    Serial.printf("Error on HTTP request: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();  // 关闭HTTP连接
  return success;
}

// 中断内不能写阻塞操作(例如print), 因此需要设置flag后, 在loop中处理
void WifiTimInterrupt(void){  // 定时器中断
  if(!wifi_connected) return;
  if (WiFi.status() != WL_CONNECTED) {
    wifi_connected = false;
    digitalWrite(26, LOW);
  } else {
    digitalWrite(26, HIGH);
  }
}

void setup() {
  Serial.begin(115200);
  uart.begin(9600, SWSERIAL_8N1, 19, 21, false, 256);

  pinMode(26, OUTPUT);  // wifi led
  digitalWrite(26, LOW);

  htim = timerBegin(1, 39999, true);  // 主频40MHz, 时钟1kHz
  timerAttachInterrupt(htim, WifiTimInterrupt, true);
  timerAlarmWrite(htim, 20000, true);  //2s进一次中断
  timerAlarmEnable(htim);

  WiFi.begin(ssid, password);

  // 等待连接到Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //   Point origin = {113.965307,22.581946};
  // uint16_t name[30] = {0x6df1, 0x5733, 0x5317, 0x7ad9};
  // AccessUrl(origin, name);

}

void loop() {
  // put your main code here, to run repeatedly:
  if(!wifi_connected){
    // 重新连接 WiFi
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500); // 等待500毫秒
    }
    wifi_connected = true;
    digitalWrite(26, HIGH);
  }

  if(uart.available()){
    rxbuf[rxlen] = uart.read();  // 储存字符在缓冲区末尾
    // Serial.println(rxlen);
    // Serial.println(rxbuf[rxlen]);
    if(rxbuf[rxlen] == 0x04){  // 传输完成标志位
      // 判断包是否完整
      if(rxbuf[0] != 0x02){  // 起始字符
        rxlen = 0;
        memset(rxbuf, 0, 128);
        return;
      }
      if(rxlen >= rxbuf[2] + 3){  // 接收对应长度数据(datalen + 4 - 1)
        // 处理数据
        if(rxbuf[1] == 0x11){  // GPS定位信息
          memcpy((uint8_t*)&rx_ori_pos, rxbuf + 3, 16);
        } else if(rxbuf[1] == 0x12){  // 目的地名称
          // 访问url
          uint16_t rx_dst_name[30];
          memcpy((uint8_t*)rx_dst_name, rxbuf + 3, 60);
          uint16_t city[10];
          memcpy((uint8_t*)city, rxbuf + 63, 20);
          while(!AccessUrl(rx_ori_pos, rx_dst_name, city));
          if(doc["status"] == 0){  // 规划成功
            // 发送路径参数
            uint8_t txbuf[35] = {0x02, 0x91, 31};
            uint32_t distance = doc["distance"].as<uint32_t>();
            memcpy(txbuf + 3, (uint8_t*)&distance, 4);
            uint16_t duration = doc["duration"].as<uint16_t>();
            memcpy(txbuf + 7, (uint8_t*)&duration, 2);
            uint8_t count = doc["count"].as<uint8_t>();
            memcpy(txbuf + 9, (uint8_t*)&count, 1);
            Point center = {
              doc["center"][0].as<uint32_t>() / 1e6,
              doc["center"][1].as<uint32_t>() / 1e6};
            memcpy(txbuf + 10, (uint8_t*)&center, 16);
            // double halfside = atof(doc["halfside"].as<char*>());
            double halfside = doc["halfside"].as<uint32_t>() / 1e6;
            memcpy(txbuf + 26, (uint8_t*)&halfside, 8);
            txbuf[34] = 0x04;
            uart.write(txbuf, 35);
            delay(20);

            // 发送路径点
            for(int i = 0; i < (count + 4) / 5; i++){
              // 包序号i对应路径点5i, 5i+1, ..., 5i+4
              uint8_t txbuf_route[85] = {0x02, 0x92, 81};
              txbuf_route[3] = i;
              for(int j = 0; j < 5; j++){
                if(5 * i + j < count){
                  Point routepoint = {
                    doc["route"][5 * i + j][0].as<uint32_t>() / 1e6,
                    doc["route"][5 * i + j][1].as<uint32_t>() / 1e6
                  };
                  memcpy(txbuf_route + 4 + 16 * j, (uint8_t*)&routepoint, 16);
                }
              }
              txbuf_route[84] = 0x04;
              // if(i == 0){
              //   for(int k = 0; k < 85; k++){
              //       Serial.println(k);
              //       Serial.println(txbuf_route[k]);
              //   }
              // }
              uart.write(txbuf_route, 85);
              delay(20);
            }
          } else {
            // 1: 未搜索到地点  2: 无法规划路径
              uint8_t txbuf_route[5] = {0x02, 0x93, 1};
              txbuf_route[3] = doc["status"];
              txbuf_route[4] = 0x04;
              uart.write(txbuf_route, 5);
          }
        }
        // 清空缓冲区
        rxlen = 0;
        memset(rxbuf, 0, 128);
        return;
      }
    } else rxlen++;
  }
}
