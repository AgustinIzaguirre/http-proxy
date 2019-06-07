#include <admin.h>
#include <commandParser.h>

int main(int argc, char const *argv[]) {
	uint8_t operation;
	uint8_t id;
	void *data;
	size_t dataLength;

	/* TODO: receive from argv IP and PORT for the server to administrate */

	data = malloc(sizeof(void *));

	enum returnCode_t returnCode = IGNORE;
	do {
		returnCode = parseCommand(&operation, &id, &data, &dataLength);

		/* TODO: remove this switch, is for testing commandParser */
		switch (returnCode) {
			case IGNORE:
				printf("IGNORE\n");
				break;
			case INVALID:
				printf("INVALID\n");
				break;
			case NEW:
				printf("NEW\n");
				switch (operation) {
					case BYE_OP:
						printf("BYE\n");
						break;
					case SET_OP:
						printf("SET\n");
						break;
					case GET_OP:
						printf("GET\n");
						break;
				}
				printf("dataLength = %ld\n", dataLength);
				break;
			case SEND:
				printf("SEND\n");
				break;
		}

		free(data);
	} while (returnCode != SEND);

	return 0;
}
