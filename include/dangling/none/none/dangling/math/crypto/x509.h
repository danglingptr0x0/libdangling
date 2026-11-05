#ifndef LDG_MATH_CRYPTO_X509_H
#define LDG_MATH_CRYPTO_X509_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_MATH_CRYPTO_X509_BUFF_CUNT 898
#define LDG_MATH_CRYPTO_X509_PATH_LEN_NONE UINT32_MAX
#define LDG_MATH_CRYPTO_X509_USAGE_SIG 0x0080U
#define LDG_MATH_CRYPTO_X509_USAGE_KEY_ENC 0x0020U
#define LDG_MATH_CRYPTO_X509_USAGE_KEY_AGREE 0x0008U
#define LDG_MATH_CRYPTO_X509_USAGE_CERT_SIGN 0x0004U

typedef enum ldg_math_crypto_x509_sig
{
    LDG_MATH_CRYPTO_X509_SIG_RSA_SHA256,
    LDG_MATH_CRYPTO_X509_SIG_RSA_SHA384,
    LDG_MATH_CRYPTO_X509_SIG_RSA_SHA512,
    LDG_MATH_CRYPTO_X509_SIG_ECDSA_SHA256,
    LDG_MATH_CRYPTO_X509_SIG_ECDSA_SHA384,
    LDG_MATH_CRYPTO_X509_SIG_ECDSA_SHA512,
    LDG_MATH_CRYPTO_X509_SIG_CUNT
} ldg_math_crypto_x509_sig_t;

typedef enum ldg_math_crypto_x509_key
{
    LDG_MATH_CRYPTO_X509_KEY_RSA,
    LDG_MATH_CRYPTO_X509_KEY_P256,
    LDG_MATH_CRYPTO_X509_KEY_P384,
    LDG_MATH_CRYPTO_X509_KEY_SECP256K1,
    LDG_MATH_CRYPTO_X509_KEY_CUNT
} ldg_math_crypto_x509_key_t;

typedef struct ldg_math_crypto_x509_cert
{
    const uint8_t *tbs;
    const uint8_t *sig;
    const uint8_t *issuer;
    const uint8_t *subject;
    const uint8_t *key;
    const uint8_t *exponent;
    const uint8_t *alt_name;
    uint64_t not_before;
    uint64_t not_after;
    uint32_t tbs_len;
    uint32_t sig_len;
    uint32_t issuer_len;
    uint32_t subject_len;
    uint32_t key_len;
    uint32_t exponent_len;
    uint32_t alt_name_len;
    ldg_math_crypto_x509_sig_t sig_alg;
    ldg_math_crypto_x509_key_t key_alg;
    uint32_t path_len_max;
    uint16_t key_usage;
    uint8_t ca_is;
    uint8_t key_usage_has;
    uint8_t ext_usage_has;
    uint8_t ext_usage_server_is;
    uint8_t pudding[2];
} ldg_math_crypto_x509_cert_t;

LDG_EXPORT uint32_t ldg_math_crypto_x509_cert_parse(const uint8_t *der, uint32_t len, ldg_math_crypto_x509_cert_t *out);
LDG_EXPORT uint8_t ldg_math_crypto_x509_cert_signed_is(const ldg_math_crypto_x509_cert_t *cert, const ldg_math_crypto_x509_cert_t *issuer, uint32_t *buff, uint32_t buff_cunt);
LDG_EXPORT uint8_t ldg_math_crypto_x509_cert_name_has(const ldg_math_crypto_x509_cert_t *cert, const char *host, uint32_t host_len);
LDG_EXPORT uint8_t ldg_math_crypto_x509_cert_current_is(const ldg_math_crypto_x509_cert_t *cert, uint64_t now);
LDG_EXPORT uint8_t ldg_math_crypto_x509_chain_valid_is(const ldg_math_crypto_x509_cert_t *chain, uint32_t chain_cunt, const ldg_math_crypto_x509_cert_t *anchor, uint32_t anchor_cunt, const char *host, uint32_t host_len, uint64_t now, uint32_t *buff, uint32_t buff_cunt);

#endif
