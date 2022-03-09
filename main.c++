#include <PZEM004Tv30.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define TX_01 4 //-- FASE 01
#define RX_01 5 // --

#define TX_02 13 //--- Fase 2
#define RX_02 12 // ---

/* configurações  */
const char* ssid = "meuwifi";
const char* password =  "senha";
const char* mqttServer = "192.168.1.123";
const int mqttPort = 1883;
const char* mqttUser = "user";
const char* mqttPassword = "senha";

//taxa de erros
const int ZeroErrorTolerance = 10;
const int DiscErrorTolerance = 5;

//tempo para cada envio
int TimeUpload = 10000;
int timeLoop = 100;
//calculo de payloads perdidos
int errorPayloadsRuleZero[2] = {0, 0};
int errorPayloadsRuleDisc[2] = {0, 0};


PZEM004Tv30 pzem01(RX_01, TX_01);
PZEM004Tv30 pzem02(RX_02, TX_02);
WiFiClient espClient;
PubSubClient client(espClient);

int rulePayload(
  float volt,
  float cur,
  float powe,
  float ener,
  float freq,
  float pf,
  int payloadWhich
)
{
  if (isnan(volt) and isnan(cur) and isnan(powe) and isnan(ener) and isnan(freq) and isnan(pf))
  {
    errorPayloadsRuleZero[payloadWhich] += 1;
    Serial.println(errorPayloadsRuleZero[payloadWhich]);
    return 0;//manda zeros
  }else{errorPayloadsRuleZero[payloadWhich]=0;}
  if (isnan(volt) or isnan(cur) or isnan(powe) or isnan(ener) or isnan(freq) or isnan(pf))
  {
    errorPayloadsRuleDisc[payloadWhich] += 1;
    return 1;//discarta
  }
  return 2;
}

String jsonPayload(
  float volt,
  float cur,
  float powe,
  float ener,
  float freq,
  float pf,
  String infoPzem,
  int payloadWhich)
{
  switch (rulePayload(volt, cur, powe, ener, freq, pf, payloadWhich))
  {
    case 0:
      Serial.println(" case 0");
      volt = 0;
      cur = 0;
      powe = 0;
      ener = 0;
      freq = 0;
      pf =  0;
      break;
    case 1:
      Serial.println(" case 1");
      return "disc";
      break;
    case 2:
      Serial.println(" case 2");
      break;

  }

  String payload = "{";
  payload += "\"volt" + String(infoPzem) + "\":" + String(volt) + ",";
  payload += "\"cur" + String(infoPzem) + "\":" + String(cur) + ",";
  payload += "\"powe" + String(infoPzem) + "\":" + String(powe) + ",";
  payload += "\"ener" + String(infoPzem) + "\":" + String(ener) + ",";
  payload += "\"freq" + String(infoPzem) + "\":" + String(freq) + ",";
  payload += "\"pf" + String(infoPzem) + "\":" + String(pf);
  payload += "}";
  Serial.println(payload);
  return payload;


}

void wifiConect()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Tentando conectar wifi ...");
  }
  Serial.println("Conectado ao wifi OK");

}

void ConectMQTT()
{
  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    Serial.println("Conectando MQTT...");

    if (client.connect("ESP8266F1F2", mqttUser, mqttPassword )) {

      Serial.println("Conectado");

    } else {

      Serial.print("OPS! algo deu errado ao conectar MQTT");
      Serial.print(client.state());
      delay(2000);

    }
  }
}

void uploadPayload1()
{
  int codMyPayload1 = 0;
  char attributes[100];
  String payload1 = jsonPayload(
                      pzem01.voltage(),
                      pzem01.current(),
                      pzem01.power(),
                      pzem01.energy(),
                      pzem01.frequency(),
                      pzem01.pf(),
                      "f",
                      codMyPayload1
                    );


  Serial.println(payload1);
  Serial.println(payload1.length());
  if (errorPayloadsRuleZero[codMyPayload1] == 0 or errorPayloadsRuleZero[codMyPayload1] >= ZeroErrorTolerance)
  {
    if (payload1.equals("disc")) {
      Serial.println("is NAN");
    } else
    {
      payload1.toCharArray(attributes, 100);
      if (client.publish("v1/fase", attributes)) {
        Serial.println("Payload1 Enviado");
      } else {
        Serial.println("Payload1 Falha!");
      }
    }
  } else {
    Serial.println("Regra não Aceita! 1");
  }



}

void uploadPayload2()
{
  int codMyPayload1 = 1;
  char attributes[100];
  String payload2 = jsonPayload(
                      pzem02.voltage(),
                      pzem02.current(),
                      pzem02.power(),
                      pzem02.energy(),
                      pzem02.frequency(),
                      pzem02.pf(),
                      "f",
                      codMyPayload1
                    );
  Serial.println(payload2);

  if (errorPayloadsRuleZero[codMyPayload1] == 0 or errorPayloadsRuleZero[codMyPayload1] >= ZeroErrorTolerance)
  {
    if (payload2.equals("disc") ) {
      Serial.println("is NAN");
    } else
    {
      payload2.toCharArray(attributes, 100);
      if (client.publish("v1/fase/fase", attributes)) {
        Serial.println("Payload1 Enviado");
      } else {
        Serial.println("Payload1 Falha!");
      }
    }
  } else {
    Serial.println("Regra não Aceita! 2");
  }
}

void ruleReset()
{
  for (int x = 0; x <= 1; x++)
  {
    errorPayloadsRuleZero[x] = 0;
    errorPayloadsRuleDisc[x] = 0;
  }
}

/* start */
void setup() {
  Serial.begin(115200);
  pzem01.resetEnergy();
  pzem02.resetEnergy();
  pzem01.setAddress(0x42);
  pzem02.setAddress(0x42);
  Serial.println("Etapa 1 -> WIFI");
  wifiConect();
  Serial.println("Etapa 2 -> MQTT");
  ConectMQTT();

}

void loop() {
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Estou conectado wifi ...");

    if (client.connected()) {
      client.loop();
      delay(TimeUpload);
      client.loop();
      ruleReset();

      while (true)
      { 
        client.loop();
        uploadPayload1();
        if (errorPayloadsRuleZero[0] == 0 or errorPayloadsRuleZero[0] >= ZeroErrorTolerance)
        {
          Serial.println("loop break 1");
          break;
        } else {
          Serial.println("Estou em loop1 rule");
        }
        delay(timeLoop);
      }

      while (true)
      {
        client.loop();
        uploadPayload2();
        if (errorPayloadsRuleZero[1] == 0 or errorPayloadsRuleZero[1] >= ZeroErrorTolerance)
        {
          Serial.println("loop break 2");
          break;
        } else {
          Serial.println("Estou em loop2 rule");
        }

        delay(timeLoop);
      }

    } else
    {
      ConectMQTT();
    }

  } else {
    Serial.println("Não estou conectado no wifi...");
    wifiConect();
  }

}
