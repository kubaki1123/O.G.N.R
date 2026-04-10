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
            Server_tcp();//konstruktor
            bool start_server(uint32_t port);//odpala server na porcie np.8080

        private:
            struct tcp_pcb* server_pcb;//struktura trzymajaca glowne poloczenie servera 
            std::map<struct tcp_pcb*, std::string> klienci;
            void process_incoming_data(std::string data,struct tcp_pcb* client_pcb);
            std::string encrypt_data(std::string text);
            void send_to_client(struct tcp_pcb* client_pcb,std::string message);


            // --- CALLBACKI lwIP (Muszą być static!) ---
            static err_t on_connect(void *arg, struct tcp_pcb *newpcb, err_t err);
            static err_t on_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    };

}


