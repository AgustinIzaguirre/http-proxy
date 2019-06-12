#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>
#include <logger.h>

int fileExists(const char *file) {
  struct stat buffer;
  return (stat(file, &buffer) == 0);
}

void testCreateLogFile() {
  // Before
  char *testLog = "./test/access.log";

  // When
  createLogFile();

  // Assert
  assert(fileExists(testLog));

  // After
  remove(testLog);
}

void testAddLogEntry() {
  // When
  // sending http request

  // Assert
  // a new entry has been added for the http request
}
