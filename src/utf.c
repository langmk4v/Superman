#include <stdlib.h>
#include <stdint.h>

typedef uint16_t char16_t;

void utf_free(void* p) { free(p); }

/* UTF-16 → UTF-8 */
char* utf16_to_utf8_c(const char16_t* s, unsigned long len) {
  unsigned long cap = len * 4 + 1;
  char* out = (char*)malloc(cap);
  char* p = out;

  for (unsigned long i = 0; i < len; ++i) {
    uint32_t cp = s[i];

    if (0xD800 <= cp && cp <= 0xDBFF && i + 1 < len) {
      uint32_t low = s[++i];
      cp = ((cp - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
    }

    if (cp <= 0x7F) {
      *p++ = (char)cp;
    } else if (cp <= 0x7FF) {
      *p++ = (char)(0xC0 | (cp >> 6));
      *p++ = (char)(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
      *p++ = (char)(0xE0 | (cp >> 12));
      *p++ = (char)(0x80 | ((cp >> 6) & 0x3F));
      *p++ = (char)(0x80 | (cp & 0x3F));
    } else {
      *p++ = (char)(0xF0 | (cp >> 18));
      *p++ = (char)(0x80 | ((cp >> 12) & 0x3F));
      *p++ = (char)(0x80 | ((cp >> 6) & 0x3F));
      *p++ = (char)(0x80 | (cp & 0x3F));
    }
  }

  *p = '\0';
  return out;
}

/* UTF-8 → UTF-16 */
char16_t* utf8_to_utf16_c(const char* s, unsigned long len) {
  char16_t* out = (char16_t*)malloc((len + 1) * sizeof(char16_t));
  char16_t* p = out;

  for (unsigned long i = 0; i < len;) {
    uint32_t cp;
    unsigned char c = s[i++];

    if (c < 0x80) {
      cp = c;
    } else if ((c >> 5) == 0x6) {
      cp = ((c & 0x1F) << 6) | (s[i] & 0x3F);
      i += 1;
    } else if ((c >> 4) == 0xE) {
      cp = ((c & 0x0F) << 12) | ((s[i] & 0x3F) << 6) | (s[i + 1] & 0x3F);
      i += 2;
    } else {
      cp = ((c & 0x07) << 18) | ((s[i] & 0x3F) << 12) |
           ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
      i += 3;
    }

    if (cp <= 0xFFFF) {
      *p++ = (char16_t)cp;
    } else {
      cp -= 0x10000;
      *p++ = (char16_t)(0xD800 + (cp >> 10));
      *p++ = (char16_t)(0xDC00 + (cp & 0x3FF));
    }
  }

  *p = 0;
  return out;
}
