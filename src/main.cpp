#include <chrono>   // medir tiempo
#include <csignal>  // signal(), SIGPIPE
#include <fstream>  // std::ofstream: escribir archivos
#include <iostream>
#include <random>   // generadores de números aleatorios
#include <string>
#include "target_runner.h" 

// Genera un input de longitud aleatoria (0 a max_len bytes) con bytes aleatorios.
// 'rng' es el generador aleatorio; se pasa por referencia (&) para que
// avance su estado en cada llamada en vez de repetir siempre lo mismo.
std::string generate_random_input(std::mt19937 &rng, size_t max_len)
{
    std::uniform_int_distribution<size_t> len_dist(0, max_len); // reparte uniformemente entre 0 y max_len
    std::uniform_int_distribution<int> byte_dist(0, 255);       // un byte puede valer de 0 a 255

    size_t len = len_dist(rng);
    std::string input;
    input.reserve(len); // reserva memoria de golpe para no ir ampliando
    for (size_t i = 0; i < len; i++)
    {
        input.push_back(static_cast<char>(byte_dist(rng)));
    }
    return input;
}

// argc = num de argumentos, argv = los argumentos como texto.
// Si ejecutas "./crashfinder targets/bug1 1000": argv[0]="./crashfinder",
// argv[1]="targets/bug1", argv[2]="1000", y argc=3.
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Uso: " << argv[0] << " <target> <num_ejecuciones>" << std::endl;
        return 1;
    }

    std::string target = argv[1];
    //runs es el numero de ejecuciones que quiero hacer
    int runs = std::stoi(argv[2]); // stoi: convierte el texto "1000" en el número 1000

    // Si el hijo muere antes de leer su input, write() sobre el pipe roto
    // enviaría SIGPIPE y mataría a nuestro fuzzer. Con SIG_IGN lo ignoramos.
    signal(SIGPIPE, SIG_IGN);

    std::random_device rd;  // semilla impredecible del sistema
    std::mt19937 rng(rd()); // generador Mersenne Twister, inicializado con esa semilla

    int crashes = 0;
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < runs; i++)
    {
        std::string input = generate_random_input(rng, 200);

        if (run_target(target, input))
        {
            crashes++;
            // Guardamos el input que provocó el crash para poder reproducirlo
            std::string path = "crashes/crash_" + std::to_string(crashes) + ".bin";
            std::ofstream out(path, std::ios::binary); // binary: bytes tal cual, sin traducir saltos de línea
            out.write(input.data(), input.size());
        }
    }

    auto end = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(end - start).count();

    std::cout << "Executions:   " << runs << std::endl;
    std::cout << "Crashes:      " << crashes << std::endl;
    std::cout << "Normal exits: " << runs - crashes << std::endl;
    std::cout << "Time (s):     " << secs << std::endl;
    std::cout << "Exec/sec:     " << runs / secs << std::endl;

    return 0;
}