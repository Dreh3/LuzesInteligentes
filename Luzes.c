#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h> 
#include <stdlib.h>

#include "pico/stdlib.h"         
#include "pico/cyw43_arch.h"     //Biblioteca responsável por integrar o cyw43 e o lwIP

#include "lwip/pbuf.h"           //Biblioteca para manipulação dos pacotes de dados 
#include "lwip/tcp.h"            
#include "lwip/netif.h" 

#include "lib/matriz.h"

// Credenciais WIFI
#define WIFI_SSID "Nome"
#define WIFI_PASSWORD "senha"

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN   // GPIO do CI CYW43

//Pino da matriz de leds
PIO pio = pio0; 
uint sm = 0;    

static uint estado_luzes = 0;   //Permite modificar entre ligado e desligado na matriz de leds
static uint modo = 0;           //Permite alternar entre as duas intensidades disponíveis

//Protótipos das funções

void Matriz_Leds();

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

// Tratamento do request do usuário
void user_request(char **request);

int main()
{
    //Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();

    //Configurações para matriz de leds
    uint offset = pio_add_program(pio, &pio_matrix_program);
    pio_matrix_program_init(pio, sm, offset, MatrizLeds, 800000, IS_RGBW);

    uint tentativas = 1;                        //Usado no laço para controlar o número de tentativas de conexão

    //Inicializa a arquitetura do cyw43
    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar módulo Wi-Fi\n");
        sleep_ms(100);
        return -1;                              //Return -1 pode ser usado para indicar que houve erro de conexão
    }

    // GPIO do CI CYW43 em nível alto
    cyw43_arch_gpio_put(LED_PIN, 1);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI - fazer um loop para três tentativas para que seja conectado
    printf("Conectando ao Wi-Fi...\n");
    //Tenta conectar ao Wi-Fi até que uma rede seja conectada, uma falha detectada ou exceda o tempo limite
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        if(tentativas == 3){    //Verifica a quantidade de tentativas. 
            return -1;          //Sai do laço caso não haja conexão após três tentativas
            printf("Encerrando o programa\n");
        };
        tentativas++;
        printf("Conectando ao Wi-Fi...\n");
    };
    printf("Conectado ao Wi-Fi\n");

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr)); //Transforma o IP padrão em uma string
    };

    // Configura o servidor TCP
    struct tcp_pcb *server = tcp_new();
    if (!server)                        //Se server for NULL houve falha ou o limite de conexões foi atingido
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    };

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)        //Ou a porta já está em uso ou o PCB não está em um estado válido
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    };

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");


    while (true)
    {
        cyw43_arch_poll(); //Permite que o wi-fi mantenha ativo
        sleep_ms(100);     
    };

    
    cyw43_arch_deinit();
    return 0;
};

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Tratamento do request do usuário - digite aqui
void user_request(char **request){

    if (strstr(*request, "GET /luz_on") != NULL)
    {
        estado_luzes = 1;
    }
    else if (strstr(*request, "GET /luz_off") != NULL)
    {
        estado_luzes = 0;
    }
    else if (strstr(*request, "GET /modo_n") != NULL)
    {
        modo = 0;
    }
    else if (strstr(*request, "GET /modo_s") != NULL)
    {
       modo = 1;
    }
};

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento de request
    user_request(&request);

    //Chama aqui a função para a matriz
    Matriz_Leds();

    // Cria a resposta HTML
    char html[1100];

    // Instruções html do webserver
    snprintf(html, sizeof(html), // Formatar uma string e armazená-la em um buffer de caracteres
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             //"<html lang=\"pt-BR\">\n"
            "<style>\n"
            "body { background-color: #FFFFFF ; font-family: Arial, sans-serif; text-align: center; margin-top: 50px;color: #333 }\n"
            "h1 { font-size: 32px; margin-bottom: 30px; color: #0288D1 }\n"
            "button { background-color: #4FC3F7;cursor: pointer; font-size: 18px; margin: 10px; padding: 20px 40px; border-radius: 25px;color: white ;border: none; }\n"
            ".button-group {display: flex;justify-content: center;gap: 20px;flex-wrap: wrap;}\n"
            "</style>\n"
            "<head>\n"
            //"<meta charset=\"UTF-8\">\n"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> \n"
            "<title>Controle Casa</title>\n"
            "</head>\n"
            "<body>\n"
            "<h1>Sistema de Luzes Inteligente</h1>\n"
            "<h2>Quarto</h2>\n"
            "<div class=\"section\">\n"
            "<div class=\"button-group\">\n"
            "<form action=\"./luz_on\"><button>Ligar</button></form>\n"
            "<form action=\"./luz_off\"><button>Desligar</button></form>\n"
            "</div>\n"
            "</div>\n"
            "<h4>Selecione a intensidade:</h4>\n"
            "<div class=\"button-group\">\n"
            "<form action=\"./modo_n\"><button>Normal</button></form>\n"
            "<form action=\"./modo_s\"><button>Suave</button></form>\n"
            "</div>\n"
            "</div>\n"
            "</body>\n"
            "</html>\n");

    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    //libera memória alocada dinamicamente
    free(request);
    
    //libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
};

//Função para a matriz de Leds
void Matriz_Leds(){

    COR_RGB apagado = {0.0,0.0,0.0};    //Define os padrões para desligar um led
    float intensidades []= {1,0.2};     //Vetor com as intensidades disponíveis
    
    //Vetor para permitir alterna o estado do led: posição 0 - led desligado; posição 1 - led branco
    //Estando no vetor facilita as modificações ao atualizar a matriz
    COR_RGB luzes [] = {{0.0,0.0,0.0},{0.5*intensidades[modo],0.5*intensidades[modo],0.5*intensidades[modo]}}; 

    Matriz_leds iluminacao = {{apagado,apagado,apagado,apagado,apagado},
                            {apagado,luzes[estado_luzes], luzes[estado_luzes],luzes[estado_luzes],apagado},
                            {apagado,luzes[estado_luzes], apagado,luzes[estado_luzes],apagado},
                            {apagado,luzes[estado_luzes], luzes[estado_luzes],luzes[estado_luzes],apagado},
                            {apagado,apagado,apagado,apagado,apagado}};
    
    //Chama a função que ligará os leds definidos na matriz acima
    ligar_leds(iluminacao);
    
};