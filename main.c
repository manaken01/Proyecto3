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


//Estructura para enviar messagges del cliente
struct clientMessagge {
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
int counter;

struct dirent  *dp;
struct stat     statbuf;
struct passwd  *pwd;
struct group   *grp;
struct tm      *tm;
char            datestring[256];

void freeListNoDirectory() {
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

void printListNoDirectory() {
    struct listNoDirectory* current = listNoDirectoryHead;

    while (current != NULL) {
        printf("Name: %s\nSize: %jd\nDate: %s\n\n", current->file.name, current->file.size,current->file.date);
        current = current->next;
    }
}

void readData(char* dirName) {
    struct fileInfo file;
    counter = 0;
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s/%s", dirName, ".logs.txt");
    FILE* logs = fopen(fullpath, "rb");

    if (logs == NULL) {
        perror("Error al abrir el archivo de logs");
        return;
    }

    while (fread(&file, sizeof(struct fileInfo), 1, logs) > 0) {
        if (strcmp(file.name, ".logs.txt") != 0) {
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

            counter++;
        }
    }
    fclose(logs);
}

// Función para borrar un nodo de la lista
void deleteNodeListNoDirectory(struct listNoDirectory* node) {
    if (node == NULL) {
        return; // No hay nada que borrar
    }

    if (node == listNoDirectoryHead) {
        // Si el nodo a borrar es la cabeza de la lista
        listNoDirectoryHead = listNoDirectoryHead->next;
    } else {
        // Si el nodo a borrar está en medio o al final de la lista
        struct listNoDirectory* current = listNoDirectoryHead;
        while (current != NULL && current->next != node) {
            current = current->next;
        }

        if (current != NULL) {
            current->next = node->next;
        }
    }

    free(node); // Liberar la memoria del nodo borrado
}

void checkConflicts(char* filename, char* dirName){
    char fullpath2[PATH_MAX];
    snprintf(fullpath2, PATH_MAX, "%s/%s", dirName, filename);

    /* Get entry's information. */
    lstat(fullpath2, &statbuf);

    struct fileInfo fileS;
    readData(dirName);

    tm = localtime(&statbuf.st_mtime);  // Se inicializa tm con la última fecha de modificación
    strftime(fileS.date, sizeof(fileS.date), nl_langinfo(D_T_FMT), tm);

    strncpy(fileS.name,filename, sizeof(fileS.name) - 1);

    struct listNoDirectory* current = listNoDirectoryHead;
    while (current != NULL) {
        if (strcmp(fileS.name, current->file.name) == 0) {
            if (strcmp(fileS.date, current->file.date) != 0) {
                char newName[256];
                snprintf(newName, 256,"%s_%s", "(1)",filename);

                char newFullPath[PATH_MAX];
                snprintf(newFullPath, PATH_MAX, "%s/%s", dirName, newName);
                if (rename(fullpath2, newFullPath) == 0) {
                    printf("El archivo se ha renombrado exitosamente.\n");
                   
                } else {
                    printf("");
                }
                
                printf("El archivo %s hay que actualizarlo \n", fileS.name);
            }
            break;
        } else {
            current = current->next;
        }
    
    }
    freeListNoDirectory();

}

void saveDirectory(char* dirName) {
    DIR* dir = opendir(dirName); // Se abre el directorio

    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s/%s", dirName, ".logs.txt");

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

        char fullpath2[PATH_MAX];
        snprintf(fullpath2, PATH_MAX, "%s/%s", dirName, dp->d_name);

        /* Get entry's information. */
        if (lstat(fullpath2, &statbuf) == -1) {
            continue;
        }

        if (dp->d_type != DT_DIR) {
            if (strcmp(dp->d_name,".logs.txt") != 0) {
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

void deleteFile(int clientSocket, const char* fileName) {
    struct clientMessagge messagge;
    strncpy(messagge.proc, "eliminar", sizeof(messagge.proc)); //Copia el nombre de la funcion
    send(clientSocket, &messagge, sizeof(messagge), 0); //Envia el messagge
    // Espera la respuesta del servidor
    char response[BUFFER_SIZE];
    ssize_t bytesReceived = recv(clientSocket, response, sizeof(response) - 1, 0);
    if (bytesReceived == -1) {
        perror("Error al recibir la respuesta del servidor");
        exit(EXIT_FAILURE);
    }
    response[bytesReceived] = '\0';
    printf("Respuesta del servidor: %s\n", response);
    strncpy(messagge.proc, fileName, sizeof(messagge.proc)); //Copia el nombre de la funcion
    send(clientSocket, &messagge, sizeof(messagge), 0); //Envia el messagge
}

void receiveDeleteFile(int clientSocket,char *dirName) {
    struct clientMessagge messagge;
    // recibir el nombre del archivo a borrar
    recv(clientSocket, &messagge, sizeof(messagge), 0);
    // Intenta eliminar el archivo
    printf("%s\n",messagge.proc);
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s/%s", dirName, messagge.proc);
    if (remove(fullpath) == 0) {
        printf("El archivo \"%s\" se eliminó correctamente.\n", messagge.proc);
    } else {
        printf("");
    }
}

void sendFile(int clientSocket, const char* filePath, const char* fileName) {

    FILE* file = fopen(filePath, "rb");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return;
    }
    struct clientMessagge messagge;
    strncpy(messagge.proc, "crear", sizeof(messagge.proc)); //Copia el nombre de la funcion
    send(clientSocket, &messagge, sizeof(messagge), 0); //Envia el messagge
    // Espera la respuesta del servidor
    char response[BUFFER_SIZE];
    ssize_t bytesReceived = recv(clientSocket, response, sizeof(response) - 1, 0);
    if (bytesReceived == -1) {
        perror("Error al recibir la respuesta del servidor");
        exit(EXIT_FAILURE);
    }
    response[bytesReceived] = '\0';
    printf("Respuesta del servidor: %s\n", response);
    // Obtener el tamaño del archivo
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    // Obtener el tamaño del nombre del archivo
    size_t fileName_size = strlen(fileName);

    // Enviar el tamaño del nombre del archivo al servidor
    if (send(clientSocket, &fileName_size, sizeof(fileName_size), 0) == -1) {
        perror("Error al enviar el tamaño del nombre del archivo");
        fclose(file);
        return;
    }

    // Enviar el nombre del archivo al servidor
    if (send(clientSocket, fileName, fileName_size, 0) == -1) {
        perror("Error al enviar el nombre del archivo");
        fclose(file);
        return;
    }

    // Enviar el tamaño del archivo al servidor
    if (send(clientSocket, &fileSize, sizeof(fileSize), 0) == -1) {
        perror("Error al enviar el tamaño del archivo");
        fclose(file);
        return;
    }

    // Enviar el contenido del archivo al servidor en bloques
    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(clientSocket, buffer, bytesRead, 0) == -1) {
            perror("Error al enviar el archivo");
            fclose(file);
            return;
        }
    }

    fclose(file);
}


void firstTime(int sock, char *dirName) {
    DIR *dir = opendir(dirName);
    /* Loop through directory entries. */
    while ((dp = readdir(dir)) != NULL) {
       
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;
        
        char fullpath[PATH_MAX];
        snprintf(fullpath, PATH_MAX, "%s/%s", dirName, dp->d_name);

        /* Get entry's information. */
        if (lstat(fullpath, &statbuf) == -1) {
            continue;
        }

        if (dp->d_type != DT_DIR) {
            sendFile(sock,fullpath,dp->d_name);
        }
    }
    closedir(dir);
}

void checkDirectory(int sock, char *dirName){
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s/%s", dirName, ".logs.txt");
    FILE* logs = fopen(fullpath, "rb");
    if (logs == NULL) {
        firstTime(sock,dirName);
        saveDirectory(dirName);
        return;
    } else {
        DIR *dir = opendir(dirName);
        readData(dirName);
        /* Loop through directory entries. */
        // En este primer loop se compara por cada archivo del directorio si se encuentra en los logs
        //Caso 1: si se encuentra el archivo del directorio en los logs pero no se modificó
        //Caso 1.2: si se encuentra el archivo del directorio en los logs y se modificó
        //Caso 2: si el archivo que se encuentra en el directorio no se encuentra en los logs es porque es nuevo, se crea el archivo
        while ((dp = readdir(dir)) != NULL) {
            struct fileInfo fileD;
            if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
                continue;

            char fullpath2[PATH_MAX];
            snprintf(fullpath2, PATH_MAX, "%s/%s", dirName, dp->d_name);

            /* Get entry's information. */
            if (lstat(fullpath2, &statbuf) == -1) {
                continue;
            }
                
            if (dp->d_type != DT_DIR) {
                if (strcmp(dp->d_name,".logs.txt") != 0) {
                    fileD.size = statbuf.st_size;
                    strncpy(fileD.name, dp->d_name, sizeof(fileD.name) - 1);

                    tm = localtime(&statbuf.st_mtime);  // Se inicializa tm con la última fecha de modificación
                    strftime(fileD.date, sizeof(fileD.date), nl_langinfo(D_T_FMT), tm);

                    int found = 0;
                    struct listNoDirectory* current = listNoDirectoryHead;
                    while (current != NULL) {
                        if (strcmp(fileD.name, current->file.name) == 0) {
                            if (strcmp(fileD.date, current->file.date) != 0) {
                                //Se actualiza el archivo
                                struct clientMessagge messagge;
                                strncpy(messagge.proc, "modificar", sizeof(messagge.proc)); //Copia el nombre de la funcion
                                send(sock, &messagge, sizeof(messagge), 0); //Envia el messagge

                                strncpy(messagge.proc, fileD.name, sizeof(messagge.proc)); //Copia el nombre de la funcion
                                send(sock, &messagge, sizeof(messagge), 0); //Envia el messagge
                                sendFile(sock,fullpath2,dp->d_name);
                            }
                            found = 1;
                            // modificar la lista de los archivos que están en los logs pero no en el directorio
                            struct listNoDirectory* nextNode = current->next;  // Guardar el siguiente node antes de borrar el actual
                            deleteNodeListNoDirectory(current);
                            counter--;
                            current = nextNode;  // Actualizar current al siguiente node guardado
                        } else {
                            current = current->next;
                        }
                    }
                    if (found == 0) {
                        sendFile(sock,fullpath2,dp->d_name);
                        //no lo encontro en los logs
                        // es un archivo nuevo

                    }
                }
            }
        }

        //Al final la lita deberia tener los nombres de los archivos que estan en los logs pero no en el directorio
        //Por lo que se han borrado
        if (counter > 0){
            //while con funcion de borrar donde se mandan los archivos que deberian ser borrados del servidor
            printf("Se borro #%i archivos \n", counter);
            struct listNoDirectory* current = listNoDirectoryHead;
            while (current != NULL) {
                deleteFile(sock,current->file.name);
                struct listNoDirectory* nextNode = current->next;  // Guardar el siguiente node antes de borrar el actual
                deleteNodeListNoDirectory(current);
                counter--;
                current = nextNode;  // Actualizar current al siguiente node guardado
            }
        }
        freeListNoDirectory();
        closedir(dir);
        fclose(logs);
    }
}


// Función para recibir un archivo del cliente
void receiveFile(int clientSocket,char *dirName) {
    // Recibir el tamaño del nombre del archivo
    size_t fileName_size;
    if (recv(clientSocket, &fileName_size, sizeof(fileName_size), 0) == -1) {
        perror("Error al recibir el tamaño del nombre del archivo");
        return;
    }

    // Recibir el nombre del archivo
    char fileName[fileName_size + 1];
    if (recv(clientSocket, fileName, fileName_size, 0) == -1) {
        perror("Error al recibir el nombre del archivo");
        return;
    }
    fileName[fileName_size] = '\0';
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s/%s", dirName, fileName);
    FILE* file = fopen(fullpath, "wb");
    if (file == NULL) {
        perror("Error al crear el archivo");
        return;
    }
    // Recibir el tamaño del archivo del cliente
    long fileSize;
    if (recv(clientSocket, &fileSize, sizeof(fileSize), 0) == -1) {
        perror("Error al recibir el tamaño del archivo");
        fclose(file);
        return;
    }

    // Recibir el contenido del archivo en bloques y escribirlo en el archivo
    char buffer[1024];
    size_t bytesReceived;
    while (fileSize > 0) {
        size_t bytesToReceive = sizeof(buffer);
        if (fileSize < sizeof(buffer)) {
            bytesToReceive = fileSize;
        }

        bytesReceived = recv(clientSocket, buffer, bytesToReceive, 0);
        if (bytesReceived == -1) {
            perror("Error al recibir el archivo");
            fclose(file);
            return;
        }

        fwrite(buffer, 1, bytesReceived, file);
        fileSize -= bytesReceived;
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
    int socket_desc, client_sock, c, readSize;
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
    struct clientMessagge messagge;
    while ((readSize = recv(client_sock, &messagge, sizeof(messagge), 0)) > 0) {
        if (strcmp("crear", messagge.proc) == 0) {
            const char* response = "Listo para recibir archivo";
            send(client_sock, response, strlen(response), 0);
            receiveFile(client_sock,dirName);
        } 
        if (strcmp("eliminar", messagge.proc) == 0) {
            const char* response = "Listo para eliminar archivo";
            send(client_sock, response, strlen(response), 0);
            receiveDeleteFile(client_sock,dirName);
        } 
        if (strcmp("modificar", messagge.proc) == 0) {
            recv(client_sock, &messagge, sizeof(messagge), 0);
            checkConflicts(messagge.proc, dirName);
        } 
        if (strcmp("break", messagge.proc) == 0) {
            break;
        } 
    }
    checkDirectory(client_sock,dirName);
    saveDirectory(dirName);
    strncpy(messagge.proc, "break", sizeof(messagge.proc)); //Copia el nombre de la funcion
    send(client_sock, &messagge, sizeof(messagge), 0); //Envia el messagge
    sleep(0.1);
    printf("Directorio actual\n");
    readData(dirName);
    printListNoDirectory();
    printf("La sincronización ha finalizado\n");
    freeListNoDirectory();
    if (readSize == 0) {
        puts("Client disconnected");
        fflush(stdout);
    } else if (readSize == -1) {
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
        int readSize;
        int sock = connectoServer(argv[2]);
        checkDirectory(sock, argv[1]);
        struct clientMessagge messagge;
        strncpy(messagge.proc, "break", sizeof(messagge.proc)); //Copia el nombre de la funcion
        send(sock, &messagge, sizeof(messagge), 0); //Envia el messagge
        sleep(0.1);
        while ((readSize = recv(sock, &messagge, sizeof(messagge), 0)) > 0) {
            if (strcmp("crear", messagge.proc) == 0) {
                const char* response = "Listo para recibir archivo";
                send(sock, response, strlen(response), 0);
                receiveFile(sock,argv[1]);
            } 
            if (strcmp("eliminar", messagge.proc) == 0) {
                const char* response = "Listo para eliminar archivo";
                send(sock, response, strlen(response), 0);
                receiveDeleteFile(sock,argv[1]);
            } 
            if (strcmp("modificar", messagge.proc) == 0) {
                recv(sock, &messagge, sizeof(messagge), 0);
            } 
            if (strcmp("break", messagge.proc) == 0) {
                break;
            } 
        }
        saveDirectory(argv[1]);
        printf("Directorio actual\n");
        readData(argv[1]);
        printListNoDirectory();
        printf("La sincronización ha finalizado\n");
        freeListNoDirectory();
        close(sock);
    }
    return 0;

}
