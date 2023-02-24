# Find your friends

Each `ESP01` board has a preprogrammed color that defines the person. Each board needs to be preprogrammed with the color you want it to send to other devices. When you sit next to another person, that person's board will fade with the color of your board, and your board will fade with the color of the other person.


## Code
The values from the array define the PWM values for R, G, and B channels, and last it defines the fade effect speed:

```c
const int personValues[] = { 255, 100, 0, 30 };
```

The board is prepared to be used with common anode RGB LEDs, if you're willing to use common cathode LEDs, just comment out the following definition:

```c
#define COMMON_ANODE
```

The code is prepared to be debugged with an ESP8266 board that already has a USB port, just comment out the following definitions:

```c
#define ESP01
#define DEBUG
```
Mind that on the `ESP01` board, the RX and TX pins are used as GPIO outputs to drive the led so it's mandatory to comment out the `DEBUG` define to get the three outputs working.

## Electronics

The pinout arrangement was done in a way that simplifies the wiring procedure. Both boards are glued together near their pin holes, the battery is glued back on both boards and the RGB LED sits over the `ESP01` board, pointing to the WiFi antenna.

![Board schematic](schematic.png)

## Case

The case is a pentagon in form of a jewel, the [top](pendant-top.stl) part stores the boards, led, and battery and the [bottom](pendant-bottom.stl) one is just a cap to make it look nice.

![Pendants](pendants.jpg)