#pragma once
#include<iostream>
#include<string>
#include<vector>
#include<map>
#include<queue>
#include "pico/cyw43_arch.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lora_mesh.h"


namespace server{

    
    class Server_tcp{
        public:
            Server_tcp(lora_mesh::MeshRadio* radio);//konstruktor przyjmujacy wskaznik do obiektu radia
            bool start_server(uint32_t port);//odpala server na porcie np.8080
            void deliver_to_clients(lora_mesh::MeshPacket& packet);//funkcja odpowiedzialna za rozpoznanie typu pakietu i wyslanie go odpowiednim klienta przez tcp 
            bool has_pending_packet() { return !kolejka_wysylania.empty(); }// funckja sprawdzajaca czy w kolejce sa jakies pakiety do wyslania 
            lora_mesh::MeshPacket pop_packet() {
                auto p = kolejka_wysylania.front();
                kolejka_wysylania.pop();
                return p;
            }
        private:
            lora_mesh::MeshRadio* radio;//wskaznik na obiekta radia uzywany do zarzadzania radiem
            struct tcp_pcb* server_pcb;//struktura trzymajaca glowne poloczenie servera 
            std::map<struct tcp_pcb*, std::string> klienci;// mapa polaczen TCP do nazw zalogowanych urzytkownikow 
            void process_incoming_data(std::string data,struct tcp_pcb* client_pcb);// przetwarza dane parsuje je i decyduje co z nimi zrobic 
            
            void send_to_client(struct tcp_pcb* client_pcb,std::string data);//wysyla do odpowiedniego klienta 
            std::map<std::string, struct tcp_pcb*> nick_do_pcb;// mapa nickow i przypisanych im plytek
            uint8_t msg_counter = 0;//licznik ilosci wyslanych wiadomosci uzywany do ustawiania id wiadomosci
            std::map<std::string, uint8_t> nick_do_id;//mapa id klientow i ich nickow 
            uint8_t next_id = 1; // auto-inkrementowany licznik ID dla nowych klientów
            struct tcp_pcb* info_requester_pcb = nullptr;
            uint8_t node_id=1;// id wezla pico trzeba zmienic przy tworzeniu drugiego fizycznego ukladu potem skompilowac i wgrac na plytke jest to jedyna zmiana w softwarze  
            std::queue<lora_mesh::MeshPacket> kolejka_wysylania;//kolejka pakietow do wsylania 

            // --- CALLBACKI lwIP (Muszą być static!) ---
            static err_t on_connect(void *arg, struct tcp_pcb *newpcb, err_t err);//funckje obslugujace zachowanie ujkladu gdy otrzyma wiadomosc i polaczy sie z urzytkownikiem 
            static err_t on_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

    };

}


