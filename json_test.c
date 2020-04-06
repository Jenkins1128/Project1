#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct config {
	char key[100];
	char value[100];
};

struct config* create_config(int* count) {
	FILE *fPtr = fopen("myconfig.json", "r");
	int bufferSize = 1000;
	char buffer[bufferSize];
	int totalRead = 0;

	if (fPtr == NULL) {
		perror("Unable to open config file:");
		exit(EXIT_FAILURE);
	}

	while (fgets(buffer, bufferSize, fPtr) != NULL) {
		totalRead = strlen(buffer);

		if (buffer[totalRead - 1] == '\n') {
			buffer[totalRead - 1] = '\0';
		} else {
			buffer[totalRead - 1] = buffer[totalRead - 1];
		}

		(*count)++;
	}
	fclose(fPtr);
	struct config* config_settings = malloc((*count) * sizeof(struct config));
	return config_settings;
}

void populate_config(struct config* settings) {
	int i = 0;

	FILE *fPtr = fopen("myconfig.json", "r");
	int bufferSize = 1000;
	char buffer[bufferSize];
	char key[100];
	char value[100];
	int totalRead = 0;

	if (fPtr == NULL) {
		perror("Unable to open config file:");
		exit(EXIT_FAILURE);
	}

	while (fgets(buffer, bufferSize, fPtr) != NULL) {
		totalRead = strlen(buffer);

		if (buffer[totalRead - 1] == '\n') {
			buffer[totalRead - 1] = '\0';
		} else {
			buffer[totalRead - 1] = buffer[totalRead - 1];
		}

		char delim[3] = ":";
		strcpy(((struct config*)settings + i)->key, strtok(buffer, delim));
		strcpy(((struct config*)settings + i)->value, strtok(NULL, delim));
		++i;
	}
	fclose(fPtr);
	return;
}

char* get_value(struct config* settings, char* key_name, int count) {
	for (int i = 0; i < count; i++) {
		if (strcmp((settings + i)->key, key_name) == 0) {
			return (settings + i)->value;
		}
	}
	return "NaN";
}

int main(void) {
	int settings_count = 0;
	struct config* config_settings = create_config(&settings_count);

	if (config_settings == NULL) {
		perror("Error creating config settings");
		return EXIT_FAILURE;
	}

	populate_config(config_settings);
	for (int i = 0; i < settings_count; i++) {
		printf("Key: %s\tValue: %s\n", (config_settings + i)->key, (config_settings + i)->value);
	}

	printf("KeyName: udp_ttl Value: %s\n", get_value(config_settings, "udp_ttl", settings_count));


	return 0;
}
