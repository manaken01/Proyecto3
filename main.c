#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 /* Windows XP. */
#endif
#include <Ws2tcpip.h>
#include <winsock2.h>
#else
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#endif
#if defined _WIN32
#define close(x) closesocket(x)
#endif

typedef struct fileInfo {
    char name[256]; // Nombre del archivo
    long size;      // Tamaño del archivo
    char date[20];  // Última fecha de modificación (en formato de cadena)
    int first;
    struct fileInfo* next;
} fileInfo;

struct dirent  *dp;
struct stat     statbuf;
struct passwd  *pwd;
struct group   *grp;
struct tm      *tm;
char            datestring[256];

void imprimirDirectorio(char *dirName) {
    DIR *dir = opendir(dirName); //Se abre el directorio 
    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) //Se compara para saber si se esta dentro
            continue;                                                       //de un directorio o subdirectorio, si lo es continua

        char path[1024];// Se inicializa la variable
        strcpy(path, dirName); //Necesario para copiar el nombre de la ruta y comparar despues
        strcat(path, "/");  //Agrega una barrita ya que son directorios 
        strcat(path, dp->d_name); //Agrega el nombre del directorio actual al fi

        if (stat(path, &statbuf) == -1)  //Si da error es que no se puede acceder a un archivo o un directorio
            continue; //Continua con la siguiente recursion

        for (int i = 0; i < identacion; i++) {
            printf(" ");
        }

        if (dp->d_type == DT_DIR) { //Verifica si se encuentra en un directorio
            printf("📁 %s\n", dp->d_name); //Imprime el directorio con un emoji de carpeta
            imprimirDirectorio(path, identacion + 2); //Sigue buscando directorios, se aumenta la identación
        } else {
            tm = localtime(&statbuf.st_mtime);
            strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm); //Saca la fecha que se creo
            printf("📄 %s - Tamaño: %ld - Creado el: %s\n",  dp->d_name, (intmax_t)statbuf.st_size, datestring); //Imprime el nombre, tamaño y fecha
        }
    }
    closedir(dir);
}

void startServer() {
    int socket_desc, client_sock, c, read_size;
    struct sockaddr_in server, client;
#if defined _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif
    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
    }
    puts("Socket created");
    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);
    // Bind
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        // print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
    // Listen
    listen(socket_desc, 3);
    // Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    // accept connection from an incoming client
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }
    puts("Connection accepted");
    // Receive a message from client

    if (read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    } else if (read_size == -1) {
        perror("recv failed");
    }
#if defined _WIN32
    WSACleanup();
#endif
    return 0;
}

int main(int argc, char* argv[]) {

    if (argc == 2) {
        startServer();
    } else if (argc == 3) {

    }


}