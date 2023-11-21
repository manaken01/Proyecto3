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

#define BUFFER_SIZE 1024

#if defined _WIN32
#define close(x) closesocket(x)
#endif


//Estructura para enviar mensajes del cliente
struct MensajeCliente {
    char proc[256];
};

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

// Función para enviar un comando al servidor
void send_command(int client_socket, const char* command) {
    // Enviar el comando al servidor
    if (send(client_socket, command, strlen(command), 0) == -1) {
        perror("Error al enviar el comando al servidor");
    }
}


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

// Función para borrar un nodo de la lista
void borrarNodoListaNoDirectorio(struct listNoDirectory* nodo) {
    if (nodo == NULL) {
        return; // No hay nada que borrar
    }

    if (nodo == listNoDirectoryHead) {
        // Si el nodo a borrar es la cabeza de la lista
        listNoDirectoryHead = listNoDirectoryHead->next;
    } else {
        // Si el nodo a borrar está en medio o al final de la lista
        struct listNoDirectory* current = listNoDirectoryHead;
        while (current != NULL && current->next != nodo) {
            current = current->next;
        }

        if (current != NULL) {
            current->next = nodo->next;
        }
    }

    free(nodo); // Liberar la memoria del nodo borrado
}

void guardarDirectorio(char* dirName) {
    DIR* dir = opendir(dirName); // Se abre el directorio

    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s/%s", dirName, "logs.txt");

    FILE* logs = fopen(fullpath, "wb");

    if (dir == NULL || logs == NULL) {
        perror("Error al abrir el directorio o el archivo de logs");
        return;
    }

    /* Loop through directory entries. */
    while ((dp = readdir(dir)) != NULL) {
        struct fileInfo file;

        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;

        /* Get entry's information. */
        if (lstat(fullpath, &statbuf) == -1) {
            continue;
        }

        if (dp->d_type != DT_DIR) {
            if (strcmp(dp->d_name,"logs.txt") != 0) {
                file.size = statbuf.st_size;
                strncpy(file.name, dp->d_name, sizeof(file.name) - 1);

                tm = localtime(&statbuf.st_mtime);
                strftime(file.date, sizeof(file.date), nl_langinfo(D_T_FMT), tm);

                fwrite(&file, sizeof(file), 1, logs);
                /* Print size of file. */
            }
        }
    }

    closedir(dir);
    fclose(logs);
}

void send_file_clientSide(int client_socket, const char* file_path, const char* file_name) {

    FILE* file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return;
    }
    struct MensajeCliente mensaje;
    strncpy(mensaje.proc, "crear", sizeof(mensaje.proc)); //Copia el nombre de la funcion
    send(client_socket, &mensaje, sizeof(mensaje), 0); //Envia el mensaje
    // Espera la respuesta del servidor
    char response[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, response, sizeof(response) - 1, 0);
    if (bytes_received == -1) {
        perror("Error al recibir la respuesta del servidor");
        exit(EXIT_FAILURE);
    }
    response[bytes_received] = '\0';
    printf("Respuesta del servidor: %s\n", response);
    // Obtener el tamaño del archivo
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Obtener el tamaño del nombre del archivo
    size_t file_name_size = strlen(file_name);

    // Enviar el tamaño del nombre del archivo al servidor
    if (send(client_socket, &file_name_size, sizeof(file_name_size), 0) == -1) {
        perror("Error al enviar el tamaño del nombre del archivo");
        fclose(file);
        return;
    }

    // Enviar el nombre del archivo al servidor
    if (send(client_socket, file_name, file_name_size, 0) == -1) {
        perror("Error al enviar el nombre del archivo");
        fclose(file);
        return;
    }

    // Enviar el tamaño del archivo al servidor
    if (send(client_socket, &file_size, sizeof(file_size), 0) == -1) {
        perror("Error al enviar el tamaño del archivo");
        fclose(file);
        return;
    }

    // Enviar el contenido del archivo al servidor en bloques
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) == -1) {
            perror("Error al enviar el archivo");
            fclose(file);
            return;
        }
    }

    fclose(file);
}


void firstTime(int sock, char *dirName) {
    printf("%s\n",dirName);
    DIR *dir = opendir(dirName);
    /* Loop through directory entries. */
    // En este primer loop se compara por cada archivo del directorio si se encuentra en los logs
    //Caso 1: si se encuentra el archivo del directorio en los logs pero no se modificó
    //Caso 1.2: si se encuentra el archivo del directorio en los logs y se modificó
    //Caso 2: si el archivo que se encuentra en el directorio no se encuentra en los logs es porque es nuevo, se crea el archivo
    while ((dp = readdir(dir)) != NULL) {
       
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;
        
        char fullpath[PATH_MAX];
        snprintf(fullpath, PATH_MAX, "%s/%s", dirName, dp->d_name);

        /* Get entry's information. */
        if (lstat(fullpath, &statbuf) == -1) {
            continue;
        }

        printf("%s\n",fullpath);

        if (dp->d_type != DT_DIR) {
                send_file_clientSide(sock,fullpath,dp->d_name);
        }
    }
    closedir(dir);
}

void compararDirectorio(int sock, char *dirName){

    FILE* logs = fopen("logs.txt", "rb");
    if (logs == NULL) {
        firstTime(sock,dirName);
        guardarDirectorio(dirName);
        return;
    }else{
        DIR *dir = opendir(dirName);
        readData();
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
                if (strcmp(dp->d_name,"logs.txt") != 0) {
                    fileD.size = statbuf.st_size;
                    strncpy(fileD.name, dp->d_name, sizeof(fileD.name) - 1);

                    tm = localtime(&statbuf.st_mtime);  // Se inicializa tm con la última fecha de modificación
                    strftime(fileD.date, sizeof(fileD.date), nl_langinfo(D_T_FMT), tm);

                    int found = 0;
                    struct listNoDirectory* current = listNoDirectoryHead;
                    while (current != NULL) {
                        if(strcmp(fileD.name,current->file.name) == 0){
                            if(strcmp(fileD.date, current->file.date) != 0){
                                //actualizar archivo
                                printf("El archivo %s hay que actualizarlo \n", fileD.name);
                            }
                            found = 1;
                            //modificar la lista de los archivos que estan en los logs pero no en el directorio
                            //borrarNodoListaNoDirectorio(current);
                            contador--;
                        }
                        current = current->next;
                    }
                    if (found == 0){
                        //no lo encontro en los logs
                        // es un archivo nuevo
                        printf("No esta en los logs el archivo : %s \n", fileD.name);
                    }
                }
            }
        }

        //Al final la slita deberia tener los nombres de los archivos que estan en los logs pero no en el directorio
        //Por lo que se han borrado
        if(contador > 0){
            //while con funcion de borrar donde se mandan los archivos que deberian ser borrados del servidor
            printf("Se borro #%i archivos \n", contador);
        }

        liberarListaNoDirectorio();
        closedir(dir);
        fclose(logs);
    }
}


// Función para recibir un archivo del cliente
void receive_file_serverSide(int client_socket,char *dirName) {
    // Recibir el tamaño del nombre del archivo
    size_t file_name_size;
    if (recv(client_socket, &file_name_size, sizeof(file_name_size), 0) == -1) {
        perror("Error al recibir el tamaño del nombre del archivo");
        return;
    }

    // Recibir el nombre del archivo
    char file_name[file_name_size + 1];
    if (recv(client_socket, file_name, file_name_size, 0) == -1) {
        perror("Error al recibir el nombre del archivo");
        return;
    }
    file_name[file_name_size] = '\0';
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s/%s", dirName, file_name);
    FILE* file = fopen(fullpath, "wb");
    if (file == NULL) {
        perror("Error al crear el archivo");
        return;
    }
    // Recibir el tamaño del archivo del cliente
    long file_size;
    if (recv(client_socket, &file_size, sizeof(file_size), 0) == -1) {
        perror("Error al recibir el tamaño del archivo");
        fclose(file);
        return;
    }

    // Recibir el contenido del archivo en bloques y escribirlo en el archivo
    char buffer[1024];
    size_t bytes_received;
    while (file_size > 0) {
        size_t bytes_to_receive = sizeof(buffer);
        if (file_size < sizeof(buffer)) {
            bytes_to_receive = file_size;
        }

        bytes_received = recv(client_socket, buffer, bytes_to_receive, 0);
        if (bytes_received == -1) {
            perror("Error al recibir el archivo");
            fclose(file);
            return;
        }

        fwrite(buffer, 1, bytes_received, file);
        file_size -= bytes_received;
    }

    fclose(file);
}

//----------- cliente
int connectoServer(char* ip){
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
int startServer(char *dirName) {
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
    struct MensajeCliente mensaje;
    while ((read_size = recv(client_sock, &mensaje, sizeof(mensaje), 0)) > 0) {
        if (strcmp("crear", mensaje.proc) == 0) {
            const char* response = "Listo para recibir archivo";
            send(client_sock, response, strlen(response), 0);
            receive_file_serverSide(client_sock,dirName);
        } 
    }
    guardarDirectorio(dirName);

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
        startServer(argv[1]);
    }else if(argc == 3){
        compararDirectorio(connectoServer(argv[2]), argv[1]);
    }
    return 0;

}