#ifndef UNICODE_ENCODING_H
#define UNICODE_ENCODING_H

// Encode the Unicode codepoint with UTF-8.  The buffer must be at least 4
// bytes in size.
static inline int encodeUtf8(char *out, unsigned int code) {
    if (code < 0x80) {
        out[0] = code;
        return 1;
    } else if (code < 0x800) {
        out[0] = ((code >> 6) & 0x1F) | 0xC0;
        out[1] = ((code >> 0) & 0x3F) | 0x80;
        return 2;
    } else if (code < 0x10000) {
        out[0] = ((code >> 12) & 0x0F) | 0xE0;
        out[1] = ((code >>  6) & 0x3F) | 0x80;
        out[2] = ((code >>  0) & 0x3F) | 0x80;
        return 3;
    } else if (code < 0x110000) {
        out[0] = ((code >> 18) & 0x07) | 0xF0;
        out[1] = ((code >> 12) & 0x3F) | 0x80;
        out[2] = ((code >>  6) & 0x3F) | 0x80;
        out[3] = ((code >>  0) & 0x3F) | 0x80;
        return 4;
    } else {
        // Encoding error
        return 0;
    }
}

// Encode the Unicode codepoint with UTF-8.  The buffer must be at least 2
// elements in size.
static inline int encodeUtf16(wchar_t *out, unsigned int code) {
    if (code < 0x10000) {
        out[0] = code;
        return 1;
    } else if (code < 0x110000) {
        code -= 0x10000;
        out[0] = 0xD800 | (code >> 10);
        out[1] = 0xDC00 | (code & 0x3FF);
        return 2;
    } else {
        // Encoding error
        return 0;
    }
}

static inline unsigned int decodeSurrogatePair(wchar_t ch1, wchar_t ch2) {
    return ((ch1 - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000;
}

#endif // UNICODE_ENCODING_H
