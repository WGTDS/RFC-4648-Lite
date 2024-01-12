/****************************************************************************
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <https://unlicense.org>
 ***************************************************************************/
/*---------------------------------------------------------------------------
    General Purpose Base[16|32|64] [En|De]coder

    Author  : White Guy That Don't Smile
    Date    : 2024/01/12, Friday, January 12th; 0812 HOURS
    License : UnLicense | Public Domain

    Base16: [0-F]
    Base32: [A-Z 2-7] and [0-9 A-V]
    Base64: [A-Z a-z], [+/] and [-_]
---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

FILE *ifile = NULL;
FILE *ofile = NULL;
char use_padding;
char alphabet[64];

/* Base32 */
const unsigned b32_block_size_enc[6] = { 0, 2, 4, 5, 7, 8 };
const unsigned b32_block_size_dec[7] = { 5, 4, 0, 3, 2, 0, 1 };

/* Base64 */
const unsigned b64_block_size_enc[7] = { 0, 2, 3, 4, 6, 7, 8 };
const unsigned b64_block_size_dec[7] = { 6, 5, 4, 0, 3, 2, 1 };

unsigned char ibuf[8]; /* input buffer */
unsigned char *pi; /* input buffer pointer */
unsigned char obuf[8]; /* output buffer */
unsigned char *po; /* output buffer pointer */

unsigned i; /* iteration */
unsigned n; /* in */
unsigned o; /* out */

static void set_indices(char *p, int c, unsigned amount)
{
    while (amount) {
        *p++ = c++;
        --amount;
    }

    return;
}

static void write_padding(void)
{
    while (o) {
        fputc('=', ofile);
        o--;
    }

    return;
}

static void base16_encode(void)
{
    do {
        if ((n = fread(ibuf, 1, 4, ifile)) == 0) {
            break;
        }

        for (i = 0; i < 4; i++) {
            obuf[i << 1] = alphabet[ibuf[i] >> 4];
            obuf[(i << 1) + 1] = alphabet[ibuf[i] & 15];
        }

        fwrite(obuf, 1, n << 1, ofile);
    } while (1);

    return;
}

static void base16_decode(void)
{
    do {
        if ((n = fread(ibuf, 1, 8, ifile)) == 0) {
            break;
        }

        o = n >> 1;

        for (i = 0; i < 4; i++) {
            if (i < o) {
                char *idx = memchr(alphabet, ibuf[i << 1], 16);

                if (idx == NULL) {
                    goto jump;
                }

                obuf[i] = idx - alphabet;
                obuf[i] <<= 4;
                idx = memchr(alphabet, ibuf[(i << 1) + 1], 16);

                if (idx == NULL) {
                    goto jump;
                }

                obuf[i] |= (idx - alphabet);
            }
            else {
                obuf[i] = 0;
            }
        }

        fwrite(obuf, 1, o, ofile);
    } while (1);
jump:
    return;
}

static void base32_encode(void)
{
    o = 8;

    do {
        if ((n = fread(ibuf, 1, 5, ifile)) == 0) {
            break;
        }

        obuf[0] = ibuf[0] >> 3;
        obuf[1] = ((ibuf[1] >> 6) | ((ibuf[0] & 7) << 2)) & 31;
        obuf[2] = (ibuf[1] >> 1) & 31;
        obuf[3] = (ibuf[2] >> 4) | ((ibuf[1] & 1) << 4);
        obuf[4] = ((ibuf[3] >> 7) | (ibuf[2] << 1)) & 31;
        obuf[5] = (ibuf[3] >> 2) & 31;
        obuf[6] = (ibuf[4] >> 5) | ((ibuf[3] & 3) << 3);
        obuf[7] = ibuf[4] & 31;

        for (i = 0; i < 8; i++) {
            obuf[i] = alphabet[obuf[i]];
        }

        o = b32_block_size_enc[n];
        fwrite(obuf, 1, o, ofile);
        memset(ibuf, 0, 5);
    } while (1);

    if (use_padding == 'Y') {
        o = 8 - o;
        write_padding();
    }

    return;
}

static void base32_decode(void)
{
    do {
        if ((n = fread(ibuf, 1, 8, ifile)) == 0) {
            break;
        }

        o = 8;

        for (i = 0; i < 8; i++) {
            if ((ibuf[i] != '=') && (i < n)) {
                char *idx = memchr(alphabet, ibuf[i], 32);

                if (idx == NULL) {
                    goto jump;
                }

                ibuf[i] = idx - alphabet;
                o--;
            }
            else {
                ibuf[i] = 0;
            }
        }

        obuf[0] = (ibuf[1] >> 2) | (ibuf[0] << 3);
        obuf[1] = (ibuf[3] >> 4) | (ibuf[2] << 1) | (ibuf[1] << 6);
        obuf[2] = (ibuf[4] >> 1) | (ibuf[3] << 4);
        obuf[3] = (ibuf[6] >> 3) | (ibuf[5] << 2) | (ibuf[4] << 7);
        obuf[4] = ibuf[7] | (ibuf[6] << 5);
        fwrite(obuf, 1, b32_block_size_dec[o], ofile);
    } while (1);
jump:
    return;
}

static void base64_encode(void)
{
    o = 8;

    do {
        if ((n = fread(ibuf, 1, 6, ifile)) == 0) {
            break;
        }

        pi = &ibuf[0];
        po = &obuf[0];

        while (po < &obuf[8]) {
            po[0] = pi[0] >> 2;
            po[1] = ((pi[1] >> 4) | ((pi[0] & 3) << 4)) & 63;
            po[2] = ((pi[2] >> 6) | ((pi[1] & 15)) << 2) & 63;
            po[3] = pi[2] & 63;
            pi += 3;
            po += 4;
        }

        for (i = 0; i < 8; i++) {
            obuf[i] = alphabet[obuf[i]];
        }

        o = b64_block_size_enc[n];
        fwrite(obuf, 1, o, ofile);
        memset(ibuf, 0, 6);
    } while (1);

    if (use_padding == 'Y') {
        o = (8 - o) & 3;
        write_padding();
    }

    return;
}

static void base64_decode(void)
{
    do {
        if ((n = fread(ibuf, 1, 8, ifile)) == 0) {
            break;
        }

        o = 8;

        for (i = 0; i < 8; i++) {
            if ((ibuf[i] != '=') && (i < n)) {
                char *idx = memchr(alphabet, ibuf[i], 64);

                if (idx == NULL) {
                    goto jump;
                }

                ibuf[i] = idx - alphabet;
                o--;
            }
            else {
                ibuf[i] = 0;
            }
        }

        pi = &ibuf[0];
        po = &obuf[0];

        while (po < &obuf[6]) {
            po[0] = (pi[1] >> 4) | (pi[0] << 2);
            po[1] = (pi[2] >> 2) | (pi[1] << 4);
            po[2] = pi[3] | (pi[2] << 6);
            pi += 4;
            po += 3;
        }

        fwrite(obuf, 1, b64_block_size_dec[o], ofile);
    } while (1);
jump:
    return;
}

static void time_elapsed(void)
{
    struct tm time;
    unsigned msec;
    unsigned sec;
    char fmt[12];

    msec = clock();
    sec = msec / CLOCKS_PER_SEC;
    memset(fmt, 0, 12);
    memset(&time, 0, sizeof(time));
    time.tm_yday = sec / 86400;
    time.tm_hour = sec / 3600;
    time.tm_min = (sec / 60) % 60;
    time.tm_sec = sec % 60;
    msec %= 1000;
    strftime(fmt, 12, "%Hh:%Mm:%Ss", &time);
    printf("Time Elapsed: %u day(s), %s;%.3ums\n",
           time.tm_yday, fmt, msec);
    return;
}

int main(int argc, char *argv[])
{
    if (argc != 7) {
        printf("Usage: [prog] [options] [infile] [outfile]\n"
               "\tOptions\t: Description\n"
               "\te|d\t: Encode|Decode\n"
               "\ta|b|c\t: Base[16|32|64]\n"
               "\ty|n\t: Padding [Yes|No]\n"
               "\ty|n\t: Alternate Alphabet [Yes|No]\n"
               "\t\t> Base32 -> Extended Hexadecimal\n"
               "\t\t> Base64 -> URL|Filename-safe\n"
               "Arguments Counted: [%d/6]\n", argc - 1);
        return 1;
    }
    else {
        char use_alt_alphabet;
        void (*function)(void);
        void (*base[2])(void);
        char *s;

        if ((s = argv[1], s[1] || strpbrk(s, "DEde") == NULL) ||
            (s = argv[2], s[1] || strpbrk(s, "ABCabc") == NULL) ||
            (s = argv[3], s[1] || strpbrk(s, "NYny") == NULL) ||
            (s = argv[4], s[1] || strpbrk(s, "NYny") == NULL) ||
            (s = argv[5], (ifile = fopen(s, "rb")) == NULL) ||
            (s = argv[6], (ofile = fopen(s, "wb")) == NULL)) {
            printf("Problem with Argument:\n\t\"%s\"\n", s);
            goto fail;
        }

        use_padding = (char)toupper(*argv[3]);
        use_alt_alphabet = (char)toupper(*argv[4]);

        switch ((char)toupper(*argv[2])) {
            case 'A': {
                base[1] = base16_encode;
                base[0] = base16_decode;
                set_indices(&alphabet[0], '0', 10);
                set_indices(&alphabet[10], 'A', 6);
                break;
            }
            case 'B': {
                base[1] = base32_encode;
                base[0] = base32_decode;

                if (use_alt_alphabet == 'Y') {
                    set_indices(&alphabet[0], '0', 10);
                    set_indices(&alphabet[10], 'A', 22);
                }
                else {
                    set_indices(&alphabet[0], 'A', 26);
                    set_indices(&alphabet[26], '2', 6);
                }

                break;
            }
            default: {
                base[1] = base64_encode;
                base[0] = base64_decode;

                set_indices(&alphabet[0], 'A', 26);
                set_indices(&alphabet[26], 'a', 26);
                set_indices(&alphabet[52], '0', 10);

                if (use_alt_alphabet == 'Y') {
                    alphabet[62] = '-';
                    alphabet[63] = '_';
                }
                else {
                    alphabet[62] = '+';
                    alphabet[63] = '/';
                }

                break;
            }
        }

        function = base[(char)toupper(*argv[1]) == 'E'];
        function();
        fflush(ofile);
fail:
        if (ofile != NULL) {
            fclose(ofile);

            if (n != 0) {
                printf("Decoding Error: Illegal character [x%.2X]!\n",
                       ibuf[i]);
                remove(argv[6]);
            }
        }

        if (ifile != NULL) {
            fclose(ifile);
        }

        time_elapsed();
        return 0;
    }
}
