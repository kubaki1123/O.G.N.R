#pragma once // Zabezpieczenie przed wielokrotnym wklejaniem tego samego pliku podczas kompilacji

#include <iostream> 
#include <string>   
#include <cstdint>  // Dodaje precyzyjne typy liczbowe (np. uint32_t, uint16_t), których wymaga biblioteka

#include <RadioLib.h> // Wgrywamy główny trzon biblioteki, aby kompilator poznał bazowe klasy (np. RadioLibHal)


namespace lora_mesh {

    // Klasa MeshRadio to nasz "wrapper" (opakowanie) na skomplikowaną bibliotekę RadioLib.
    class MeshRadio {
        public:
            // KONSTRUKTOR: Wywoływany automatycznie przy tworzeniu obiektu. 
            // Przyjmuje numery pinów Pico, które posłużą do sterowania nakładką (CS, DIO1, RESET, BUSY).
            MeshRadio(uint32_t PIN_CS, uint32_t PIN_DIO1, uint32_t PIN_RESET, uint32_t PIN_BUSY);

            // METODY: Funkcje dostępne z zewnątrz (np. w main.cpp)
            // Służy do uruchomienia radia z zadaną częstotliwością, mocą i tzw. słowem synchronizującym (hasłem sieci)
            bool initLoRa(double czestotliwosc, int moc, uint16_t synch_word);
            
            void send(std::string wiadomosc); // Funkcja do nadawania pakietów
            std::string receive();            // Funkcja do odbierania pakietów

            // KRYTYCZNA FLAGA: Słowo 'volatile' mówi kompilatorowi: "Ta zmienna może się zmienić 
            // w każdej chwili w tle (przez przerwanie sprzętowe z pinu DIO1)". Bez tego słowa 
            // pętla główna mogłaby zignorować przyjście nowej wiadomości.
            volatile bool nowa_wiadomosc = false;
            
        private:
            // WSKAŹNIKI (*): Używamy wskaźników, bo obiekty te utworzymy dynamicznie (słowem 'new') 
            // dopiero wewnątrz konstruktora (w pliku .cpp).
            
            // POLIMORFIZM: Używamy uniwersalnej klasy bazowej 'RadioLibHal'. 
            // Dzięki temu plik nagłówkowy nie musi w ogóle wiedzieć o istnieniu 'PicoHal', 
            // co chroni nas przed błędami preprocesora i utrzymuje plik .h w czystości.
            RadioLibHal* hal; 
            
            // Moduł to warstwa pośrednia łącząca piny fizyczne (hal) z właściwym sterownikiem
            Module* mod;      
            
            // Główny obiekt reprezentujący nasz fizyczny chip SX1262 na nakładce Waveshare
            SX1262* radio;    

            // Prywatna funkcja pomocnicza, służąca do sprawdzania stanu pinu DIO1
            bool check_DIO1(); 

            uint32_t PIN_DIO1_private;
    };

}


