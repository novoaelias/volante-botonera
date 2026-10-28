/*
 * volante-botonera.ino
 * Botonera del aro F1 para Arduino Pro Micro (ATmega32u4).
 *
 * Entradas:
 *   12 pulsadores de acero inoxidable de 12 mm  -> botones 1 a 12
 *    6 microswitches de las levas magneticas    -> botones 13 a 18
 *    1 encoder rotativo EC11                    -> botones 19 (CW) y 20 (CCW)
 *    1 potenciometro WH148 10k                  -> eje Throttle
 *
 * La matriz es de 4 filas x 5 columnas = 20 posiciones, con 18 usadas y 2 libres.
 * Cada switch lleva un diodo 1N4148 en serie, con la BANDA (catodo) HACIA LA FILA.
 * Sin ese diodo aparece ghosting al apretar varios botones a la vez.
 *
 * Requiere la libreria Joystick de MHeironimus:
 *   https://github.com/MHeironimus/ArduinoJoystickLibrary
 *   En el IDE: Programa > Incluir libreria > Anadir biblioteca .ZIP
 *
 * Placa a seleccionar en el IDE: Arduino Leonardo (o SparkFun Pro Micro 5V/16MHz).
 */

#include <Joystick.h>

/* ---------------------------------------------------------------
 * Pines
 * ------------------------------------------------------------- */

// Filas: salidas. Se ponen en LOW de a una para escanear.
const uint8_t FILAS[] = {4, 5, 6, 7};
const uint8_t N_FILAS = 4;

// Columnas: entradas con pull-up interno. En reposo leen HIGH.
const uint8_t COLUMNAS[] = {8, 9, 10, 16, 14};
const uint8_t N_COLUMNAS = 5;

// Encoder EC11. Pines 2 y 3 son los de interrupcion del 32u4.
const uint8_t ENC_A = 2;
const uint8_t ENC_B = 3;

// Potenciometro
const uint8_t POTE = A0;

/* ---------------------------------------------------------------
 * Mapa de la matriz
 *
 * Numero de boton del joystick que corresponde a cada cruce.
 * -1 = posicion libre, reservada para crecer.
 *
 * Los botones del joystick se numeran desde 0. Windows los muestra
 * desde 1, asi que el boton 0 de aca es el "Boton 1" en joy.cpl.
 *
 * Las filas estan agrupadas por zona fisica del aro para acortar el cableado:
 * la fila 0 y 1 son todo lo del lado izquierdo, la 2 y 3 todo lo del derecho.
 * L1 a L6 y R1 a R6 son los pulsadores de 12 mm numerados de arriba hacia abajo
 * en cada lado. LV1 a LV6 son los microswitches de las levas.
 */
const int8_t MAPA[N_FILAS][N_COLUMNAS] = {
  /*            col 0   col 1   col 2   col 3   col 4  */
  /* fila 0 */ {   0,      1,      2,      3,      4  },  // L1  L2  L3  L4  L5
  /* fila 1 */ {   5,      6,      7,      8,     -1  },  // L6  LV1 LV2 LV3  -
  /* fila 2 */ {   9,     10,     11,     12,     13  },  // R1  R2  R3  R4  R5
  /* fila 3 */ {  14,     15,     16,     17,     -1  }   // R6  LV4 LV5 LV6  -
};

const uint8_t BOTON_ENC_CW  = 18;
const uint8_t BOTON_ENC_CCW = 19;
const uint8_t TOTAL_BOTONES = 20;

/* ---------------------------------------------------------------
 * Antirrebote
 * ------------------------------------------------------------- */
const unsigned long REBOTE_MS = 12;      // subir si algun boton "tartamudea"
const unsigned long PULSO_ENC_MS = 40;   // cuanto dura el pulso del encoder

bool     estado[N_FILAS][N_COLUMNAS];
bool     crudo[N_FILAS][N_COLUMNAS];
uint32_t ultimoCambio[N_FILAS][N_COLUMNAS];

/* ---------------------------------------------------------------
 * Encoder
 * ------------------------------------------------------------- */
volatile int8_t  acumulado = 0;   // suma de transiciones, 4 por detente
volatile uint8_t estadoPrev = 0;

uint32_t finPulsoCW  = 0;
uint32_t finPulsoCCW = 0;

// Tabla de decodificacion en cuadratura.
// Indice = (estado anterior << 2) | estado nuevo.
const int8_t TABLA_ENC[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

void leerEncoder() {
  uint8_t ahora = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  acumulado += TABLA_ENC[(estadoPrev << 2) | ahora];
  estadoPrev = ahora;
}

/* ---------------------------------------------------------------
 * Filtro del potenciometro
 * ------------------------------------------------------------- */
const int HISTERESIS = 6;   // subir si el eje tiembla solo en reposo
int valorPote = 0;

/* ---------------------------------------------------------------
 * Joystick
 * ------------------------------------------------------------- */
Joystick_ Joystick(
  JOYSTICK_DEFAULT_REPORT_ID,
  JOYSTICK_TYPE_JOYSTICK,
  TOTAL_BOTONES,   // cantidad de botones
  0,               // sin hat switch
  false, false,    // sin X ni Y
  false,           // sin Z
  false, false,    // sin Rx ni Ry
  false,           // sin Rz
  false,           // sin rudder
  true,            // Throttle: aca va el potenciometro
  false,           // sin accelerator
  false,           // sin brake
  false            // sin steering
);

/* ---------------------------------------------------------------
 * Setup
 * ------------------------------------------------------------- */
void setup() {
  for (uint8_t f = 0; f < N_FILAS; f++) {
    pinMode(FILAS[f], INPUT);          // alta impedancia hasta que toque escanear
    for (uint8_t c = 0; c < N_COLUMNAS; c++) {
      estado[f][c] = false;
      crudo[f][c] = false;
      ultimoCambio[f][c] = 0;
    }
  }

  for (uint8_t c = 0; c < N_COLUMNAS; c++) {
    pinMode(COLUMNAS[c], INPUT_PULLUP);
  }

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  estadoPrev = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  attachInterrupt(digitalPinToInterrupt(ENC_A), leerEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), leerEncoder, CHANGE);

  pinMode(POTE, INPUT);
  valorPote = analogRead(POTE);

  Joystick.setThrottleRange(0, 1023);
  Joystick.begin(false);               // false = mandamos el reporte a mano
  Joystick.setThrottle(valorPote);
  Joystick.sendState();
}

/* ---------------------------------------------------------------
 * Escaneo de la matriz
 * ------------------------------------------------------------- */
void escanearMatriz(uint32_t ahora) {
  for (uint8_t f = 0; f < N_FILAS; f++) {

    // Activamos una sola fila, poniendola en LOW.
    pinMode(FILAS[f], OUTPUT);
    digitalWrite(FILAS[f], LOW);
    delayMicroseconds(30);             // tiempo de asentamiento

    for (uint8_t c = 0; c < N_COLUMNAS; c++) {
      if (MAPA[f][c] < 0) continue;

      // Con pull-up, apretado = LOW.
      bool apretado = (digitalRead(COLUMNAS[c]) == LOW);

      if (apretado != crudo[f][c]) {
        crudo[f][c] = apretado;
        ultimoCambio[f][c] = ahora;
      }
      else if (apretado != estado[f][c] &&
               (ahora - ultimoCambio[f][c]) >= REBOTE_MS) {
        estado[f][c] = apretado;
        Joystick.setButton((uint8_t)MAPA[f][c], apretado ? 1 : 0);
      }
    }

    // Volvemos la fila a alta impedancia antes de pasar a la siguiente.
    pinMode(FILAS[f], INPUT);
  }
}

/* ---------------------------------------------------------------
 * Encoder como dos botones momentaneos
 * ------------------------------------------------------------- */
void procesarEncoder(uint32_t ahora) {
  int8_t pasos;

  noInterrupts();
  pasos = acumulado / 4;               // 4 transiciones por detente
  acumulado -= pasos * 4;
  interrupts();

  if (pasos > 0) {
    Joystick.setButton(BOTON_ENC_CW, 1);
    finPulsoCW = ahora + PULSO_ENC_MS;
  } else if (pasos < 0) {
    Joystick.setButton(BOTON_ENC_CCW, 1);
    finPulsoCCW = ahora + PULSO_ENC_MS;
  }

  if (finPulsoCW && ahora >= finPulsoCW) {
    Joystick.setButton(BOTON_ENC_CW, 0);
    finPulsoCW = 0;
  }
  if (finPulsoCCW && ahora >= finPulsoCCW) {
    Joystick.setButton(BOTON_ENC_CCW, 0);
    finPulsoCCW = 0;
  }
}

/* ---------------------------------------------------------------
 * Potenciometro
 * ------------------------------------------------------------- */
void procesarPote() {
  int lectura = analogRead(POTE);
  if (abs(lectura - valorPote) > HISTERESIS) {
    valorPote = lectura;
    Joystick.setThrottle(valorPote);
  }
}

/* ---------------------------------------------------------------
 * Loop
 * ------------------------------------------------------------- */
void loop() {
  uint32_t ahora = millis();

  escanearMatriz(ahora);
  procesarEncoder(ahora);
  procesarPote();

  Joystick.sendState();
  delay(2);                            // ~500 Hz, de sobra para una botonera
}
