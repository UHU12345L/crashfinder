#include <cstdlib> //exit(), EXIT_FAILURE
#include <cstring> //strerror(): convierte un código de error en un mensaje
#include <cerrno>  //errno: variable global que guarda el último error del sistema
#include <iostream>
#include <string>
#include <unistd.h>   //fork(), execve(), pipe(), dup2(), close(), write()
#include <sys/wait.h> //waitpid(), WIFSIGNALED, WTERMSIG

// Ejecuta el programa en target_path pasándolen input como si se lo hubieran
// tecleado por stdin. Devuelve true si el proceso murió por una señal (crash).
bool run_target(const std::string &target_path, const std::string &input)
{

    int pipe_fds[2];
    if (pipe(pipe_fds) == -1)
    {
        std::cerr << "Error creando el pipe: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Hijo: fork() devuelve 0
    // Padre: fork() devuelve pid del hijo
    pid_t pid = fork();

    if (pid == -1)
    {
        std::cerr << "Error en fork(): " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        // HIJO

        close(pipe_fds[1]); // hijo lee, cierro escritura

        // dup2(origen, destino)
        // STDIN_FILENO: descriptor estándar de entrada (el 0).
        // cuando el target lea de std::cin, en realidad lee del pipe.
        dup2(pipe_fds[0], STDIN_FILENO);
        close(pipe_fds[0]); // cierro lectura

        // execve() sustituye el proceso actual por otro programa distinto.
        // El hijo se convierte en target_path
        // target_path.c_str(): Ruta del ejecutable
        // args[0]: nombre del programa, args[1]: terminado en nullptr
        // nullptr: variables del entorno del nuevo programa

        char *args[] = {const_cast<char *>(target_path.c_str()), nullptr};
        // NO SIRVE
        // execve(target_path.c_str(), args, nullptr);

        char *envp[] = {
            const_cast<char *>("ASAN_OPTIONS=abort_on_error=1"),
            nullptr // igual que con args[], la lista de entorno también termina en nullptr
        };

        execve(target_path.c_str(), args, envp);
        std::cerr << "Error en execve(): " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    // PADRE (pid != 0)
    close(pipe_fds[0]); // padre escribe, cierro lectura

    // hijo recibe lo escrito en pipe como si se lo hubise tecleado
    // write(donde, el qué, tamaño)
    write(pipe_fds[1], input.c_str(), input.size());

    close(pipe_fds[1]); // cerrar escritura para que hijo lea

    int status;               // guerdo aqui estado final hijo
    waitpid(pid, &status, 0); // espero hijo termine

    // WIFSIGNALED(status) verdadero si proceso murió por una señal (crash)
    if (WIFSIGNALED(status))
    {
        std::cout << "CRASH detectado. Senal: " << WTERMSIG(status) << std::endl;
        return true;
    }

    return false; // termina normal
}

int main()
{
    std::string target = "targets/bug1";

    std::cout << "--- Probando con input seguro ---" << std::endl;
    run_target(target, "hola\n");

    std::cout << "--- Probando con input largo (deberia crashear) ---" << std::endl;
    std::string long_input(100, 'A'); // crea un string de 100 caracteres A seguidos
    run_target(target, long_input + "\n");

    return 0;
}