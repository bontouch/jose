/* vim: set tabstop=8 shiftwidth=4 softtabstop=4 expandtab smarttab colorcolumn=80: */

#include "jose.h"
#include "conv.h"

#include <string.h>

BIGNUM *
bn_from_buf(const uint8_t buf[], size_t len)
{
    return BN_bin2bn(buf, len, NULL);
}

BIGNUM *
bn_from_json(const json_t *json)
{
    jose_key_t *key = NULL;
    BIGNUM *bn = NULL;

    key = jose_b64_decode_key(json);
    if (key)
        bn = bn_from_buf(key->key, key->len);

    jose_key_free(key);
    return bn;
}

bool
bn_to_buf(const BIGNUM *bn, uint8_t buf[], size_t len)
{
    int bytes = 0;

    if (!bn)
        return false;

    if (len == 0)
        len = BN_num_bytes(bn);

    bytes = BN_num_bytes(bn);
    if (bytes < 0 || bytes > (int) len)
        return false;

    memset(buf, 0, len);
    return BN_bn2bin(bn, &buf[len - bytes]) > 0;
}

json_t *
bn_to_json(const BIGNUM *bn, size_t len)
{
    jose_key_t *key = NULL;
    json_t *out = NULL;

    if (!bn)
        return false;

    if (len == 0)
        len = BN_num_bytes(bn);

    key = jose_key_new(len);
    if (!key)
        return NULL;

    if (bn_to_buf(bn, key->key, len))
        out = jose_b64_encode_key(key);

    jose_key_free(key);
    return out;
}

json_t *
compact_to_obj(const char *compact, ...)
{
    json_t *out = NULL;
    size_t count = 0;
    size_t c = 0;
    va_list ap;

    if (!compact)
        return NULL;

    va_start(ap, compact);
    while (va_arg(ap, const char *))
        count++;
    va_end(ap);

    size_t len[count];

    memset(len, 0, sizeof(len));

    for (size_t i = 0; compact[i]; i++) {
        if (compact[i] != '.')
            len[c]++;
        else if (++c > count - 1)
            return NULL;
    }

    if (c != count - 1)
        return NULL;

    out = json_object();
    if (!out)
        return NULL;

    c = 0;
    va_start(ap, compact);
    for (size_t i = 0; i < count; i++) {
        json_t *val = json_stringn(&compact[c], len[i]);
        if (json_object_set_new(out, va_arg(ap, const char *), val) < 0) {
            json_decref(out);
            va_end(ap);
            return NULL;
        }

        c += len[i] + 1;
    }
    va_end(ap);

    if (json_object_size(out) == 0) {
        json_decref(out);
        return NULL;
    }

    return out;
}