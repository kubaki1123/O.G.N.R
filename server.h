#pragma once
#include<iostream>
#include<string>
#include<vector>
#include<map>
#include "pico/cyw43_arch.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lora_mesh.h"


namespace server{

    
    class Server_tcp{
        public:
            Server_tcp(lora_mesh::MeshRadio* radio);//konstruktor
            bool start_server(uint32_t port);//odpala server na porcie np.8080
            void deliver_to_clients(lora_mesh::MeshPacket& packet);
        private:
            lora_mesh::MeshRadio* radio;
            struct tcp_pcb* server_pcb;//struktura trzymajaca glowne poloczenie servera 
            std::map<struct tcp_pcb*, std::string> klienci;
            void process_incoming_data(std::string data,struct tcp_pcb* client_pcb);
            
            void send_to_client(struct tcp_pcb* client_pcb,std::string data);
            std::map<std::string, struct tcp_pcb*> nick_do_pcb;
            uint8_t msg_counter = 0;
            std::map<std::string, uint8_t> nick_do_id;
            uint8_t next_id = 1; // auto-inkrementowany licznik ID dla nowych klientów
            struct tcp_pcb* info_requester_pcb = nullptr;
            uint8_t node_id=1;

            // --- CALLBACKI lwIP (Muszą być static!) ---
            static err_t on_connect(void *arg, struct tcp_pcb *newpcb, err_t err);
            static err_t on_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

    };

}


