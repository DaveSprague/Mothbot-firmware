#include <LowPower.h>

#include <SPI.h>
#include <Wire.h>
#include <RH_RF95.h>
#include "lcdgfx.h"
#include <SoftwareSerial.h>



SoftwareSerial Serial1(3, 4); // RX, TX

DisplaySSD1306_128x64_I2C display(-1); // This line is suitable for most platforms by default

// radio pins for mothbot
#define RFM95_CS 8
#define RFM95_RST 7
#define RFM95_INT 2

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 434.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

#define LED 14

void types(String a) { Serial.println("it's a String"); }
void types(int a) { Serial.println("it's an int"); }
void types(char *a) { Serial.println("it's a char*"); }
void types(float a) { Serial.println("it's a float"); }
void types(bool a) { Serial.println("it's a bool"); }

void setup()
{
  pinMode(LED, OUTPUT);

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115600);
  //  while (!Serial) {
  //    delay(1);
  //  }
  Serial1.begin(9600);
  delay(100);
  display.setFixedFont( ssd1306xled_font6x8 );
  display.begin();
  display.fill(0x00);
  display.setFixedFont(ssd1306xled_font6x8);
  display.printFixed (0,  8, "Starting...", STYLE_NORMAL);
  Serial.println("Starting...");
//  display.printFixed (0, 16, "Line 2. Bold text", STYLE_BOLD);
//  display.printFixed (0, 24, "Line 3. Italic text", STYLE_ITALIC);
//  display.printFixedN (0, 32, "Line 4. Double size", STYLE_BOLD, FONT_SIZE_2X);

  delay(1000);
  
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println(F("LoRa radio init failed"));
    Serial.println(F("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info"));
    while (1);
  }
  Serial.println(F("LoRa radio init OK!"));

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println(F("setFrequency failed"));
    while (1);
  }
  Serial.print(F("Set Freq to: ")); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(5, false);
}

int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop()
{
  //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, 
  //        SPI_OFF, USART0_OFF, TWI_OFF);
   // LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
  int start_time = millis();
  // read battery voltage and convert to 10ths of volts
  int voltsX10 = round(10* 2 * 3.3 *analogRead(A3)/1023);
  voltsX10=25; //  *** REMOVE THIS WHEN BATTERY USED ***
  // read temp/humid/press

  int countBuffer = 0;
  // Empty the software serial buffer
  while (Serial1.available()) {
    Serial1.read();
    countBuffer++;
  }

  Serial.print(F("Cleared Buffer: "));
  Serial.println(countBuffer);
  
  // wait for an R
  while (!Serial1.available() || Serial1.read() != 'R')
  {
    //Wait for an 'R' that significes the start of a message
    //  from the Maxbotix
  }
  //Read the distance measurement
  char buff[5]="    ";
  int count = 0;
  while (count < 4) {
    char c = Serial1.read();
    if (int(c) == -1) {
      continue;
    }else {
      buff[count] = c;
      count++;
    }   
  }
  buff[4] = 0;
  display.fill(0x00);
  digitalWrite(LED, HIGH);
  delay(5);
  digitalWrite(LED, LOW);
  char radiopacket[25] = "D";
  //itoa(packetnum, radiopacket + 13, 10);
  strcat(radiopacket, buff);
  strcat(radiopacket, "mm");
  char voltStr[3];
  itoa(voltsX10, voltStr, 10);
  strcat(radiopacket, voltStr);
  strcat(radiopacket, "v");
  char seq[2];
  itoa(packetnum%10, seq, 10);
  strcat(radiopacket, seq);
  packetnum++;
  Serial.print(F("Sending Msg: "));
  Serial.println(radiopacket);
  delay(10);
  display.printFixed (0,  0, "Sending...", STYLE_NORMAL);
  display.printFixed (0, 8, radiopacket, STYLE_NORMAL);
  
  radiopacket[19] = 0;
  rf95.send((uint8_t *)radiopacket, 20);

  delay(10);
  rf95.waitPacketSent();
  

  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.waitAvailableTimeout(1000))
  {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len))
    {
      // delay(500);
      digitalWrite(LED, HIGH);
      delay(5);
      digitalWrite(LED, LOW);
      display.printFixed (0,  24, "Reply:", STYLE_NORMAL);
      display.printFixed (0, 36, (char*)buf, STYLE_NORMAL);
      display.printFixed (0, 48, "RSSI:", STYLE_NORMAL);
      char rs[5];
      itoa(rf95.lastRssi(), rs,10);
      display.printFixed (40, 48, rs, STYLE_NORMAL);
      digitalWrite(LED, LOW);
    }
    else
    {
      Serial.println(F("Receive failed"));
    }
  }
  else
  {
    Serial.println(F("No reply, is there a listener around?"));
    display.printFixed (0, 24, "No Reply", STYLE_NORMAL);
    display.printFixed (0, 36, "Anyone There?", STYLE_NORMAL);
  }
  delay(1000);
  display.fill(0x00);
  delay(8000);           
  int end_time = millis();
  Serial.print(F("Loop duration: "));Serial.println(end_time - start_time);
}
