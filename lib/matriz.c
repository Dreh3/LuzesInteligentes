#include "matriz.h"

//Retorno o valor binário para a cor passada por parâmetro
uint32_t cor_binario (double b, double r, double g)
{
unsigned char R, G, B;
R = r * 255; 
G = g * 255;  
B = b * 255;
return (G << 24) | (R << 16) | (B << 8);
};

//Função responsável por acender os leds desejados 
void ligar_leds(Matriz_leds matriz){
  //Primeiro for para percorrer cada linha
  for (int linha =4;linha>=0;linha--){
      /*
      Devido à ordem de disposição dos leds na matriz de leds 5X5, é necessário
      ter o cuidado para imprimir o desenho na orientação correta. Assim, o if abaixo permite o 
      desenho saia extamente como projetado.
      */

      if(linha%2){                             //Se verdadeiro, a numeração das colunas começa em 4 e decrementam
          for(int coluna=0;coluna<5;coluna++){
              uint32_t cor = cor_binario(matriz[linha][coluna].blue,matriz[linha][coluna].red,matriz[linha][coluna].green);
              pio_sm_put_blocking(pio, sm, cor);
          };
      }else{                                      //Se falso, a numeração das colunas começa em 0 e incrementam
          for(int coluna=4;coluna>=0;coluna--){
              uint32_t cor = cor_binario(matriz[linha][coluna].blue,matriz[linha][coluna].red,matriz[linha][coluna].green);
              pio_sm_put_blocking(pio, sm, cor);
          };
      };
  };
};
