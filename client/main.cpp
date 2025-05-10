#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

int main () {
    struct sockaddr_un *servaddr = NULL;
    int sockserver;

    const char *path= "../mangohud.sock";
    char buffer[50];
    const char *string = "Hello from client";

    /* Part 1 – create the socket */
    sockserver = socket(AF_UNIX, SOCK_SEQPACKET, 0);

    if (sockserver < 0) {
        printf("creating client socket failed\n");
        exit(0);
    }

    servaddr = (struct sockaddr_un *)malloc(sizeof(struct sockaddr_un));

    if(servaddr == NULL) {
        printf("unable to allocate memory\n");
        goto end;
    }

    servaddr->sun_family = AF_UNIX;

    if ((strlen(path)) > sizeof(servaddr->sun_path)) {
        printf("Path is too long\n");
        goto end1;
    }

    snprintf(servaddr->sun_path, (strlen(path)+1), "%s", path);

    if (0 != (connect(sockserver, (struct sockaddr *)servaddr, sizeof(struct sockaddr_un)))) {
        printf("unable to connect to the server\n");
        goto end1;
    }

    /* read and write data from/to socket */
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, (strlen(string) + 1), "%s", string);
        write(sockserver, buffer, sizeof(buffer));

        sleep(5);

        memset(buffer, 0, sizeof(buffer));
        read(sockserver, buffer, sizeof(buffer));
        printf("%s\n", buffer);
    }
end1:
    free(servaddr);
end:
    close(sockserver);
    return 0;
}
