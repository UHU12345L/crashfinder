#include <chrono>     // medir tiempo
#include <csignal>    // signal(), SIGPIPE
#include <cstdint>    // uint64_t: entero de 64 bits sin signo, tamaño garantizado
#include <filesystem> // std::filesystem::exists(): comprobar si un archivo existe
#include <fstream>    // std::ofstream / std::ifstream: escribir y leer archivos
#include <iostream>
#include <iterator>   // std::istreambuf_iterator: leer un archivo entero
#include <random>     // generadores de números aleatorios modernos
#include <string>
#include "target_runner.h" // comillas "" = archivo nuestro, no de la librería estándar

// Hash FNV-1a de 64 bits: convierte unos bytes en un número.
// El mismo contenido da siempre el mismo número, y contenidos distintos
// dan números distintos (casi siempre).
uint64_t fnv1a_hash(const std::string &data)
{
    uint64_t hash = 14695981039346656037ULL; // valor inicial fijado por el algoritmo ("offset basis")
    for (unsigned char c : data)
    {
        hash ^= c;                // mezcla el byte actual con el hash (XOR bit a bit)
        hash *= 1099511628211ULL; // multiplica por un número primo fijado por el algoritmo
    }
    return hash;
}

// Convierte un número en texto hexadecimal de 16 dígitos (para nombres de archivo).
std::string to_hex(uint64_t value)
{
    const char *digits = "0123456789abcdef";
    std::string out(16, '0'); // 16 caracteres, todos '0' de partida
    for (int i = 15; i >= 0; i--)
    {
        out[i] = digits[value & 0xF]; // & 0xF se queda con los 4 bits de la derecha (un dígito hex)
        value >>= 4;                  // desplaza 4 bits a la derecha para pasar al siguiente dígito
    }
    return out;
}

// Genera un input de longitud aleatoria (0 a max_len bytes) con bytes aleatorios.
// 'rng' se pasa por referencia (&) para que su estado avance en cada llamada.
std::string generate_random_input(std::mt19937 &rng, size_t max_len)
{
    std::uniform_int_distribution<size_t> len_dist(0, max_len);
    std::uniform_int_distribution<int> byte_dist(0, 255);

    size_t len = len_dist(rng);
    std::string input;
    input.reserve(len);
    for (size_t i = 0; i < len; i++)
    {
        input.push_back(static_cast<char>(byte_dist(rng)));
    }
    return input;
}

// Modo replay: lee un archivo de crash y lo vuelve a lanzar contra el target.
int replay(const std::string &target, const std::string &crash_file)
{
    std::ifstream in(crash_file, std::ios::binary);
    if (!in)
    {
        std::cerr << "No se pudo abrir " << crash_file << std::endl;
        return 1;
    }
    // Lee el archivo entero: construye un string desde el primer byte hasta el final
    // (istreambuf_iterator recorre el archivo byte a byte; el segundo, vacío, marca el fin)
    std::string input((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    std::cout << "Replay de " << crash_file << " (" << input.size() << " bytes)" << std::endl;
    if (run_target(target, input))
    {
        std::cout << "Resultado: CRASH reproducido" << std::endl;
    }
    else
    {
        std::cout << "Resultado: no crashea" << std::endl;
    }
    return 0;
}

// Si ejecutas "./crashfinder targets/bug1 1000":
// argv[0]="./crashfinder", argv[1]="targets/bug1", argv[2]="1000", argc=3.
int main(int argc, char *argv[])
{
    // Si el hijo muere antes de leer su input, write() sobre el pipe roto
    // enviaría SIGPIPE y mataría al fuzzer. Con SIG_IGN lo ignoramos.
    signal(SIGPIPE, SIG_IGN);

    // Modo replay: ./crashfinder --replay <target> <archivo_crash>
    if (argc == 4 && std::string(argv[1]) == "--replay")
    {
        return replay(argv[2], argv[3]);
    }

    if (argc != 3)
    {
        std::cerr << "Uso:" << std::endl;
        std::cerr << "  " << argv[0] << " <target> <num_ejecuciones>" << std::endl;
        std::cerr << "  " << argv[0] << " --replay <target> <archivo_crash>" << std::endl;
        return 1;
    }

    std::string target = argv[1];
    int runs = std::stoi(argv[2]);

    std::random_device rd;  // semilla impredecible del sistema
    std::mt19937 rng(rd()); // generador Mersenne Twister, inicializado con esa semilla

    int crashes = 0;        // todos los crashes, repetidos incluidos
    int unique_crashes = 0; // solo los inputs distintos
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < runs; i++)
    {
        std::string input = generate_random_input(rng, 200);

        if (run_target(target, input))
        {
            crashes++;

            // El nombre del archivo es el hash del contenido: mismo input, mismo nombre
            std::string path = "crashes/crash_" + to_hex(fnv1a_hash(input)) + ".bin";

            // Si el archivo ya existe, es un duplicado y no lo guardamos otra vez
            if (!std::filesystem::exists(path))
            {
                unique_crashes++;
                std::ofstream out(path, std::ios::binary); // binary: bytes tal cual
                out.write(input.data(), input.size());
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(end - start).count();

    std::cout << "Executions:     " << runs << std::endl;
    std::cout << "Crashes:        " << crashes << std::endl;
    std::cout << "Unique crashes: " << unique_crashes << std::endl;
    std::cout << "Normal exits:   " << runs - crashes << std::endl;
    std::cout << "Time (s):       " << secs << std::endl;
    std::cout << "Exec/sec:       " << runs / secs << std::endl;

    return 0;
}