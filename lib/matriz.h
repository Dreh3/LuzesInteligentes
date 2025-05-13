//Arquivo .pio
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "build/pio_matrix.pio.h"
#define IS_RGBW false 
#define MatrizLeds 7 //Pino para matriz de leds
extern PIO pio;
extern uint sm;

//Definindo struct para cores personalizadas
typedef struct {
    double red;
    double green;
    double blue;
  }Led_RGB;
  
//Definindo tipo Cor
typedef Led_RGB COR_RGB;
  
// Definição de tipo da matriz de leds
typedef Led_RGB Matriz_leds[5][5];

uint32_t cor_binario (double b, double r, double g);
void ligar_leds(Matriz_leds matriz);


