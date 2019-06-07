#include <admin.h>
#include <commandParser.h>

int main(int argc, char const *argv[]) {
	enum operation_t operation;
	enum id_t id;
	void *data;
	size_t dataLength;

	/* TODO: receive from argv IP and PORT for the server to administrate */

	enum returnCode_t returnCode = IGNORE;
	do {
		returnCode = parseCommand(&operation, &id, &data, &dataLength);

		free(data);
	} while (returnCode != SEND);

	return 0;
}
