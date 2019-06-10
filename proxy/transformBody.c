#include <stdint.h>
#include <buffer.h>
#include <httpProxyADT.h>
#include <transformBody.h>
#include <configuration.h>
#include <handleParsers.h>

void transformBodyInit(const unsigned state, struct selector_key *key) {
	// struct transformBody *transformBody =
	// getTransformBodyState(GET_DATA(key));
	printf("arrived to transform body state\n");
	buffer_reset(getReadBuffer(GET_DATA(key)));
}

unsigned transformBodyRead(struct selector_key *key) {
	return ERROR;
}
unsigned transformBodyWrite(struct selector_key *key) {
	return ERROR;
}

int executeTransformCommand(struct selector_key *key) {
	httpADT_t state						= GET_DATA(key);
	struct transformBody *transformBody = getTransformBodyState(state);
	int inputPipe[]						= {-1, -1};
	int outputPipe[]					= {-1, -1};
	int errorFd =
		-1; // TODO open /dev/null or the setted in config and dup2 stderr
	char *commandPath = getCommand(getConfiguration());
	pid_t commandPid;

	if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1) {
		return PIPE_CREATION_ERROR;
	}

	commandPid = fork();
	if (commandPid == -1) {
		return FORK_ERROR;
	}
	else if (commandPid == 0) {
		dup2(inputPipe[0], 0);  // setting pipe as stdin
		dup2(outputPipe[1], 1); // setting pipe as stdout
		close(inputPipe[0]);	// closing unused copy of pipe
		close(outputPipe[1]);   // closing unused copy o pipe
		close(inputPipe[1]);	// closing write end of input pipe
		close(outputPipe[0]);   // closing read end of output pipe

		if (execl("/bin/sh", "sh", "-c", commandPath, (char *) 0) == -1) {
			// closing other pipes end
			close(inputPipe[0]);
			close(outputPipe[1]);
			return EXEC_ERROR;
		}
	}
	else {
		// In father process
		close(inputPipe[0]);  // closing read end of input pipe
		close(outputPipe[1]); // closing write end of output pipe
		transformBody->writeToTransformFd  = inputPipe[1];
		transformBody->readFromTransformFd = outputPipe[0];

		if (SELECTOR_SUCCESS !=
				selector_set_interest(key->s, inputPipe[1], OP_WRITE) ||
			SELECTOR_SUCCESS !=
				selector_set_interest(key->s, outputPipe[0], OP_READ)) {
			return SELECT_ERROR;
		}
		// TODO register fd on selector
	}

	return TRANSFORM_COMMAND_OK;
}