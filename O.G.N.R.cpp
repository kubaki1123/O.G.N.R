#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/cyw43_arch.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "dhcpserver.h"
#include "lora_mesh.h"
#include "server.h"

// SPI Defines
#define SPI_PORT spi1
#define PIN_MISO 12 //tym pasem magistrali wiadomosci wracaja z lora do pico (master in slave out)
#define PIN_CS   3 //(chip select) pico uzya tego pinu zeby poinformowac lora o nadchodzącym przesyle danych 
#define PIN_SCK  10 //(serial clock) zegar który synchronizuje predkosci przesyłania danych miedzy układami 
#define PIN_MOSI 11 //(master out slave in) tedy pico wysyła komendy do modułu lora 

#define PIN_BUSY 2 //układ sx1262 ma wewnatrz siebie procesor odpowiedzialny za przetwarzanie skomplikowanych komend kiedy procesor mysli ustawia na pinie BUSY stan wysoki 
#define PIN_RESET 15 //kiedy pico sie uruchamia nadaje na ten pin krótki sygnał który wymusza reset układu sx1262
#define PIN_DIO1 20 // Zamiast kazać procesorowi Raspberry Pi Pico co milisekundę pytać modułu LoRa: Hej, odebrałeś już coś?(co pożera prąd i blokuje inne zadania), Pico może zająć się swoimi sprawami. Kiedy moduł LoRa usłyszy ramkę danych sam z siebie wysyła krótki impuls elektryczny


int main()
{
    stdio_init_all();// odpala komunikacje po kablu miedzy komputerem a raspberry 
    sleep_ms(3000);
   
    if (cyw43_arch_init()) {    //cyw43_arch_init budzi układ wi-fi na płytce pico ze snu jezeli z jakiegos poodu jest on uszkodzony wyswietli sie blad 
        printf("Wi-Fi init failed\n");
        return -1;
    }

    
    spi_init(SPI_PORT, 1000*1000);// uruchamia spi z predkoscia 1Mhz to standardowa bezpieczna predkosc dla takich modułów 
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);//te 4 komendy gpio_set_function mówią piną w pico ze od teraz nie sa przełącznikami pradu tylko służa WYŁĄCZNIE do komunikacji SPI 
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // gpio_set_dir i gpio_put konfiguruja pin CS domyslnmie ustawiaja na nim stan wysoki mówiący "Module LoRa narazie nic od ciebie nie chce mozesz spac"
   
    
    cyw43_arch_enable_ap_mode("O.G.N.R.1","123456789",CYW43_AUTH_WPA2_MIXED_PSK);// uruchamia mobilny access point z nazwa sieci O.G.N.R.1 i hasłem 123456789
    ip4_addr_t gw, mask;
    IP4_ADDR(&gw, 192, 168, 4, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);

    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &gw, &mask);
    printf("Serwer DHCP uruchomiony\n");

    ip4_addr_t ip;
    IP4_ADDR(&ip, 192, 168, 4, 1);
    netif_set_addr(netif_default, &ip, &mask, &gw);
    printf("Access Point O.G.N.R.1 został uruchomiony\n");


    lora_mesh::MeshRadio radio(PIN_CS, PIN_DIO1, PIN_RESET, PIN_BUSY);
    server::Server_tcp server(&radio); 

    radio.initLoRa(868.0,22,0x44);
    server.start_server(8080);

    while (true) {
        cyw43_arch_poll();
        lora_mesh::MeshPacket packet = radio.receive();
        if (packet.payload_len > 0|| packet.type!=0){
            printf("[LoRa] Odebrano pakiet type:%d src_id:%d\n", packet.type, packet.src_id);
            server.deliver_to_clients(packet);
        }
        sleep_ms(10);
    }   
}
