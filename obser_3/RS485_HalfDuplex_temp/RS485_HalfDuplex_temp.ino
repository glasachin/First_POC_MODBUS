#include <ModbusMaster.h>

#define MAX485_DE      5
#define MAX485_RE_NEG  4

// instantiate ModbusMaster object
ModbusMaster node;

void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

void setup()
{
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  // Init in receive mode
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  Serial.begin(115200);
  // Modbus communication runs at 115200 baud
  Serial2.begin(115200);

  // Modbus slave ID 1
  node.begin(1, Serial2);
  // Callbacks allow us to configure the RS485 transceiver correctly
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}

bool state = true;

void loop()
{
  uint8_t result;
  uint16_t data[6];
  
  // Toggle the coil at address 0x0002 (Manual Load Control)
  result = node.writeSingleCoil(0x0002, state);
  state = !state;

  // Read 5 registers starting at 0x30001)
  result = node.readInputRegisters(0x0000, 5);
  if (result == node.ku8MBSuccess)
  {
    char temp = 0x00;
    for(int i = 0; i < 5; i++)
    {
      Serial.print("Register ");Serial.print(i);Serial.print(" : ");
      Serial.println(node.getResponseBuffer(temp));
      temp+= 0x01;
    }
//    Serial.println(node.getResponseBuffer(0x00)/100.0f);
//    Serial.println(node.getResponseBuffer(0x01)/100.0f);
//    Serial.print("Pload: ");
//    Serial.println((node.getResponseBuffer(0x0D) +
//                    node.getResponseBuffer(0x0E) << 16)/100.0f);
  }
  else
  {
    Serial.println("Not working");
  }

  delay(1000);
}
