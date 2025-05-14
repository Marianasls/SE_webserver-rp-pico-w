
/**
 * Autor: Mariana da Silva Lima Santos
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"        // Biblioteca para arquitetura Wi-Fi da Pico com CYW43
#include "hardware/adc.h"        // Biblioteca da Raspberry Pi Pico para manipulação do conversor ADC
#include "hardware/pio.h"

#include "lib/led_matrix.h"
#include "lib/ws2812.pio.h"

#include "lwip/pbuf.h"           // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"            // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h"          // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)
#include "lwipopts.h"

// Credenciais WIFI - substitua pelos dados da sua rede
const char WIFI_SSID[] = "ssid_rede";
const char WIFI_PASSWORD[] = "senha";
uint8_t power_state = 0; // Estado de energia dos LEDs

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN   // GPIO do CI CYW43
#define LED_BLUE_PIN 12                 // GPIO12 - LED azul
#define LED_GREEN_PIN 11                // GPIO11 - LED verde
#define LED_RED_PIN 13                  // GPIO13 - LED vermelho
#define NUM_PIXELS 25
#define WS2812_PIN 7

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define buttonB 6
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}

// Função de callback para processar requisições HTTP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

// Tratamento do request do usuário
void user_request(char **request);

// Leitura da temperatura interna
float temp_read(void);


// Inicializar os Pinos GPIO para acionamento dos LEDs
void gpio_init_all(void){
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(buttonB);
    gpio_set_dir(buttonB, GPIO_IN);
    gpio_pull_up(buttonB);
    gpio_set_irq_enabled_with_callback(buttonB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Configuração dos LEDs como saída
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, false);
    
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);
    
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, false);

    // Inicializar a matriz de LEDs (WS2812)
	PIO pio = pio0;
	int sm = 0;
	uint offset = pio_add_program(pio, &ws2812_program);
	ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    clear_buffer();
    set_leds(0, 0, 0); // Limpa a matriz de LEDs
}

int main() {
    stdio_init_all();
    gpio_init_all();

    if (cyw43_arch_init()) {
        printf("Falha ao iniciar o wi-fi\n");
        return -1;
    }
    printf("Wi-Fi iniciado com sucesso\n");
    
    cyw43_arch_enable_sta_mode(); // Habilita o modo estação (cliente)

    // Conectar à rede WiFI - fazer um loop até que esteja conectado
    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0){
        printf("Tentando conexão...\n");
    }
    printf("Conectado com sucesso! \n");

    // Caso seja a interface de rede padrão.
    if (netif_default) {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // inicia servidor HTTP e configura periféricos utilizados
    // Se receber requisição HTTP, realiza ação necessária

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server) {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");

    // A PARTIR DAQUI EDITAR PARA TAREFA 
    // Inicializa o conversor ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);

    while (true)
    {
        /* 
        * Efetuar o processamento exigido pelo cyw43_driver ou pela stack TCP/IP.
        * Este método deve ser chamado periodicamente a partir do ciclo principal 
        * quando se utiliza um estilo de sondagem pico_cyw43_arch 
        */
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        sleep_ms(100);      // Reduz o uso da CPU
    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
    return 0;

}

// Tratamento do request do usuário - digite aqui
void user_request(char **request) {
    // Verifica rota /power
    if (strstr(*request, "GET /power?") != NULL) {
        power_state = !power_state; // Alterna o estado de energia
        if (power_state) {
            turn_on_leds();
            set_leds(1, 1, 1); // Liga os LEDs cor branca
        } else {
            clear_buffer(); // Desliga os LEDs
            set_leds(0, 0, 0);
        }
    }
    // Verifica rota /set_color
    else if (strstr(*request, "GET /set_color?") != NULL) {
        const char* param = strstr(*request, "color=");
        if (param) {
            param += strlen("color=");
            int r, g, b;
            if (sscanf(param, "%d,%d,%d", &r, &g, &b) == 3) {
                printf("cor rgb: %d %d %d\n", r, g, b);
                set_leds(r, g, b);
            }
        }
    }
    // Verifica rota /set_intensity
    else if (strstr(*request, "GET /set_intensity?") != NULL) {
        const char* param = strstr(*request, "value=");
        if (param) {
            param += strlen("value=");
            int value = atoi(param);
            if (value >= 0 && value <= 100) {
                printf("Intensidade: %d\n", value);
                set_led_intensity(value); // Ajusta a intensidade dos LEDs
            }
        }
    }
};

// Leitura da temperatura interna
float temp_read(void){
    adc_select_input(4);
    uint16_t raw_value = adc_read();
    const float conversion_factor = 3.3f / (1 << 12);
    float temperature = 27.0f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;
    return temperature;
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

const char *html_template = "<!DOCTYPE html><html lang=\"pt-BR\"><head><meta charset=\"UTF-8\"><title>Controle de LEDs RGB</title><style>\n"
"body{font-family:Arial,sans-serif;background:#f4f4f4;text-align:center;padding:20px;}\n"
".controle-led{display:inline-block;padding:20px;border:2px solid #ccc;border-radius:12px;background:#fff;box-shadow:0 0 10px rgba(0,0,0,0.1);}\n"
".color-buttons{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin:10px auto;max-width:150px;}\n"
".color-button{width:40px;height:40px;border:none;border-radius:6px;cursor:pointer;}\n"
".power-button{width:120px;height:40px;background:#999;color:#fff;border:none;border-radius:6px;cursor:pointer;margin-bottom:15px;font-weight:bold;}\n"
".intensity-container{margin:15px 0;}\n"
"input[type=range]{width:200px;}\n"
"</style></head><body><h2>Controle de LEDs RGB</h2><div class=\"led-control\">\n"
"<form action=\"/power\" method=\"get\"><button class=\"power-button\" type=\"submit\">Liga/Desliga</button></form>\n"
"<div class=\"color-buttons\">\n"
"<form action=\"/set_color\" method=\"get\"><button class=\"color-button\" style=\"background:red;\" type=\"submit\" name=\"color\" value=\"255,0,0\" title=\"Vermelho\"></button></form>\n"
"<form action=\"/set_color\" method=\"get\"><button class=\"color-button\" style=\"background:green;\" type=\"submit\" name=\"color\" value=\"0,255,0\" title=\"Verde\"></button></form>\n"
"<form action=\"/set_color\" method=\"get\"><button class=\"color-button\" style=\"background:blue;\" type=\"submit\" name=\"color\" value=\"0,0,255\" title=\"Azul\"></button></form>\n"
"<form action=\"/set_color\" method=\"get\"><button class=\"color-button\" style=\"background:yellow;\" type=\"submit\" name=\"color\" value=\"255,255,0\" title=\"Amarelo\"></button></form>\n"
"<form action=\"/set_color\" method=\"get\"><button class=\"color-button\" style=\"background:cyan;\" type=\"submit\" name=\"color\" value=\"0,255,255\" title=\"Ciano\"></button></form>\n"
"<form action=\"/set_color\" method=\"get\"><button class=\"color-button\" style=\"background:magenta;\" type=\"submit\" name=\"color\" value=\"255,0,255\" title=\"Magenta\"></button></form>\n"
"</div>\n"
"<form action=\"/set_intensity\" method=\"get\" class=\"intensity-container\">\n"
"<label for=\"intensityRange\">Intensidade:</label><br>\n"
"<input type=\"range\" id=\"intensityRange\" name=\"value\" min=\"0\" max=\"100\" value=\"100\" oninput=\"this.nextElementSibling.value=this.value\">\n"
"<output>100</output><br><br><button type=\"submit\">Aplicar</button></form>\n"
"</div><p class=\"temperature\">Temperatura Interna: %.2f &deg;C</p></body></html>\n";

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p) {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento de request - Controle dos LEDs
    user_request(&request);
    
    // Leitura da temperatura interna
    float temperature = temp_read();

    // Cria a resposta HTML
    char response[2400];

    // Instruções html do webserver
    snprintf(response, sizeof(response), html_template, temperature);

    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    //libera memória alocada dinamicamente
    free(request);
    
    //libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}