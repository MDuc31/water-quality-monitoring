#include <SPI.h>
#include <LoRa.h>
#include <HardwareSerial.h>

#define SCK     PA5
#define MISO    PA6
#define MOSI    PA7
#define SS      PA4
#define RST     PB0
#define DIO0    PB1

#define BUTTON1_PIN PB6 // Chân nút nhấn 1
#define BUTTON2_PIN PB7 // Chân nút nhấn 2

#define STM32_TX PA2
#define STM32_RX PA3
HardwareSerial MySerial(USART2); // Khai báo UART1 cho STM32

byte localAddress = 0xBB;  // Địa chỉ của thiết bị gửi
byte destination = 0xFF;   // Địa chỉ của thiết bị nhận
byte msgCount = 0;         // Đếm số tin nhắn gửi đi
int s_node2 = 0;
String outgoing;              // tin nhắn gửi đi
String mymessage = "";
bool lastButton1State = HIGH; // Trạng thái trước đó của nút nhấn
bool lastButton2State = HIGH; // Trạng thái trước đó của nút nhấn

void sendMessage(String outgoing) {
    LoRa.beginPacket();
    LoRa.write(destination);  // Địa chỉ nhận
    LoRa.write(localAddress); // Địa chỉ gửi
    LoRa.write(msgCount);     // ID tin nhắn
    LoRa.write(outgoing.length());  // Độ dài tin nhắn
    LoRa.print(outgoing); // Nội dung tin nhắn
    LoRa.endPacket();

    Serial.println("Sent: " + outgoing);
    msgCount++; 
}

void onReceive(int packetSize) {
    if (packetSize == 0) return;

    byte recipient = LoRa.read();
    byte sender = LoRa.read();
    byte incomingMsgId = LoRa.read();
    byte incomingLength = LoRa.read();

    String incoming = "";
    while (LoRa.available()) {
        incoming += (char)LoRa.read();
    }

    if (incomingLength != incoming.length()) {
        Serial.println("Lỗi: Độ dài tin nhắn không khớp!");
        return;
    }

    if (recipient != localAddress && recipient != 0xFF) {
        Serial.println("Tin nhắn này không dành cho tôi.");
        return;
    }


    Serial.println("Dữ liệu nhận từ " + String(sender, HEX) + ": " + incoming);
    String temp = getValue(incoming, ',', 0);
    String tds = getValue(incoming, ',', 1);
    String tur = getValue(incoming, ',',2 );
    String ph = getValue(incoming, ',',3 );
    Serial.println(temp);
    Serial.println(tds);
    Serial.println(tur);

    MySerial.println(incoming); // Gửi dữ liệu qua UART đến ESP32

}
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  if (found > index) {
    return data.substring(strIndex[0], strIndex[1]);
  } else {
    return "";
  }
}

void setup() {
    Serial.begin(9600);
    MySerial.begin(9600);

    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);
    
    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("LoRa init failed!");
        while (1);
    }

    Serial.println("LoRa Sender Ready");
}

void loop() {
    bool button1State = digitalRead(BUTTON1_PIN);
    bool button2State = digitalRead(BUTTON2_PIN);

    if (button1State == LOW && lastButton1State == HIGH) {
        delay(50); // Chống dội phím
        sendMessage("ON1");
    }
    if (button2State == LOW && lastButton2State == HIGH) {
        delay(50); // Chống dội phím
        sendMessage("ON2");
    } 

    lastButton1State = button1State;
    lastButton2State = button1State;

    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        onReceive(packetSize);
    }
}
