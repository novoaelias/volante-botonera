# Volante Botonera

Botonera para un aro de volante de simulación de carreras F1, hecha con un Arduino Pro Micro que se comporta como joystick USB en Windows.

## Qué problema resuelve

Un volante de simulador necesita muchos botones, pero el microcontrolador tiene pocos pines. Este sketch lee una matriz de 4 filas por 5 columnas con diodos, un encoder rotativo y un potenciómetro, y los presenta al juego como un joystick estándar con 20 botones y un eje.

## Funcionalidades

- Matriz de pulsadores 4x5 con un diodo 1N4148 por switch para evitar el ghosting (varios botones apretados a la vez).
- 12 pulsadores de acero inoxidable de 12 mm (botones 1 a 12) y 6 microswitches de las levas magnéticas (botones 13 a 18).
- Encoder rotativo EC11 leído por interrupciones, que genera pulsos de botón en cada sentido (botones 19 y 20).
- Potenciómetro WH148 de 10k filtrado con histéresis, expuesto como eje Throttle.
- Antirrebote por software con tiempo ajustable.
- Dos posiciones de la matriz libres para crecer.

## Stack

| Parte | Detalle |
|---|---|
| Placa | Arduino Pro Micro (ATmega32u4), seleccionable como Arduino Leonardo o SparkFun Pro Micro 5V/16MHz |
| Lenguaje | C++ de Arduino |
| Librería | [ArduinoJoystickLibrary](https://github.com/MHeironimus/ArduinoJoystickLibrary) de MHeironimus |
| Componentes | Diodos 1N4148, pulsadores de 12 mm, microswitches, encoder EC11, potenciómetro WH148 10k |

## Arquitectura

Las filas son salidas que se ponen en LOW de a una para escanear. Las columnas son entradas con pull-up interno. El número de botón de cada cruce sale de la tabla `MAPA`. El encoder usa los pines 2 y 3, que son los de interrupción del 32u4, y una tabla de decodificación en cuadratura. Cada switch lleva su diodo con la banda (cátodo) hacia la fila.

## Cómo cargarlo

1. Instalá la librería Joystick de MHeironimus en el IDE de Arduino (Programa > Incluir librería > Añadir biblioteca .ZIP).
2. Abrí `volante-botonera/volante-botonera.ino`.
3. Elegí la placa Arduino Leonardo (o SparkFun Pro Micro 5V/16MHz) y el puerto.
4. Cargá el sketch. En Windows, los botones se ven desde 1 en `joy.cpl`.

Pines usados: filas 4, 5, 6 y 7; columnas 8, 9, 10, 16 y 14; encoder 2 y 3; potenciómetro A0.

## Tests

No hay tests automatizados. Se verifica en Windows con `joy.cpl` (Configurar dispositivos de juego), comprobando cada botón, el encoder y el eje.

## Estado del proyecto

Prototipo de hardware: el sketch está escrito para el cableado descrito en el código. Si algún botón rebota o el eje tiembla en reposo, se pueden ajustar las constantes `REBOTE_MS` y `HISTERESIS`.

## Qué aprendí

- Escanear una matriz de teclas con diodos para ahorrar pines y evitar el ghosting.
- Leer un encoder en cuadratura con interrupciones.
- Presentar un dispositivo propio como joystick USB con la librería de MHeironimus.

## English summary

Volante Botonera is an Arduino sketch for the button box of an F1 sim-racing wheel. It runs on a Pro Micro and appears as a USB joystick on Windows. It scans a 4x5 diode matrix with debouncing, reads a rotary encoder with interrupts, and filters a potentiometer for a throttle axis. In total it exposes 20 buttons and one axis to the game. It needs the MHeironimus Joystick library. It is a hardware prototype with no automated tests.