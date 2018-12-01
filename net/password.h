#ifndef HERO_PASSWORD_H_
#define HERO_PASSWORD_H_


#define SHA1_HASH_SIZE 20 /* Hash size in bytes */
#define SCRAMBLE_LENGTH 20

#include <sys/types.h>

typedef unsigned char   uint8;
typedef signed char int8;
typedef short int16;
typedef unsigned char uchar;
typedef unsigned long long  int ulonglong;
typedef unsigned int uint32;

enum sha_result_codes
{
  SHA_SUCCESS = 0,
  SHA_NULL,		/* Null pointer parameter */
  SHA_INPUT_TOO_LONG,	/* input data too long */
  SHA_STATE_ERROR	/* called Input after Result */
};

typedef struct SHA1_CONTEXT{

  ulonglong  Length;		/* Message length in bits      */
  uint32 Intermediate_Hash[SHA1_HASH_SIZE/4]; /* Message Digest  */
  int Computed;			/* Is the digest computed?	   */
  int Corrupted;		/* Is the message digest corrupted? */
  int16 Message_Block_Index;	/* Index into message block array   */
  uint8 Message_Block[64];	/* 512-bit message blocks      */

} SHA1_CONTEXT;

void scramble(char *to, const char *message, const char *password);

#endif
