#ifndef ADMIN_H
#define ADMIN_H
#include <stdlib.h>

enum id_t {
	NO_ID,
	MIME_ID,
	BF_ID,
	CMD_ID,
	MTR_CN_ID,
	MTR_HS_ID,
	MTR_BT_ID,
	MTR_ID,
	TF_ID
};
typedef enum id_t id_t;

#define ON 0x01
#define OFF 0x00

#define DEFAULT_PORT 8069
#define DEFAULT_IP "127.0.0.1"

#define STREAM_QUANTITY 2
#define GET_STREAM 0
#define SET_STREAM 1
#define BYE_STREAM 1

#define ID_QUANTITY 9

#endif
