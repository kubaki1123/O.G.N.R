// 1. TWARDY OVERRIDE (MAGIA PREPROCESORA): 
// Zanim załadujemy bibliotekę, wymuszamy flagę środowiska RPi Pico. 
// Dzięki temu plik PicoHal.h "odblokowuje" swoją zawartość dla kompilatora.
#define RADIOLIB_BUILD_RPI_PICO 1

// 2. KRYTYCZNA KOLEJNOŚĆ: Najpierw ładujemy SDK od Raspberry, 
// żeby kompilator poznał sprzęt (zrozumiał np. czym jest 'spi1').
#include "pico/stdlib.h"
#include "hardware/spi.h"

// 3. DOPIERO TERAZ ładujemy RadioLib. Biblioteka widzi załadowane przed chwilą SDK 
// i widzi naszą wymuszoną flagę wyżej, więc bez błędów wczytuje instrukcję PicoHal!
#include <RadioLib.h>
#include <hal/RPiPico/PicoHal.h>

//w pliku sciaganym z biblioteki radiolib odnoszacej sie do picohal nie bylo wypelnionej klasy tone 
void PicoHal::tone(uint32_t pin, unsigned int frequency, unsigned long duration) {
    (void)pin;
    (void)frequency;
    (void)duration;
    
}

// 4. Na samym końcu ładujemy nasz własny nagłówek, który z tego wszystkiego skorzysta.
#include "lora_mesh.h"

#include "mbedtls/aes.h"


namespace lora_mesh {

    // IMPLEMENTACJA KONSTRUKTORA
    MeshRadio::MeshRadio(uint32_t PIN_CS, uint32_t PIN_DIO1, uint32_t PIN_RESET, uint32_t PIN_BUSY)
    :PIN_DIO1_private(PIN_DIO1)
    {
        
        // KROK 1: Inicjalizacja warstwy sprzętowej (HAL).
        // Podajemy tu kanał SPI (spi1) oraz fizyczne piny MISO (12), MOSI (11) i SCK (10) z nakładki.
        hal = new PicoHal(spi1, 12, 11, 10);
        
        // KROK 2: Inicjalizacja Modułu.
        // Podajemy mu instrukcję obsługi sprzętu (hal) oraz piny kontrolne, które przekazano w konstruktorze.
        mod = new Module(hal, PIN_CS, PIN_DIO1, PIN_RESET, PIN_BUSY);
        
        // KROK 3: Stworzenie ostatecznego obiektu radia.
        // Przekazujemy gotowy, oprawiony moduł do sterownika chipu SX1262.
        radio = new SX1262(mod);
    }

    // IMPLEMENTACJA INICJALIZACJI
    bool MeshRadio::initLoRa(double czestotliwosc, int moc, uint16_t synch_word) {
        std::cout << "Rozpoczynam inicjalizacje radia LoRa SX1262..." << std::endl;
        // KRYTYCZNA FUNKCJA: radio->begin()
        // Konfiguruje i uruchamia sprzęt. Parametry w kolejności:
        // 1. czestotliwosc (MHz) - np. 433.0 lub 868.0 w zależności od anteny
        // 2. 125.0 (Bandwidth) - Szerokość pasma w kHz (125 to standard w LoRa)
        // 3. 9 (Spreading Factor) - SF9 to świetny kompromis między zasięgiem (w km) a prędkością
        // 4. 7 (Coding Rate) - Stopień korekcji błędów (4/7)
        // 5. synch_word - tzw. "Hasło" sieciowe. Radia z innym hasłem zignorują Twoje pakiety
        // 6. moc - Moc nadawania w dBm (od 10 do 22 dla SX1262)
        // 7. 8 (Preamble) - Długość preambuły (sygnału ostrzegawczego przed danymi)
        int state = radio->begin(czestotliwosc,125.0,9,7,synch_word,moc,8);

        //sprawdzenie czy czip radiowy poprawnie został skonfigurowany
        if(state == RADIOLIB_ERR_NONE){
            std::cout<<"[OK] Radio gotowe do pracy"<<std::endl;
            radio->startReceive();
            
            return true;
        }
        else{
            std::cout<<"[BLAD] Inicjalizacja nie powiodla sie, kod bledu:"<<state<<std::endl;
            return false;
        }
        
    }

    
    void MeshRadio::send(MeshPacket& packet) {
        std::cout<<"wysylam:"<<packet.payload<<"..."<<std::endl;

        size_t rozmiar_do_nadania = sizeof(MeshPacket)- sizeof(packet.payload) + packet.payload_len;
        printf("[DEBUG] Wysylam %d bajtow, payload_len=%d\n", rozmiar_do_nadania, packet.payload_len);

        encrypt_payload(packet.payload,packet.payload_len,packet.msg_id,packet.src_id);

        int state = radio->transmit((uint8_t*)&packet,rozmiar_do_nadania);

        if(state == RADIOLIB_ERR_NONE){
            std::cout<<"wiadomosc wyslana pomyslnie"<<std::endl;
        }
        else{
            std::cout<<"Blad wysylania wiadomosci"<<std::endl;
        }

        radio->startReceive();
    }

    MeshPacket MeshRadio::receive() {
        MeshPacket received_payload= {};
        if (check_DIO1()==true){
            
            size_t dlugosc = radio->getPacketLength();// Najpierw pytamy radio, ile znaków w ogóle do nas przyszło
            uint8_t bufor[sizeof(MeshPacket)]={0};//tworzymy puste pudelko na bajty

            int state = radio->readData(bufor,dlugosc);//ladujemy dane do pudelka
            printf("[DEBUG] Odebrano %d bajtow\n", dlugosc);
            if (state == RADIOLIB_ERR_NONE) {
                memcpy(&received_payload,bufor,dlugosc);
                decrypt_payload(received_payload.payload,received_payload.payload_len,received_payload.msg_id,received_payload.src_id);
                std::cout << "Odebrano: " << received_payload.payload << std::endl;
            } else {
                std::cout << "Odebrano uszkodzony pakiet, kod: " << state << std::endl;
            }
            radio->startReceive();
        }

        return received_payload;
    }

    bool MeshRadio::check_DIO1() {
        return gpio_get(PIN_DIO1_private);
    }

    void MeshRadio::decrypt_payload(uint8_t *payload, size_t len, uint8_t msg_id, uint8_t src_id){
        uint8_t nonce[16]={0};// NONCE (Number Used Once)liczba jednorazowa 16 bajtow sprawia ze ten sam tekst zaszyfrowany dwuktornie daje rozne wyniki 
        nonce[0]=msg_id;//unikalny numer wiadomosci
        nonce[1]=src_id;//id nadawcy 

        size_t nc_off = 0;// offset w strumieniu klucza mbedTLS aktualizuje te zmienna zeby mozna bylo szyfrowac fragmentami ale my chcemy zaszyfrowac caly payload wiec zaczynamy od 0
        uint8_t stream_block[16] = {0};// wewnetrzny bufor mbedTLS na blok strumienia klucza 
        
        mbedtls_aes_context ctx;//inicjalizacja AES
        mbedtls_aes_init(&ctx);// ctx - struktura przechowywująca rozszerzony klucz AES-128 rozszerza klucz do 176 bajtów mbedtls_aes_init rezerwuje na to pamięć 

        mbedtls_aes_setkey_enc(&ctx,SUPER_SPECIAL_ENCRYPTION_KEY,128);//ładuje klucz i oblicza key schedule w trybie CTR używamy zawsze setkey_enc nawet do deszyfrowania 

        mbedtls_aes_crypt_ctr(&ctx,len,&nc_off,nonce,stream_block,payload,payload);
        /*
        &ctx,           // kontekst z kluczem
        len,            // ile bajtów przetworzyć
        &nc_off,        // offset (aktualizowany przez mbedTLS)
        nonce,          // nasz nonce (msg_id + src_id + zera)
        stream_block,   // wewnętrzny bufor bloku
        payload,        // wejście: jawny tekst (lub zaszyfrowany przy deszyfrowaniu)
        payload         // wyjście: TAKI SAM wskaźnik = szyfrowanie "w miejscu"
                        // nadpisuje payload zaszyfrowanymi danymi bez dodatkowej pamięci
        */

        mbedtls_aes_free(&ctx);// zwalnia pamięć key schedule ważne na mikrokontlorerze z tak małą ilośćia pamięci RAM 
    }

    void MeshRadio::encrypt_payload(uint8_t* payload, size_t len, uint8_t msg_id, uint8_t src_id){// encrypt i decryp działają w identyczny sposób 
        uint8_t nonce[16]={0};
        nonce[0]=msg_id;
        nonce[1]=src_id;

        size_t nc_off = 0;
        uint8_t stream_block[16] = {0};
        
        mbedtls_aes_context ctx;//inicjalizacja AES
        mbedtls_aes_init(&ctx);

        mbedtls_aes_setkey_enc(&ctx,SUPER_SPECIAL_ENCRYPTION_KEY,128);

        mbedtls_aes_crypt_ctr(&ctx,len,&nc_off,nonce,stream_block,payload,payload);

        mbedtls_aes_free(&ctx);

    }



}