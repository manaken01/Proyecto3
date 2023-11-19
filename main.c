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
    DIR *dir;
	 
	dir = opendir(".");
	
    /* Loop through directory entries. */
    while ((dp = readdir(dir)) != NULL) {

        /* Get entry's information. */
        if (stat(dp->d_name, &statbuf) == -1)
            continue;

        /* Print out type, permissions, and number of links. */
        //printf("%10.10s", sperm (statbuf.st_mode));
        printf("%4d", statbuf.st_nlink);

        /* Print out owner's name if it is found using getpwuid(). */
        if ((pwd = getpwuid(statbuf.st_uid)) != NULL)
            printf(" %-8.8s", pwd->pw_name);
        else
            printf(" %-8d", statbuf.st_uid);

        /* Print out group name if it is found using getgrgid(). */
        if ((grp = getgrgid(statbuf.st_gid)) != NULL)
            printf(" %-8.8s", grp->gr_name);
        else
            printf(" %-8d", statbuf.st_gid);

        /* Print size of file. */
        printf(" %9jd", (intmax_t)statbuf.st_size);
        tm = localtime(&statbuf.st_mtime);
        /* Get localized date string. */
        strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm);
        printf(" %s %s\n", datestring, dp->d_name);
    }
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