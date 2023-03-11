#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>


#define DEFUALT_BUFFER (1024)
#define HEADER_SIZE (4)


typedef struct version_type {
    uint8_t  version:4;
    uint8_t  type:4;
} version_type_t;

typedef struct chat_header {
    version_type_t version_type;
    uint8_t        object;
    uint16_t       body_size;
} chat_header_t;


int copy(int from_fd, int to_fd, size_t count);

int main(int argc, char const *argv[]) {

    int client_socket;
    struct sockaddr_in server;
    char received_size[128];
    int fd;

    // create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        printf("Could not create socket\n");
        return 1;
    }

    // prepare the sockaddr_in structure
//    server.sin_addr.s_addr = inet_addr("192.168.1.85");
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(5050);

    // connect to remote poll-server
    if (connect(client_socket, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("connect failed. Error");
        return 1;
    }

    char choice;

    printf("%s", "[1] - CREATE_USER\n"
                 "[2] - CREATE_AUTH");
    scanf("%c", &choice);

    if (choice == '1') {
        // CREATE USER
        chat_header_t test_header;
        memset(&test_header, 0, sizeof(chat_header_t));
        test_header.version_type.version = 1;
        test_header.version_type.type = 1;
        test_header.object = 1;
        // for testing read_header
        printf("version: %d\n", test_header.version_type.version);
        printf("type: %d\n", test_header.version_type.type);
        printf("object: %d\n", test_header.object);

        char body[] = "bennychao\3benny\3monkey123\0";
        test_header.body_size = (uint16_t) strlen(body);
        uint16_t body_size = test_header.body_size;
        test_header.body_size = htons(test_header.body_size);
        uint32_t temp_int = 0;
        memcpy(&temp_int, &test_header, sizeof(test_header));

        while (1) {
            if (send(client_socket, &temp_int, sizeof(uint32_t), 0) < 0) {
                perror("send");
            }

            if (send(client_socket, body, body_size, 0) < 0) {
                perror("send");
            }

            char buffer[DEFUALT_BUFFER];
            if (read(client_socket, buffer, sizeof(test_header)) < 0) {
                perror("read");
            }
            memset(buffer, '\0', DEFUALT_BUFFER);
            if (read(client_socket, buffer, DEFUALT_BUFFER) < 0) {
                perror("recv");
            }
            printf("res: %s\n", buffer);
            sleep(5);
        }
    } else if (choice == '2') {
        // CREATE_AUTH
        chat_header_t test_header;
        memset(&test_header, 0, sizeof(chat_header_t));
        test_header.version_type.version = 1;
        test_header.version_type.type = 1;
        test_header.object = 4;
        // for testing read_header
        printf("version: %d\n", test_header.version_type.version);
        printf("type: %d\n", test_header.version_type.type);
        printf("object: %d\n", test_header.object);

        char body[] = "bennychao\3monkey123\0";
        test_header.body_size = (uint16_t) strlen(body);
        uint16_t body_size = test_header.body_size;
        test_header.body_size = htons(test_header.body_size);

        uint32_t temp_int = 0;
        memcpy(&temp_int, &test_header, sizeof(test_header));

        while (1) {
            if (send(client_socket, &temp_int, sizeof(uint32_t), 0) < 0) {
                perror("send");
            }

            if (send(client_socket, body, body_size, 0) < 0) {
                perror("send");
            }

            char buffer[DEFUALT_BUFFER];
            if (read(client_socket, buffer, sizeof(test_header)) < 0) {
                perror("read");
            }
            memset(buffer, '\0', DEFUALT_BUFFER);
            if (read(client_socket, buffer, DEFUALT_BUFFER) < 0) {
                perror("recv");
            }
            printf("res: %s\n", buffer);
            sleep(5);
        }
    }

//    printf("temp_int: %d\n", temp_int);
//    temp_int = htonl(temp_int);
//    printf("temp_int after htonl(): %d\n", temp_int);

    // close the file and the socket.
    //close(client_socket);
    return 0;
}

int copy(int from_fd, int to_fd, size_t count)   // NOLINT(bugprone-easily-swappable-parameters)
{
    char *buffer;
    ssize_t rbytes;
    size_t total_rbytes = 0;

    buffer = (char*) malloc(count);

    if(buffer == NULL)
    {
        return EXIT_FAILURE;
    }

    while((rbytes = read(from_fd, buffer, count)) > 0)
    {
        ssize_t wbytes;

        wbytes = write(to_fd, buffer, rbytes);
        total_rbytes += wbytes;

        if(wbytes == -1)
        {
            free(buffer);
            return EXIT_FAILURE;
        }

        if (total_rbytes >= count)
        {
            break;
        }
    }

    free(buffer);
    if(rbytes == -1)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
