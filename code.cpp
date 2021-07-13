/* Programa para Artigo FilipeFlop:
 * Monitor o volume na sua Lixeira
 * Autor: 
 * Biblioteca da Placa: "esp8266 by ESP8266 Community versão 2.3.0"
 * Placa: "NodeMCU 1.0 (ESP-12E Module)"
 * Upload Speed: "115200"
 * CPU Frequency: "160MHz"
*/
//=====================================================================
// --- Inclusão de bibliotecas ---
#include <ESP8266WiFi.h> 
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
// ======================================================================
// --- Dados de Acesso do seu roteador ---
#define WLAN_SSID       "Ayka" // Informação da SSID do seu roteador
#define WLAN_PASS       "Aykamaria"   // senha de acesso do seu roteador
// ======================================================================
// --- Dados de Acesso da Plataforma Adafruit IO ---
#define AIO_SERVER      "io.adafruit.com"     // manter fixo
#define AIO_SERVERPORT  1883                  // manter fixo
#define AIO_USERNAME    "Luiz_nunes"        // sua informação
#define AIO_KEY         "aio_kVcH75BVFQuKmPbAV9BcxeZUkfVy" // sua informação
// ======================================================================
// --- Mapeamento de Hardware ---
#define trigPin 5  //D1 - PINO DE TRIGGER PARA SENSOR ULTRASSONICO
#define echoPin 4  //D2 - PINO DE ECHO PARA SENSOR ULTRASSONICO
#define led1 D3  //  LED Verde
#define led2 D8 //  LED vermelho
#define led3 D4 //  LED Amarelo
WiFiClient client; // cria uma classe WiFiClient com o ESP8266 para conexão com o servidor MQTT
 
// Configura a classe MQTT passando para o WiFi cliente e Servidor MQTT os detalhes do login
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
 
// O endereçamento para enviar os arquivos seguem o formato: <username>/feeds/<feedname>
Adafruit_MQTT_Publish volume = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/volume");
 
// ======================================================================
// --- Variáveis Globais ---
long duracao = 0;
float distancia = 0;
int volumevar = 0;




// ======================================================================
// --- Void Setup ---
void setup() {
   
  Serial.begin(115200); // inicia comunicação serial com velocidade 115200
 
  Serial.println(F("Monitorar Volume Caixa D'água - Adafruit MQTT")); // escreve na serial
   
  // Conecta ao ponto de acesso WiFi
  Serial.println(); Serial.println();
  Serial.print("Conectando ao ");
  Serial.println(WLAN_SSID);
 
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
 
  Serial.println("WiFi Conectado");
  Serial.println("IP endereço: "); Serial.println(WiFi.localIP());
   
// ======================================================================
// --- Configuração IO ---
  pinMode(trigPin, OUTPUT); // pino D1 como saída para TRIGGER
  pinMode(echoPin, INPUT);  // pino D2 como entrada para ECHO
  pinMode(led1, OUTPUT); //pino D3 como led verde
  pinMode(led2, OUTPUT); //pino D8 como led verde
  pinMode(led3, OUTPUT); //pino D5 como led verde
}
// ======================================================================
// --- void loop ---
void loop() {
  MQTT_connect();   // chama função para conexão com MQTT server
   
  digitalWrite(trigPin, LOW);    // Limpa trigPin
  delayMicroseconds(2);          // aguarda 2 microsegundos
 
  digitalWrite(trigPin, HIGH);   // Seta trigPin HIGH aguarda 
  delayMicroseconds(10);         // aguada 10 microsegundos
  digitalWrite(trigPin, LOW);    // Seta trigPin LOW 
   
  // Leitura do echoPin, retorna a onda de som em microsegundos
  duracao = pulseIn(echoPin, HIGH);
  distancia= duracao*0.034/2;
  distancia = distancia; // converter cm para mm
  
  
  if (distancia >=20 && distancia <= 30){  // leitura mínima. Reservatório vazio
    distancia = distancia;
    
    statusLixeira("metade");
  }
    if (distancia < 20){  // leitura máxima. Reservatório vazio
    distancia = distancia;

    statusLixeira("cheio");

  }
  volumevar = map(distancia, 37, 96, 1000, 0); 

  /* Remapeia o range de leitura
   * Ao invés de ler de 37 a 96, lerá de 1000 a 0*/

  Serial.print("distanciaCM:"); // imprime "distancia:"
  Serial.println(distancia);  // imprime a variavel distancia
  Serial.print("volume:");    // imprime "volume:"
  Serial.println(volumevar);  // imprime a variavel volume
   
  volume.publish(distancia);     // publica variável "distância" em no feed "volume"
  // nossa saída será em mL
   
  delay(3000); // aguarda 3 segundos
  /* Observação: A plataforma Adafruit IO só permite 30 publicações/minuto
   * na conta gratuita. Então é importante não ultrapassar ou sua leitura 
   * na dashboard será instável e incorreta.*/

  if (distancia > 30){
   digitalWrite (led1, HIGH);
   delay (3000);
   digitalWrite (led1,LOW);}
    if (distancia < 30 && distancia > 20){
    digitalWrite (led2,HIGH);
    delay(3000);
    digitalWrite (led2,LOW);}
      if (distancia < 20){
      digitalWrite (led3, HIGH);
      delay (3000);
      digitalWrite (led3, LOW);}


}
 
// ======================================================================
// --- Função para Conexão / Reconexão MQTT ---
  void MQTT_connect() {
    int8_t ret;
   
    // para de tentar reconexão quando consegue
    if (mqtt.connected()) {
      return;
    }
    Serial.print("Conectando ao MQTT... "); // imprime na serial
    // tenta conexão 5 vezes. Depois WatchDogTime!
    uint8_t retries = 5;
    while ((ret = mqtt.connect()) != 0) { // conectará quando retornar 0
      Serial.println(mqtt.connectErrorString(ret));
      Serial.println("Nova tentativa de conexão MQTT em 5 segundos...");
      mqtt.disconnect();
      delay(5000);  // aguarda 5 segundos
      retries--;
      if (retries == 0) { // trava e espera reset
        while (1);
      }
    }
    Serial.println("MQTT Conectado!"); // imprime na serial
  }
// ======================================================================

void statusLixeira(String status) {

    int httpResponseCode;

      // comunicação back end
    HTTPClient http;
    
        // Confugurações url back end 
      String nodeapi = "http://192.168.0.26:3000/";
    
        // set url
        http.begin(nodeapi);
        http.addHeader("Content-Type", "application/json");

    if(status == "cheio") {
        httpResponseCode = http.POST("{\"statusLixeira\":\"cheio\"}");
    } else if ( status == "metade") {
         httpResponseCode = http.POST("{\"statusLixeira\":\"metade\"}");
    }

      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
}

        
// --- FIM ---