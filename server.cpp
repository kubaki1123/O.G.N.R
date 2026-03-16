#include "server.h"

namespace server{

    Server_tcp::Server_tcp()//konstruktor upewniamy sie ze server_pcb jest pusty 
    {
        server_pcb = nullptr;
    }
    
    bool Server_tcp::start_server(uint32_t port){
        std::cout<<"uruchamianie serwera tcp na porcie"<<port<<"..."<<std::endl;
        server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
        if (server_pcb == nullptr){
            std::cout << "[BLAD] Brak pamieci na serwer TCP!" << std::endl;
            return false;
        }

        err_t err = tcp_bind(server_pcb,IP_ANY_TYPE,port);//podpinamy serwer pod dowolny adress IP i nasz port 8080
        if (err != ERR_OK) {
            std::cout << "[BLAD] Port " << port << " jest zajety lub niedostepny." << std::endl;
            return false;
        }

        server_pcb = tcp_listen(server_pcb);//przestawiamy pcb w tryb nasłuchiwania 

        tcp_arg(server_pcb, this);           // 1. Przekazujemy wskaźnik na nasz obiekt serwera
        tcp_accept(server_pcb, on_connect);  // 2. Mówimy, jaka funkcja ma przyjąć klienta

        std::cout << "[ OK ] Serwer TCP nasluchuje na polaczenia!" << std::endl;
        return true;
    }
// --- CALLBACKI LwIP (Wywoływane automatycznie przez system w tle) ---

    err_t Server_tcp::on_connect(void *arg, struct tcp_pcb *newpcb, err_t err) {
        if (err != ERR_OK || newpcb == nullptr) {
            return ERR_VAL;
        }
        std::cout<<"[TCP] Nowy uzytkownik polaczyl sie z serwerem!"<< std::endl;

        // Przekazujemy wskaźnik 'this' dalej, do połączenia z tym konkretnym klientem
        tcp_arg(newpcb, arg);

        // Ustawiamy, co ma się stać, gdy ten klient coś do nas napisze
        tcp_recv(newpcb, on_recv);

        return ERR_OK;
    }

    err_t Server_tcp::on_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
        // Jeśli bufor 'p' jest pusty, to sygnał od klienta: "Rozłączam się"
        if (p == nullptr) {
            std::cout<< "[TCP] Uzytkownik sie rozlaczyl." << std::endl;
            tcp_close(tpcb);
            return ERR_OK;
        }

        // Mówimy systemowi (lwIP): "Potwierdzam odebranie tylu a tylu bajtów"
        tcp_recved(tpcb, p->tot_len);

        // Odzyskujemy nasz obiekt C++ z ukrytego argumentu
        Server_tcp* serwer = static_cast<Server_tcp*>(arg);

        // Wyciągamy surowe znaki (C-string) z bufora sieciowego i robimy z nich std::string
        std::string odebrany_tekst((char*)p->payload, p->len);

        // Zwalniamy pamięć RAM mikrokontrolera (BARDZO WAŻNE, inaczej Pico się zawiesi)
        pbuf_free(p);

        // Przekazujemy tekst do naszej logiki parsowania
        if (serwer != nullptr) {
            serwer->process_incoming_data(odebrany_tekst);
        }

        return ERR_OK;
    }

    // --- FUNKCJE LOGIKI ---

    void Server_tcp::process_incoming_data(std::string data) {
        std::cout << "Serwer przetwarza dane: " << data << std::endl;
        // Tutaj w następnym kroku napiszemy algorytm szukający "INFO" i "BOB:"
    }

    std::string Server_tcp::encrypt_data(std::string text) {
        // Pusty szkielet dla kryptografii
        return text; 
    }
    

    

        

}
