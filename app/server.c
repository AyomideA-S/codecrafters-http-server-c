/**
 * @file server.c
 * @author Ayomide Ayodele-Soyebo (midesuperbest@gmail.com)
 * @brief A simple HTTP server that listens on port 4221 and sends a 200 OK response to any client that connects to it.
 * @version 0.1
 * @date 2024-03-18
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

char *response_ok = "HTTP/1.1 200 OK\r\n\r\n";
char *response_not_found = "HTTP/1.1 404 Not Found\r\n\r\n";

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Since the tester restarts your program quite often, setting REUSE_PORT
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(4221),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	
	const int client_fd =
		accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
	if (client_fd < 0) {
		printf("Accept failed: %s \n", strerror(errno));
		return 1;
	}

	printf("Client connected\n");

	char request_buffer[BUFFER_SIZE];
	if (read(client_fd, request_buffer, BUFFER_SIZE) < 0) {
		printf("Read failed: %s \n", strerror(errno));
		return 1;
	} else {
		printf("Request from client: %s\n", request_buffer);
	}

	char *start_line = strtok(request_buffer, "\r\n");
	if (start_line == NULL) {
		printf("Invalid request\n");
		exit(1);
	}

	char *http_method = strtok(start_line, " ");
	char *path = strtok(NULL, " ");
	char *http_version = strtok(NULL, " ");
	if (http_method == NULL || path == NULL || http_version == NULL) {
		printf("Invalid request\n");
		exit(1);
	}

	char *data = strstr(path, "/echo/");

	if (data != NULL) {
		char *content = data + strlen("/echo/");
		char *responseFormat = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s";
		char response[1024];
		sprintf(response, responseFormat, strlen(content), content);
		send(client_fd, response, strlen(response), 0);
	} else {
		if (strcmp(path, "/") == 0)
			write(client_fd, response_ok, strlen(response_ok));
		else
			write(client_fd, response_not_found, strlen(response_not_found));
	}

	close(server_fd);
	close(client_fd);

	return 0;
}
