#include "strconv.h"
#include <stdlib.h>
#include <stdint.h>

/* replacement character */
#define U_REPLACEMENT 0xFFFD

/* ---------------- UTF-8 decoding ---------------- */

static uint32_t utf8_decode(const unsigned char** p)
{
    const unsigned char* s = *p;
    uint32_t cp;

    if (s[0] < 0x80) {
        cp = s[0];
        *p += 1;
        return cp;
    }
    if ((s[0] & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
        cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        *p += 2;
        return (cp >= 0x80) ? cp : U_REPLACEMENT;
    }
    if ((s[0] & 0xF0) == 0xE0 &&
        (s[1] & 0xC0) == 0x80 &&
        (s[2] & 0xC0) == 0x80) {
        cp = ((s[0] & 0x0F) << 12) |
             ((s[1] & 0x3F) << 6) |
             (s[2] & 0x3F);
        *p += 3;
        return (cp >= 0x800) ? cp : U_REPLACEMENT;
    }
    if ((s[0] & 0xF8) == 0xF0 &&
        (s[1] & 0xC0) == 0x80 &&
        (s[2] & 0xC0) == 0x80 &&
        (s[3] & 0xC0) == 0x80) {
        cp = ((s[0] & 0x07) << 18) |
             ((s[1] & 0x3F) << 12) |
             ((s[2] & 0x3F) << 6) |
             (s[3] & 0x3F);
        *p += 4;
        return (cp >= 0x10000 && cp <= 0x10FFFF) ? cp : U_REPLACEMENT;
    }

    (*p)++;
    return U_REPLACEMENT;
}

static uint32_t utf8_decode_len(
    const unsigned char** p,
    const unsigned char* end
) {
    const unsigned char* s = *p;

    if (s >= end) return U_REPLACEMENT;

    if (s[0] < 0x80) {
        (*p)++;
        return s[0];
    }

    if ((s[0] & 0xE0) == 0xC0 && s + 1 < end &&
        (s[1] & 0xC0) == 0x80) {
        uint32_t cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        *p += 2;
        return (cp >= 0x80) ? cp : U_REPLACEMENT;
    }

    if ((s[0] & 0xF0) == 0xE0 && s + 2 < end &&
        (s[1] & 0xC0) == 0x80 &&
        (s[2] & 0xC0) == 0x80) {
        uint32_t cp =
            ((s[0] & 0x0F) << 12) |
            ((s[1] & 0x3F) << 6) |
            (s[2] & 0x3F);
        *p += 3;
        return (cp >= 0x800) ? cp : U_REPLACEMENT;
    }

    if ((s[0] & 0xF8) == 0xF0 && s + 3 < end &&
        (s[1] & 0xC0) == 0x80 &&
        (s[2] & 0xC0) == 0x80 &&
        (s[3] & 0xC0) == 0x80) {
        uint32_t cp =
            ((s[0] & 0x07) << 18) |
            ((s[1] & 0x3F) << 12) |
            ((s[2] & 0x3F) << 6) |
            (s[3] & 0x3F);
        *p += 4;
        return (cp >= 0x10000 && cp <= 0x10FFFF)
            ? cp : U_REPLACEMENT;
    }

    (*p)++;
    return U_REPLACEMENT;
}

/* ---------------- UTF-16 encoding ---------------- */

static size_t utf16_len_from_utf8(const char* s)
{
    size_t len = 0;
    const unsigned char* p = (const unsigned char*)s;

    while (*p) {
        uint32_t cp = utf8_decode(&p);
        len += (cp >= 0x10000) ? 2 : 1;
    }
    return len;
}

char16_t* utf8_to_utf16(char16_t* out, char const* input)
{
    if (!input) return NULL;

    size_t len = utf16_len_from_utf8(input);
    if (!out) {
        out = (char16_t*)malloc((len + 1) * sizeof(char16_t));
        if (!out) return NULL;
    }

    const unsigned char* p = (const unsigned char*)input;
    char16_t* dst = out;

    while (*p) {
        uint32_t cp = utf8_decode(&p);
        if (cp <= 0xFFFF) {
            *dst++ = (char16_t)cp;
        } else {
            cp -= 0x10000;
            *dst++ = (char16_t)(0xD800 | (cp >> 10));
            *dst++ = (char16_t)(0xDC00 | (cp & 0x3FF));
        }
    }
    *dst = 0;
    return out;
}

/* ---------------- UTF-16 decoding ---------------- */

static uint32_t utf16_decode(const char16_t** p)
{
    uint32_t w1 = (*p)[0];

    if (w1 >= 0xD800 && w1 <= 0xDBFF) {
        uint32_t w2 = (*p)[1];
        if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
            *p += 2;
            return ((w1 - 0xD800) << 10) + (w2 - 0xDC00) + 0x10000;
        }
        (*p)++;
        return U_REPLACEMENT;
    }
    (*p)++;
    return w1;
}

static size_t utf8_len_from_utf16(const char16_t* s)
{
    size_t len = 0;
    while (*s) {
        uint32_t cp = utf16_decode(&s);
        if (cp < 0x80) len += 1;
        else if (cp < 0x800) len += 2;
        else if (cp < 0x10000) len += 3;
        else len += 4;
    }
    return len;
}

char* utf16_to_utf8(char* out, char16_t const* input)
{
    if (!input) return NULL;

    size_t len = utf8_len_from_utf16(input);
    if (!out) {
        out = (char*)malloc(len + 1);
        if (!out) return NULL;
    }

    char* dst = out;
    const char16_t* p = input;

    while (*p) {
        uint32_t cp = utf16_decode(&p);

        if (cp < 0x80) {
            *dst++ = (char)cp;
        } else if (cp < 0x800) {
            *dst++ = (char)(0xC0 | (cp >> 6));
            *dst++ = (char)(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            *dst++ = (char)(0xE0 | (cp >> 12));
            *dst++ = (char)(0x80 | ((cp >> 6) & 0x3F));
            *dst++ = (char)(0x80 | (cp & 0x3F));
        } else {
            *dst++ = (char)(0xF0 | (cp >> 18));
            *dst++ = (char)(0x80 | ((cp >> 12) & 0x3F));
            *dst++ = (char)(0x80 | ((cp >> 6) & 0x3F));
            *dst++ = (char)(0x80 | (cp & 0x3F));
        }
    }
    *dst = '\0';
    return out;
}

char16_t* utf8_to_utf16_with_len(
    char16_t* out,
    char const* input,
    size_t len
) {
    if (!input) return NULL;

    const unsigned char* p =
        (const unsigned char*)input;
    const unsigned char* end = p + len;

    /* サイズ計算 */
    size_t out_len = 0;
    {
        const unsigned char* t = p;
        while (t < end) {
            uint32_t cp = utf8_decode_len(&t, end);
            out_len += (cp >= 0x10000) ? 2 : 1;
        }
    }

    if (!out) {
        out = (char16_t*)malloc(
            (out_len + 1) * sizeof(char16_t)
        );
        if (!out) return NULL;
    }

    char16_t* dst = out;
    while (p < end) {
        uint32_t cp = utf8_decode_len(&p, end);
        if (cp <= 0xFFFF) {
            *dst++ = (char16_t)cp;
        } else {
            cp -= 0x10000;
            *dst++ = (char16_t)(0xD800 | (cp >> 10));
            *dst++ = (char16_t)(0xDC00 | (cp & 0x3FF));
        }
    }
    *dst = 0;
    return out;
}

char* utf16_to_utf8_with_len(
    char* out,
    char16_t const* input,
    size_t len
) {
    if (!input) return NULL;

    const char16_t* p = input;
    const char16_t* end = input + len;

    /* サイズ計算 */
    size_t out_len = 0;
    {
        const char16_t* t = p;
        while (t < end) {
            uint32_t cp = utf16_decode(&t);
            if (cp < 0x80) out_len += 1;
            else if (cp < 0x800) out_len += 2;
            else if (cp < 0x10000) out_len += 3;
            else out_len += 4;
        }
    }

    if (!out) {
        out = (char*)malloc(out_len + 1);
        if (!out) return NULL;
    }

    char* dst = out;
    while (p < end) {
        uint32_t cp = utf16_decode(&p);

        if (cp < 0x80) {
            *dst++ = (char)cp;
        } else if (cp < 0x800) {
            *dst++ = (char)(0xC0 | (cp >> 6));
            *dst++ = (char)(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            *dst++ = (char)(0xE0 | (cp >> 12));
            *dst++ = (char)(0x80 | ((cp >> 6) & 0x3F));
            *dst++ = (char)(0x80 | (cp & 0x3F));
        } else {
            *dst++ = (char)(0xF0 | (cp >> 18));
            *dst++ = (char)(0x80 | ((cp >> 12) & 0x3F));
            *dst++ = (char)(0x80 | ((cp >> 6) & 0x3F));
            *dst++ = (char)(0x80 | (cp & 0x3F));
        }
    }
    *dst = '\0';
    return out;
}
