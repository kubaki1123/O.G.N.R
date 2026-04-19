#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

#include <limits.h>

#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT

/* AES */
#define MBEDTLS_AES_C
#define MBEDTLS_CIPHER_MODE_CTR

/* wymagane przez AES */
#define MBEDTLS_CIPHER_C

#endif /* MBEDTLS_CONFIG_H */