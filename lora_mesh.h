#pragma once // Zabezpieczenie przed wielokrotnym wklejaniem tego samego pliku podczas kompilacji

#include <iostream> 
#include <string>   
#include <cstdint>  // Dodaje precyzyjne typy liczbowe (np. uint32_t, uint16_t), których wymaga biblioteka

#include <RadioLib.h> // Wgrywamy główny trzon biblioteki, aby kompilator poznał bazowe klasy (np. RadioLibHal)

namespace lora_mesh {
        //struktura pakietu
    struct MeshPacket{
        uint8_t type;
        uint8_t src_id;
        uint8_t dst_id;
        uint8_t msg_id;
        uint8_t time_to_live;
        uint8_t payload_len;
        uint8_t payload[215]; 
    };

    constexpr uint8_t PACKET_TYPE_MSG      = 0;
    constexpr uint8_t PACKET_TYPE_INFO_REQ = 1;
    constexpr uint8_t PACKET_TYPE_INFO_RSP = 2;
    // Klasa MeshRadio to nasz "wrapper" (opakowanie) na skomplikowaną bibliotekę RadioLib.
    class MeshRadio {
        public:
            // KONSTRUKTOR: Wywoływany automatycznie przy tworzeniu obiektu. 
            // Przyjmuje numery pinów Pico, które posłużą do sterowania nakładką (CS, DIO1, RESET, BUSY).
            MeshRadio(uint32_t PIN_CS, uint32_t PIN_DIO1, uint32_t PIN_RESET, uint32_t PIN_BUSY);

            // METODY: Funkcje dostępne z zewnątrz (np. w main.cpp)
            // Służy do uruchomienia radia z zadaną częstotliwością, mocą i tzw. słowem synchronizującym (hasłem sieci)
            bool initLoRa(double czestotliwosc, int moc, uint16_t synch_word);
            
            void send(MeshPacket& packet); // Funkcja do nadawania pakietów
            MeshPacket receive();            // Funkcja do odbierania pakietów

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

            static constexpr uint8_t SUPER_SPECIAL_ENCRYPTION_KEY[16]={ //static constexpr - jeden egzamplarz w pamieci znany w czasie kompilacji zero kosztu dla RAM podczas runtime 
                0x3D,0xAD,0x37,0x2B,0xCF,0xFF,0x2A,0x4F,0xAA,0xFB,0x23,0xFC,0x98,0xFA,0x4F,0x90
            };

            
            void encrypt_payload(uint8_t* payload, size_t len, uint8_t msg_id, uint8_t src_id);
            void decrypt_payload(uint8_t* payload, size_t len, uint8_t msg_id, uint8_t src_id);
    };

}


