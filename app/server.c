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
#include <pthread.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

char *response_ok = "HTTP/1.1 200 OK\r\n\r\n";
char *response_not_found = "HTTP/1.1 404 Not Found\r\n\r\n";

/**
 * @brief This function handles the client request
 *
 * @param client_socket The file descriptor for the client socket
 * @return void*
 */
void *handle_client(int client_socket) {
	int client_fd = client_socket;

	char buffer[BUFFER_SIZE];
	char request_buffer[BUFFER_SIZE];
	char *response = NULL;

	uint32_t bytes_read = 0;
	bytes_read = read(client_fd, buffer, BUFFER_SIZE);
	if (bytes_read < 1) {
		printf("Read failed: %s\n", strerror(errno));
		return (void *)1;
	} else {
		strncpy(request_buffer, buffer, BUFFER_SIZE - 1); /* Use strncpy to avoid buffer overflow */
		request_buffer[BUFFER_SIZE - 1] = '\0';			  /* Ensure null-termination */
		printf("Client #%d: \n", client_fd);
		printf("Request from client: %s\n", request_buffer);
	}

	char *start_line = strtok(request_buffer, "\r\n");
	char *host = strtok(NULL, "\r\n");
	char *request_line = host;
	char *user_agent;
	while (request_line != NULL) {
		if (strstr(request_line, "User-Agent") != NULL) {
			user_agent = request_line;
			break;
		}
		request_line = strtok(NULL, "\r\n");
	}

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

	char *data;
	char *content;
	int bytes_written = 0;
	if ((data = strstr(path, "/echo/")) != NULL) {
		content = data + strlen("/echo/");
		char *responseFormat = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s";
		response = (char *)malloc(BUFFER_SIZE);
		bytes_written = snprintf(response, BUFFER_SIZE, responseFormat, strlen(content), content);
		if (bytes_written < 0 || bytes_written >= BUFFER_SIZE) {
			printf("Failed to create response\n");
			free(response);
			response = NULL;
		}
		send(client_fd, response, strlen(response), 0);
	} else if ((data = strstr(path, "/user-agent")) != NULL) {
		content = user_agent + strlen("User-Agent: ");
		char *responseFormat = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s";
		response = (char *)malloc(BUFFER_SIZE);
		bytes_written = snprintf(response, BUFFER_SIZE, responseFormat, strlen(content), content);
		if (bytes_written < 0 || bytes_written >= BUFFER_SIZE) {
			printf("Failed to create response\n");
			free(response);
			response = NULL;
		}
		send(client_fd, response, strlen(response), 0);
	} else {
		if (strcmp(path, "/") == 0)
			write(client_fd, response_ok, strlen(response_ok));
		else
			write(client_fd, response_not_found, strlen(response_not_found));
	}

	/** Close the socket
	 * Returns 0 on success, -1 on error
	 */
	close(client_fd);

	return (void *)0;
}

/**
 * @brief The main function
 *
 * @return int 0 on success, 1 on error
 */
int main() {
	// Disable output buffering
	setbuf(stdout, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	int server_fd, client_addr_len, client_fd;
	struct sockaddr_in client_addr;

	/** Create a socket
	 * AF_INET: Address Family for IPv4
	 * SOCK_STREAM: Provides sequenced, reliable, two-way, connection-based byte streams
	 * 0: Protocol value for Internet Protocol (IP)
	 * Returns a file descriptor for the socket (a non-negative integer)
	 * On error, -1 is returned
	 */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}

	/** Since the tester restarts your program quite often, setting REUSE_PORT
	 * ensures that we don't run into 'Address already in use' errors
	 * setsockopt() is used to set the socket options
	 * SO_REUSEPORT: Allows multiple sockets to be bound to the same port
	 * &reuse: Pointer to the value of the option
	 */
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
		return 1;
	}

	/** create a sockaddr_in struct for the server
	 * sin_family: Address family (AF_INET)
	 * sin_port: Port number (in network byte order) / htons converts the port number to network byte order
	 * sin_addr: IP address (INADDR_ANY: Accept any incoming messages) / htonl converts the IP address to network byte order
	 */
	struct sockaddr_in serv_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(4221),
		.sin_addr = {htonl(INADDR_ANY)},
	};

	/** Bind the socket to the address and port number specified in serv_addr
	 * Returns 0 on success, -1 on error
	 */
	if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}

	/** Listen for incoming connections
	 * connection_backlog: Maximum length of the queue of pending connections
	 * Returns 0 on success, -1 on error
	 */
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

	uint16_t thread_count = 0;

	printf("Waiting for a client to connect...\n");

	while (true) {
		/** Accept a connection
		 * client_addr: Address of the client
		 * client_addr_len: Length of the client address
		 * Returns a file descriptor for the client socket (a non-negative integer)
		 * On error, -1 is returned
		 */
		client_addr_len = sizeof(client_addr);

		/** Returns a file descriptor for the accepted socket (a non-negative integer)
		 * On error, -1 is returned
		 */
		if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
			printf("Accept failed: %s \n", strerror(errno));
			return 1;
		}

		printf("Client connected\n");
		pthread_t thread;

		if (!fork()) {
			close(server_fd);
			handle_client(client_fd);
			close(client_fd);
			exit(0);
		}
		thread_count++;
	}

	close(server_fd);

	return 0;
}
