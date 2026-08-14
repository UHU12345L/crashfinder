#include <cstring>
#include <iostream>

int main() {
    char buf[16]; //tiene 15 caracteres utiles, 16 bytes
    std::string input;
    std::getline(std::cin, input); //lo que paso por echo lo guarda en variable input
    strcpy(buf, input.c_str());  //convertir string en char y meter en buf sin comprobar si cabe
    std::cout << "Recibido: " << buf << std::endl;
    return 0;
}

