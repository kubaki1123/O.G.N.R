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

// 4. Na samym końcu ładujemy nasz własny nagłówek, który z tego wszystkiego skorzysta.
#include "lora_mesh.h"

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

    
    void MeshRadio::send(std::string wiadomosc) {
        std::cout<<"wysylam:"<<wiadomosc<<"..."<<std::endl;

        int state = radio->transmit((uint8_t*)wiadomosc.c_str(),wiadomosc.length());

        if(state == RADIOLIB_ERR_NONE){
            std::cout<<"wiadomosc wyslana pomyslnie"<<std::endl;
        }
        else{
            std::cout<<"Blad wysylania wiadomosci"<<std::endl;
        }

        radio->startReceive();
    }

    std::string MeshRadio::receive() {
        std::string odebrana_wiadomosc ="";
        if (check_DIO1()==true){
            size_t dlugosc = radio->getPacketLength();// Najpierw pytamy radio, ile znaków w ogóle do nas przyszło
            uint8_t bufor[256]={0};//tworzymy puste pudelko na bajty

            int state = radio->readData(bufor,dlugosc);//ladujemy dane do pudelka

            if (state == RADIOLIB_ERR_NONE) {
                odebrana_wiadomosc = std::string((char*)bufor,dlugosc);
                std::cout << "Odebrano: " << odebrana_wiadomosc << std::endl;
            } else {
                std::cout << "Odebrano uszkodzony pakiet, kod: " << state << std::endl;
            }
            radio->startReceive();
        }
        return odebrana_wiadomosc;
    }

    bool MeshRadio::check_DIO1() {
        return gpio_get(PIN_DIO1_private);
    }

}