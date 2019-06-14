#ifndef MEDIA_RANGE_H
#define MEDIA_RANGE_H

#include <stdlib.h>
#include <stdio.h> //TODO: for test

enum matchResult { NO, YES, ALL };
#define BLOCK 10

typedef struct MediaRanges *MediaRangePtr_t;

/**
 *  Initialize a new mediaRange, from another media range
 */
MediaRangePtr_t createMediaRangeFromListOfMediaType(MediaRangePtr_t mediaRange);

/**
 *  Initialize a mediaRange, the string must respect the format of the
 *  section 5.3.2 of the RFC7231
 */
MediaRangePtr_t createMediaRange(char const *string);

/**
 *  Add a mediaRange to the list, the string must respect the format of the
 *  section 5.3.2 of the RFC7231
 */
void addMediaRange(MediaRangePtr_t mediaRange, char const *string);

/**
 *  Returns if a mediaType (section 3.1.1.1 of the RFC7231) until the n
 *  position can match the media range.
 *  WARNING: Before using with n it is expected that you already use
 *  it with n-1 and if you want to use it with another media type you
 *  must use the reset function
 */
enum matchResult doesMatchAt(int n, char mediaTypeCharAtN,
							 MediaRangePtr_t mediaRange);

/**
 *  Reset the structure so it can be use for another media types
 *  WARNING: it lose the progress of the doesMatchAt function
 */
void resetMediaRange(MediaRangePtr_t mediaRange);

void printMediaRange(MediaRangePtr_t mediaRange); // TODO: for test

#endif
