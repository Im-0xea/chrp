#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <b64/cencode.h>
#include <cjson/cJSON.h>

#include "api.h"
#include "data.h"
#include "search.h"
#include "xml_boilerplate.h"

#define POST_THREADS 4
#define POST_ALLOC_CHUNK 1024

void *search_api(void * arg) {
	int new_socket = *(int *)arg;
	char * search_term_alloc = malloc(POST_ALLOC_CHUNK);
	ssize_t total_bytes_received = 0;
	ssize_t bytes_received;
	while (1) {
		bytes_received = recv(new_socket, search_term_alloc + total_bytes_received, POST_ALLOC_CHUNK, 0);
		total_bytes_received += bytes_received;
		if (bytes_received == POST_ALLOC_CHUNK) {
			if ((search_term_alloc = realloc(search_term_alloc, total_bytes_received + POST_ALLOC_CHUNK)) == NULL) {
				printf("realloc\n");
				return NULL;
			}
		} else {
			break;
		}
	}
	search_term_alloc[total_bytes_received] = '\0';
	char * search_term = strstr(search_term_alloc, "\r\n\r\n") + 4;
	if (!search_term) {
		printf("invalid post received\n");
		return NULL;
	}
	size_t search_len = total_bytes_received - (search_term - search_term_alloc);

	printf("received request: %s\n", search_term);

	char * response = "HTTP/1.1 200 OK\nContent-Type: text/plain\n\n";
	send(new_socket, response, strlen(response), 0);

	cJSON * json_results[4];
	int ret = search_raw(search_term, search_len, (cJSON **)&json_results);
	free(search_term_alloc);

	if (!ret) {
		cJSON * json_root = cJSON_CreateObject();
		cJSON * json_database_results = cJSON_CreateArray();
		for (int x = 0; x < 4; ++x) {
			cJSON_AddItemToArray(json_database_results, json_results[x]);
		}
		cJSON_AddItemToObject(json_root, "results", json_database_results);
		char * json_text = cJSON_Print(json_root);
		send(new_socket, json_text, strlen(json_text), 0);
		//printf("%s\n", json_text);

		free(json_text);
		cJSON_Delete(json_root);
	}

	close(new_socket);
	pthread_exit(NULL);
}

int search_daemon(const int optind, const int c, char **v, int port)
{
	int server_fd = 0, new_socket = 0, sock_opt = 1;
	struct sockaddr_in address;
	int addrlen = sizeof(address);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
		printf("socket\n");
		return 1;
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &sock_opt, sizeof(sock_opt))) {
		printf("setsockopt\n");
		return 1;
	}

	address.sin_family = AF_INET;
	if (optind == c) {
		address.sin_addr.s_addr = INADDR_ANY;
		printf("address: *.*.*.*\n");
	} else if (optind == c - 1) {
		address.sin_addr.s_addr = inet_addr(v[optind]);
		printf("address: %s\n", v[optind]);
	} else {
		printf("too many arguments\n");
		return 1;
	}
	address.sin_port = htons(port);
	printf("port: %d\n", port);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		printf("bind\n");
		return 1;
	}

	if (listen(server_fd, 3) < 0) {
		printf("listen\n");
		return 1;
	}

	printf("threads: %d\n", POST_THREADS);
	pthread_t threads[POST_THREADS];
	size_t current_thread = 0;
	while (1) {
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
			printf("accept\n");
			return 1;
		}

		if (pthread_create(&threads[current_thread++], NULL, search_api, &new_socket) != 0) {
			perror("pthread_create");
			close(new_socket);
			return 1;
		}

		if (current_thread >= POST_THREADS) {
			current_thread = 0;
			while (current_thread < POST_THREADS) {
				pthread_join(threads[current_thread++], NULL);
			}
			current_thread = 0;
		}
	}
	
	close(server_fd);
}

int flush_json(const char * database, struct data_generic * data, bool print_id, cJSON ** json_ptr)
{
	cJSON * db_json = cJSON_CreateObject();
	*json_ptr = db_json;
	if (!data->id) {
		return 1;
	}
	cJSON_AddStringToObject(db_json, "database_name", database);

	if (data->structure && data->structure_type != NONE) {
		base64_encodestate state_size, state_blob;
		base64_init_encodestate(&state_size);
		base64_init_encodestate(&state_blob);
		size_t base64_size = base64_encode_length(data->structure_size, &state_size);
		char * html_image = NULL;
		if (data->structure_type == PNG) {
			//cJSON_AddStringToObject(db_json, "structure_type", "PNG");
			html_image = malloc(sizeof("data:image/png;base64,") + base64_size);
			strcpy(html_image, "data:image/png;base64,");
			base64_encode_block(data->structure, data->structure_size, html_image + sizeof("data:image/png;base64,") - 1, &state_blob);
		} else if (data->structure_type == SVG) {
			//cJSON_AddStringToObject(db_json, "structure_type", "SVG");
			html_image = malloc(sizeof("data:image/svg+xml;base64,") + base64_size);
			strcpy(html_image, "data:image/svg+xml;base64,");
			base64_encode_block(data->structure, data->structure_size, html_image + sizeof("data:image/svg+xml;base64,") - 1, &state_blob);
		}
		//cJSON_AddStringToObject(db_json, "structure", html_image);
		free(html_image);
		free(data->structure);
	}

	if (print_id) {
		cJSON_AddStringToObject(db_json, "id", data->id);
		free(data->id);
	}

	if (data->name_generic) {
		cJSON_AddStringToObject(db_json, "name_generic", strip_property(data->name_generic));
		free(data->name_generic);
	}

	if (data->name_systematic) {
		cJSON_AddStringToObject(db_json, "name_systematic", strip_property(data->name_systematic));
		free(data->name_systematic);
	}

	if (data->substitution) {
		cJSON_AddStringToObject(db_json, "structure_substitution", strip_property(data->substitution));
		free(data->substitution);
	}

	if (data->molecular_formula) {
		cJSON_AddStringToObject(db_json, "molecular_formula", strip_property(data->molecular_formula));
		free(data->molecular_formula);
	}

	if (data->molecular_weight) {
		cJSON_AddStringToObject(db_json, "molecular_weight", strip_property(data->molecular_weight));
		free(data->molecular_weight);
	}

	if (data->melting_point_count) {
		cJSON * json_melting_points = cJSON_CreateArray();
		for (int x = 0; x < data->melting_point_count; ++x) {
			cJSON_AddItemToArray(json_melting_points, cJSON_CreateString(strip_property(data->melting_points[x])));
			free(data->melting_points[x]);
		}
		cJSON_AddItemToObject(db_json, "melting_points", json_melting_points);
	}
	if (data->melting_points)
		free(data->melting_points);

	if (data->boiling_point_count) {
		cJSON * json_boiling_points = cJSON_CreateArray();
		for (int x = 0; x < data->boiling_point_count; ++x) {
			cJSON_AddItemToArray(json_boiling_points, cJSON_CreateString(strip_property(data->boiling_points[x])));
			free(data->boiling_points[x]);
		}
		cJSON_AddItemToObject(db_json, "boiling_points", json_boiling_points);
	}
	if (data->boiling_points)
		free(data->boiling_points);

	if (data->solubility_count) {
		cJSON * json_solubilities = cJSON_CreateArray();
		for (int x = 0; x < data->solubility_count; ++x) {
			cJSON_AddItemToArray(json_solubilities, cJSON_CreateString(strip_property(data->solubilities[x])));
			free(data->solubilities[x]);
		}
		cJSON_AddItemToObject(db_json, "solubilities", json_solubilities);
	}
	if (data->solubilities)
		free(data->solubilities);

	if (data->warning_count) {
		cJSON * json_warnings = cJSON_CreateArray();
		for (int x = 0; x < data->warning_count; ++x) {
			cJSON_AddItemToArray(json_warnings, cJSON_CreateString(strip_property(data->warnings[x])));
			free(data->warnings[x]);
		}
		cJSON_AddItemToObject(db_json, "warnings", json_warnings);
	}
	if (data->warnings)
		free(data->warnings);

	if (data->psychoactive_properties) {
		cJSON_AddStringToObject(db_json, "psychoactive_effects", strip_property(data->psychoactive_properties));
		free(data->psychoactive_properties);
	}

	return 0;
}
