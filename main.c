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

struct fileInfo {
    char name[256]; // Nombre del archivo
    intmax_t size;      // Tamaño del archivo
    char date[256];  // Última fecha de modificación (en formato de cadena)
};

struct listNoDirectory {
    struct fileInfo file;
    struct listNoDirectory* next;
};

// Puntero a la cabeza de la lista de archivos que estan en el log pero no en el directorio
struct listNoDirectory* listNoDirectoryHead = NULL;
struct listNoDirectory* listNoDirectoryEnd = NULL; // Puntero al final de la lista de archivos que estan en el log pero no en el directorio
int contador;

struct dirent  *dp;
struct stat     statbuf;
struct passwd  *pwd;
struct group   *grp;
struct tm      *tm;
char            datestring[256];

void liberarListaNoDirectorio() {
    struct listNoDirectory* current = listNoDirectoryHead;
    struct listNoDirectory* next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    listNoDirectoryHead = NULL;
    listNoDirectoryEnd = NULL;
}

void imprimirListaNoDirectorio() {
    struct listNoDirectory* current = listNoDirectoryHead;

    while (current != NULL) {
        printf("Name: %s\nSize: %jd\nDate: %s\n\n", current->file.name, current->file.size, current->file.date);
        current = current->next;
    }
}

void readData() {
    struct fileInfo file;
    contador = 0;
    FILE* f = fopen("logs.txt", "rb"); // Se abre el archivo de modo que se pueda lockear y leer

    if (f == NULL) {
        perror("Error al abrir el archivo de logs");
        return;
    }

    while (fread(&file, sizeof(struct fileInfo), 1, f) > 0) {
        if (strcmp(file.name, "logs.txt") != 0) {
            struct listNoDirectory* newFile = (struct listNoDirectory*)malloc(sizeof(struct listNoDirectory));
            snprintf(newFile->file.name, sizeof(newFile->file.name), "%s", file.name);
            snprintf(newFile->file.date, sizeof(newFile->file.date), "%s", file.date);
            newFile->file.size = file.size;
            newFile->next = NULL;
            if (listNoDirectoryHead == NULL) {
                listNoDirectoryHead = newFile;
                listNoDirectoryEnd = newFile;
            } else {
                listNoDirectoryEnd->next = newFile;
                listNoDirectoryEnd = newFile;
            }

            contador++;
        }
    }

    fclose(f);
}

void guardarDirectorio(char* dirName) {
    DIR* dir = opendir(dirName); // Se abre el directorio
    FILE* logs = fopen("logs.txt", "wb");

    if (dir == NULL || logs == NULL) {
        perror("Error al abrir el directorio o el archivo de logs");
        return;
    }

    /* Loop through directory entries. */
    while ((dp = readdir(dir)) != NULL) {
        struct fileInfo file;

        if (stat(dp->d_name, &statbuf) == -1)
            continue;

        if (dp->d_type != DT_DIR) {
            file.size = statbuf.st_size;
            strncpy(file.name, dp->d_name, sizeof(file.name) - 1);

            tm = localtime(&statbuf.st_mtime);
            strftime(file.date, sizeof(file.date), nl_langinfo(D_T_FMT), tm);

            fwrite(&file, sizeof(file), 1, logs);
            /* Print size of file. */
        }
    }

    closedir(dir);
    fclose(logs);
}

void compararDirectorio(int sock, char *dirName){
    struct 

    DIR *dir = opendir(dirName);
    FILE* logs = fopen("logs.txt", "rb");
    if (logs == NULL) {
        guardarDirectorio(char *dirName);
        return;
    }else{

        /* Loop through directory entries. */
        // En este primer loop se compara por cada archivo del directorio si se encuentra en los logs
        //Caso 1: si se encuentra el archivo del directorio en los logs pero no se modificó
        //Caso 1.2: si se encuentra el archivo del directorio en los logs y se modificó
        //Caso 2: si el archivo que se encuentra en el directorio no se encuentra en los logs es porque es nuevo, se crea el archivo
        while ((dp = readdir(dir)) != NULL) {
        struct fileInfo fileD;
        if (stat(dp->d_name, &statbuf) == -1)
                continue;
            if (dp->d_type != DT_DIR) {
                fileD.size = statbuf.st_size;
                strncpy(fileD.name, dp->d_name, sizeof(fileD.name) - 1);

                tm = localtime(&statbuf.st_mtime);  // Se inicializa tm con la última fecha de modificación
                strftime(fileD.date, sizeof(fileD.date), nl_langinfo(D_T_FMT), tm);

                int found = 0;
                struct fileInfo fileL;
                while ((fread(&fileL, sizeof(struct fileInfo), 1, logs)) > 0) {
                    if(strcmp(fileD.name,fileL.name) == 0){
                        if(strcmp(fileD.date, fileL.date) != 0){
                            //actualizar archivo
                        }
                        found = 1;
                    }
                    
                }
                if (found == 0){
                    //no lo encontro en los logs
                    // es un archivo nuevo
                }
                
            }
        }

        //Segundo loop, recorre los logs para encontrar si algun archivo y no existe en el directorio, significa que se eliminó



        closedir(dir);
        fclose(logs);
    }
}
//----------- cliente
int connectoServer(const char** ip){
  //crear socket para conectar con el servidor y lo retorna
  int sock;
  struct sockaddr_in server;
  
  #if defined _WIN32
  WSADATA wsa_data;
  WSAStartup(MAKEWORD(1, 1), &wsa_data);
  #endif

  // Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    printf("Could not create socket");
    return -1;
  }
  puts("Socket created");

  server.sin_addr.s_addr = inet_addr(ip);
  server.sin_family = AF_INET;
  server.sin_port = htons(8889);

  // Connect to remote server
  if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("connect failed. Error");
    return -1;
  }
  puts("Connected\n");
  return sock;
}
//-------------
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
    server.sin_port = htons(8889);
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
        guardarDirectorio(argv[1]);
        readData();
        imprimirListaNoDirectorio();
        liberarListaNoDirectorio();
    }
    return 0;

}