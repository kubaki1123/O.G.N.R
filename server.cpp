#include "server.h"

namespace server{

    Server_tcp::Server_tcp(lora_mesh::MeshRadio* radio)//konstruktor upewniamy sie ze server_pcb jest pusty i przypisujemy obiekt radia zmiennej radio 
    :server_pcb(nullptr),radio(radio)
    {
        
    }
    
    bool Server_tcp::start_server(uint32_t port){
        std::cout<<"uruchamianie serwera tcp na porcie"<<port<<"..."<<std::endl;
        server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);//stworzenie bloku kontrolnego PCB(protocol control block) to struktura w IwIP ktora reprezentuje jedno gniazdo sieciowe, akceptuje IPv4 i IPv6
        if (server_pcb == nullptr){
            std::cout << "[BLAD] Brak pamieci na serwer TCP!" << std::endl;
            return false;
        }

        err_t err = tcp_bind(server_pcb,IP_ANY_TYPE,port);//wiąże gniazdo z adresem  portem 
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
    void Server_tcp::deliver_to_clients(lora_mesh::MeshPacket& packet) {
        if (packet.type == lora_mesh::PACKET_TYPE_INFO_REQ) {// sprawdzenie jakiego typu jest odebrany pakiet
            // Ktoś pyta o naszą listę — odpowiedz
            std::string lista = "";
            for (const auto& para : nick_do_id) {// petla przechodzaca po mapie nicki_do_id i tworzaca z nich liste urzytkownikow na naszym węźle 
                lista += para.first + ":" + std::to_string(para.second) + ",";//para.first = nick para.second = id 
            }
            // przygotowanie pakietu do wysłania 
            lora_mesh::MeshPacket rsp = {};
            rsp.type = lora_mesh::PACKET_TYPE_INFO_RSP;
            rsp.src_id = node_id;
            rsp.dst_id = packet.src_id;
            rsp.msg_id = ++msg_counter;
            rsp.time_to_live = 3;
            rsp.payload_len = lista.length();
            memcpy(rsp.payload, lista.c_str(), rsp.payload_len);// zamiana pakietu na c_string czyli na surowe bajty 

            radio->send(rsp);//przekazanie pakietu do wysyłki 
            return;
        }

        if (packet.type == lora_mesh::PACKET_TYPE_INFO_RSP) {
            // Dostaliśmy listę od zdalnego węzła — parsuj i zapisz
            std::string tresc((char*)packet.payload, packet.payload_len);// zamiana z c_string na standardowy string aby bezproblemowo wyswietlic liste urzytkownikow 
            std::string wynik = "[SYSTEM] uzytkownicy zdalni (wez." 
                            + std::to_string(packet.src_id) + "): " + tresc;

            // Parsuj "NICK:ID,NICK:ID," i zapisz do nick_do_id
            size_t pos = 0;
            while (pos < tresc.size()) {
                size_t przecinek = tresc.find(',', pos);
                if (przecinek == std::string::npos) break;
                std::string para = tresc.substr(pos, przecinek - pos);
                size_t dwukropek = para.find(':');
                if (dwukropek != std::string::npos) {
                    std::string nick = para.substr(0, dwukropek);
                    uint8_t id = (uint8_t)std::stoi(para.substr(dwukropek + 1));
                    nick_do_id[nick] = id;
                }
                pos = przecinek + 1;
            }

            // Odeślij wynik do klienta który pytał
            if (info_requester_pcb != nullptr) {
                send_to_client(info_requester_pcb, wynik);
                info_requester_pcb = nullptr;
            }
            return;
        }

        // Zwykła wiadomość
        std::string tresc((char*)packet.payload, packet.payload_len);
        std::string wiadomosc = "[src:" + std::to_string(packet.src_id) + "] " + tresc;

        if (packet.dst_id == 0xFF || packet.dst_id == 0xFE) {
            for (const auto& para : klienci) {
                if (!para.second.empty()) send_to_client(para.first, wiadomosc);
            }
        } else {
            std::string odbiorca = tresc.substr(0, tresc.find(':'));
            auto it = nick_do_pcb.find(odbiorca);
            if (it != nick_do_pcb.end()) {
                send_to_client(it->second, wiadomosc);
            } else {
                printf("[INFO] Odbiorca %s nie jest lokalny\n", odbiorca.c_str());
            }
        }
    }
    // --- CALLBACKI LwIP (Wywoływane automatycznie przez system w tle) ---

    err_t Server_tcp::on_connect(void *arg, struct tcp_pcb *newpcb, err_t err) {
        //IwIP wywoluje te funkcje automatycznie gdy klient sie łączy 
        if (err != ERR_OK || newpcb == nullptr) {
            return ERR_VAL;
        }
        std::cout<<"[TCP] Nowy uzytkownik polaczyl sie z serwerem!"<< std::endl;

        // Przekazujemy wskaźnik 'this' dalej, do połączenia z tym konkretnym klientem, każde połączenie z klientem musi znać nasz obiekt serwera aby moc wywoływać jego metody 
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
                std::string nick = serwer->klienci[tpcb];
                serwer->nick_do_pcb.erase(nick);
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
        data.erase(data.find_last_not_of("\n\r\t")+1);// usuwamy znaki konca lini i inne "dodatki" bez tego porownania stringow do komend by nie działały 

        if (data.empty()){
            std::cout<<"brak wiadomosci"<<std::endl;
            return;
        }

        if (klienci[client_pcb]==""){// jezeli klient nie ma jeszcze wpisanego nicku do swojego PCB to jego pierwsza wiadomość jest nickiem 
            klienci[client_pcb] = data;//pcb = nick
            std::cout<<"[SYSTEM] zalogowano nowego uzytkownika:"<<data<<std::endl;
            nick_do_pcb[data] = client_pcb;//nick = pcb (do wiadomosci prywatnych)
            nick_do_id[data] = next_id++;  //nick = id (do pakietow lora)
            send_to_client(client_pcb, "[SYSTEM] Witaj " + data + "! Zalogowano pomyslnie w sieci LoRa.");
            return;
        }

        std::string nadawca = klienci[client_pcb];

        std::string command;
        std::string message;
        int pos = data.find(':');//parser wiadomosci na KOMENDA:[Treść]
        if (pos != std::string::npos) {
            command = data.substr(0, pos);
            message = data.substr(pos + 1);
        } else {
            command = data; // Jeśli nie ma dwukropka, całe słowo (np. "INFO") to komenda
            message = "";
        }

        if (command == "INFO") {
            // 1. Wyślij lokalną listę do klienta który pytał
            std::string lista = "[SYSTEM] uzytkownicy lokalni: ";
            for (const auto& para : klienci) {
                if (!para.second.empty()) lista += para.second + ",";
            }
            send_to_client(client_pcb, lista);
            //send_to_client(client_pcb, "[SYSTEM] czekam na odpowiedz zdalnych wezlow...");
            // 2. Zapamiętaj kto pytał (żeby odesłać odpowiedź zdalną gdy przyjdzie)
            info_requester_pcb = client_pcb;

            // 3. Wyślij INFO_REQ przez radio
            
            lora_mesh::MeshPacket req = {};
            req.type = lora_mesh::PACKET_TYPE_INFO_REQ;
            req.src_id = node_id;
            req.dst_id = 0xFF;
            req.msg_id = ++msg_counter;
            req.time_to_live = 3;
            req.payload_len = 0; // brak treści — samo zapytanie

            kolejka_wysylania.push(req);
            
            return;
        }
        else if(command == "ALL"){
            std::cout<<"wysylanie wiadomosci do wszystkich"<<std::endl;

            std::string wiadomosc ="["+ nadawca + "]" + message;
            for(const auto& para:klienci){
                if(!para.second.empty()){
                    send_to_client(para.first,wiadomosc);
                }
            }

            lora_mesh::MeshPacket packet={};
            msg_counter++;
            packet.src_id = node_id;
            packet.dst_id = 0xFF;
            packet.msg_id = msg_counter;
            packet.time_to_live=3;

            std::string tresc = nadawca +":"+ message;
            packet.payload_len = tresc.length();
            memcpy(packet.payload,tresc.c_str(),packet.payload_len);

            kolejka_wysylania.push(packet);
        }
        else{
            std::cout<<"prywatna wiadomosc do"<<command<<std::endl;
            
            auto it_local = nick_do_pcb.find(command);
            if(it_local != nick_do_pcb.end()){
                //odbiorca jest lokalny 
                std::string wiadomosc = "[" + nadawca + "]" + message;
                send_to_client(it_local->second,wiadomosc);
                printf("[LOCAL] wiadomosc do %s do %s dostarczona lokalnie\n",nadawca.c_str(),command.c_str());
                return ;
            }


            // Sprawdź czy odbiorca jest znany (lokalnie lub przez sieć)
            auto it = nick_do_id.find(command);
            uint8_t dst = (it != nick_do_id.end()) ? it->second : 0xFE; // 0xFE = nieznany

            lora_mesh::MeshPacket packet = {};
            msg_counter++;
            packet.src_id = nick_do_id[nadawca]; // ID nadawcy
            packet.dst_id = dst;
            packet.msg_id = msg_counter;
            packet.time_to_live = 3;

            std::string tresc = command + ":" + message; // "BOB:czesc"
            packet.payload_len = tresc.length();
            memcpy(packet.payload, tresc.c_str(), packet.payload_len);

            kolejka_wysylania.push(packet);
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
