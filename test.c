#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage : %s <ip> <port>\n", argv[0]);
        return 1;
    }
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));


    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect()");
        exit(1);
    }

    // sendto(sock, "HELLO\0", 10, 0, (struct sockaddr*)&addr, sizeof(addr));
    printf("finished\n");
    close(sock);
    return 0;
}