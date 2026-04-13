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
        // Odzyskujemy nasz obiekt C++ z ukrytego argumentu
        Server_tcp* serwer = static_cast<Server_tcp*>(arg);

        // Jeśli bufor 'p' jest pusty, to sygnał od klienta: "Rozłączam się"
        if (p == nullptr) {
            std::cout<< "[TCP] Uzytkownik sie rozlaczyl." << std::endl;
            if (serwer != nullptr) {
                serwer->klienci.erase(tpcb); // usuwa uzytkownikow ktorzy rozloczyli sie z siecia
            }
            tcp_close(tpcb);
            return ERR_OK;
        }

        // Mówimy systemowi (lwIP): "Potwierdzam odebranie tylu a tylu bajtów"
        tcp_recved(tpcb, p->tot_len);

        // Wyciągamy surowe znaki (C-string) z bufora sieciowego i robimy z nich std::string
        std::string odebrany_tekst((char*)p->payload, p->len);

        // Zwalniamy pamięć RAM mikrokontrolera (BARDZO WAŻNE, inaczej Pico się zawiesi)
        pbuf_free(p);

        // Przekazujemy tekst do naszej logiki parsowania
        if (serwer != nullptr) {
            serwer->process_incoming_data(odebrany_tekst,tpcb);
        }

        return ERR_OK;
    }

    // --- FUNKCJE LOGIKI ---

    void Server_tcp::process_incoming_data(std::string data,struct tcp_pcb* client_pcb) {
        std::cout << "Serwer przetwarza dane: " << data << std::endl;
        data.erase(data.find_last_not_of("\n\r\t")+1);

        if (data.empty()){
            std::cout<<"brak wiadomosci"<<std::endl;
            return;
        }

        if (klienci[client_pcb]==""){
            klienci[client_pcb] = data;
            std::cout<<"[SYSTEM] zalogowano nowego uzytkownika:"<<data<<std::endl;
            send_to_client(client_pcb, "[SYSTEM] Witaj " + data + "! Zalogowano pomyslnie w sieci LoRa.");
            return;
        }

        std::string nadawca = klienci[client_pcb];

        std::string command;
        std::string message;
        int pos = data.find(':');
        if (pos != std::string::npos) {
            command = data.substr(0, pos);
            message = data.substr(pos + 1);
        } else {
            command = data; // Jeśli nie ma dwukropka, całe słowo (np. "INFO") to komenda
            message = "";
        }

        if(command == "INFO"){
            std::cout<<"[SYSTEM]"<<nadawca<< "pyta o liste urzytkownikow podlaczonych do sieci"<<std::endl;
            
            std::string lista_lokalnie = "[SYSTEM] obecni uzytkownicy lokalnie:";
            for(const auto& para:klienci){
                if(!para.second.empty()){
                    lista_lokalnie+=para.second + ",";
                }
            }

           send_to_client(client_pcb,lista_lokalnie);

            // TODO: Zbudowanie ramki systemowej (np. "SYS_REQ_INFO")
            // TODO: Wyslanie ramki przez radio.send() w eter do drugiego Pico
           
            return; 
        }
        else if(command == "ALL"){
            std::cout<<"wysylanie wiadomosci do wszystkich"<<std::endl;

        }
        else{
            std::cout<<"prywatna wiadomosc do"<<command<<std::endl;

        }
    

    }

    void Server_tcp::send_to_client(struct tcp_pcb* client_pcb,std::string message){
        if(client_pcb == nullptr){
            return;
        }
        
        message += "\r\n";

        // TCP_WRITE_FLAG_COPY mówi serwerowi: "Skopiuj sobie ten tekst do własnej pamięci 
        // przed wysłaniem, bo zaraz go tu usunę"
        tcp_write(client_pcb, message.c_str(), message.length(), TCP_WRITE_FLAG_COPY);
        
        // Wypychamy paczkę do anteny
        tcp_output(client_pcb);

    }


    

    

        

}
