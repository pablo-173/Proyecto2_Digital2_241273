
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <ESP32SPISlave.h>
#include "Wire.h"

#define COMANDO_LEER 0x01
#define I2C_DEB 0x28
#define ledR 4
#define ledG 2
#define ledB 15
#define Size 1
#define SPI_BYTES 4
#define pot 34
#define I2C_SDA 21
#define I2C_SCL 22
#define d4 27
#define d5 14
#define d6 26
#define d7 13

#define en 32
#define rs 25

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

uint8_t sensor;
uint8_t data;

ESP32SPISlave slave;

alignas(4) uint8_t TxData[SPI_BYTES] = {0};
alignas(4) uint8_t RxData[SPI_BYTES] = {0};

uint8_t led = 0;
uint16_t tiempo = 0;

uint8_t pinActivo = 0;
uint32_t inicio = 0;

bool ledEncendido = false;
volatile bool solicitudI2C = false;

char ultimoLED = '-';
unsigned long ultimaActualizacionLCD = 0;

uint16_t lecturaADC = 0;
uint8_t valor8bits = 0;
uint16_t milivoltios = 0;
uint8_t maplecturaADC = 0;

volatile uint8_t respuestaI2C[4] = {0, 0, 0, 0};

void RecibirSPI();
void ProcesarComandoSPI();
void ControlarLED(uint8_t led, uint16_t tiempo);
void ActualizarLED();
void ActualizarI2C();
void RecibirSolicitudI2C(int cantidad);
void EnviarMedicionI2C();
void ActualizarLCD();


void setup() {
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pot1: Pot1: LED:");
    Serial.begin(115200);
    pinMode(ledR, OUTPUT);
    pinMode(ledG, OUTPUT);
    pinMode(ledB, OUTPUT);
    pinMode(pot, INPUT);
    slave.setDataMode(SPI_MODE0);
    slave.setQueueSize(Size);
    slave.begin(VSPI);
    Wire.begin(I2C_DEB, I2C_SDA, I2C_SCL, 100000);
    Wire.onReceive(RecibirSolicitudI2C);
    Wire.onRequest(EnviarMedicionI2C);

}

void loop() 
{
    RecibirSPI();
    ActualizarLED();
    ActualizarI2C();
    ActualizarLCD();
}
void RecibirSPI()
{
    // Preparar la transaccion cuando este disponible
    if (slave.hasTransactionsCompletedAndAllResultsHandled())
    {
        if (slave.queue(TxData, RxData, SPI_BYTES))
        {
            if (slave.trigger())
            {
                // Transaccion preparada
            }
            else
            {
                Serial.println("Error al iniciar SPI");
            }
        }
        else
        {
            Serial.println("Error al preparar SPI");
        }
    }

    // Revisar si se completo la transaccion
    if (slave.hasTransactionsCompletedAndAllResultsReady(Size))
    {
        auto resultados = slave.numBytesReceivedAll();

        if (!resultados.empty())
        {
            Serial.print("Bytes recibidos: ");
            Serial.println(resultados[0]);

            if (resultados[0] == SPI_BYTES)
            {
                Serial.print("SPI RX: ");

                for (int i = 0; i < SPI_BYTES; i++)
                {
                    Serial.print(RxData[i]);
                    Serial.print(" ");
                }

                Serial.println();

                ProcesarComandoSPI();
            }
            else
            {
                Serial.println("Transaccion SPI incompleta");
            }
        }
    }
}
void ProcesarComandoSPI()
{
    if (RxData[0] != 0xAA)
    {
        Serial.println("Encabezado SPI incorrecto");
        return;
    }

    uint8_t comando = RxData[1];

    uint16_t tiempoRecibido =
        ((uint16_t)RxData[2] << 8) |
        (uint16_t)RxData[3];

    Serial.println("\n===== DATOS SPI =====");

    Serial.print("LED recibido: ");
    Serial.println(comando);

    Serial.print("Tiempo recibido: ");
    Serial.println(tiempoRecibido);

    if (comando >= 1 && comando <= 3)
    {
        ControlarLED(comando, tiempoRecibido);
    }
    else
    {
        Serial.println("LED invalido");
    }
}
void ControlarLED(uint8_t ledRecibido, uint16_t tiempoRecibido)
{
    led = ledRecibido;
    tiempo = tiempoRecibido;

    // Apagar LEDs anteriores
    digitalWrite(ledR, LOW);
    digitalWrite(ledG, LOW);
    digitalWrite(ledB, LOW);

    ledEncendido = false;
    pinActivo = 0;
    if (led == 1)
    {
        pinActivo = ledR;
    }
    else if (led == 2)
    {
        pinActivo = ledB;
    }
    else if (led == 3)
    {
        pinActivo = ledG;
    }

    if (pinActivo != 0 && tiempo > 0)
    {
        digitalWrite(pinActivo, HIGH);

        inicio = millis();
        ledEncendido = true;

        Serial.print("LED encendido: ");
        Serial.println(led);

        Serial.print("Tiempo: ");
        Serial.println(tiempo);
    }
     if (led == 1)
        {
            ultimoLED = 'R';
        }
    else if (led == 2)
        {
            ultimoLED = 'B';
        }
    else if (led == 3)
        {
            ultimoLED = 'G';
        }
        }
void ActualizarLED()
{
    if (ledEncendido)
    {
        if (millis() - inicio >= tiempo)
        {
            digitalWrite(pinActivo, LOW);

            ledEncendido = false;

            Serial.println("LED apagado");
        }
    }
}

void ActualizarI2C()
{
    if (solicitudI2C)
    {
        solicitudI2C = false;

        // Lectura ADC de 12 bits (0-4095)
        lecturaADC = analogRead(pot);


        // Convertir de 0-4095 a 0-255
        valor8bits =
            ((uint32_t)lecturaADC * 255 + 2047) / 4095;

        // Calcular voltaje estimado en milivoltios
        milivoltios =
            ((uint32_t)lecturaADC * 3300 + 2047) / 4095;

        // Preparar respuesta I2C
        respuestaI2C[1] = valor8bits;
        respuestaI2C[2] = (milivoltios >> 8) & 0xFF;
        respuestaI2C[3] = milivoltios & 0xFF;

        // Indicar que la medicion esta lista
        respuestaI2C[0] = 1;

        Serial.println("\n===== MEDICION ADC =====");

        Serial.print("Lectura ADC (0-4095): ");
        Serial.println(lecturaADC);

        Serial.print("Valor convertido (0-255): ");
        Serial.println(valor8bits);

        Serial.print("Voltaje estimado: ");
        Serial.print(milivoltios);
        Serial.println(" mV");
    }
}
void RecibirSolicitudI2C(int cantidad)
{
    while (Wire.available())
    {
        uint8_t comando = Wire.read();

        if (comando == COMANDO_LEER)
        {
            respuestaI2C[0] = 0;
            solicitudI2C = true;
        }
    }
}
void EnviarMedicionI2C()
{
    uint8_t datos[4];

    datos[0] = respuestaI2C[0];
    datos[1] = respuestaI2C[1];
    datos[2] = respuestaI2C[2];
    datos[3] = respuestaI2C[3];

    Wire.write(datos, 4);
}
void ActualizarLCD()
{
    unsigned long tiempoActual = millis();

    if (tiempoActual - ultimaActualizacionLCD >= 1000)
    {
        ultimaActualizacionLCD = tiempoActual;

        lecturaADC = analogRead(pot);

        // Convertir de 0-4095 a 0-255
        valor8bits =
            ((uint32_t)lecturaADC * 255 + 2047) / 4095;

        // Voltaje aproximado en mV
        milivoltios =
            ((uint32_t)lecturaADC * 3300 + 2047) / 4095;

        // Primera fila
        lcd.setCursor(0, 0);
        lcd.print("Pot1: Pot1: LED:");

        // Segunda fila
        char texto[17];

        snprintf(texto, sizeof(texto),
                 "%lu.%02luV %-3u    %c ",
                 (unsigned long)(milivoltios / 1000),
                 (unsigned long)((milivoltios % 1000) / 10),
                 valor8bits,
                 ultimoLED);

        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print(texto);
    }
    }
