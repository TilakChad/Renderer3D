#include "PNGLoader.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


typedef struct ImageInfo
{
    uint32_t image_width;
    uint32_t image_height;
    uint32_t color_type;
    uint32_t bit_depth;
    uint8_t *image_data;

} ImageInfo;

typedef void (*handle_ptr)(unsigned char *, int);
struct handler
{
    const char *type;
    handle_ptr  func;
};
#define MAX_SIZE (10 * 1024 * 1024)

static void     header_handler(uint8_t *buffer, uint32_t len, ImageInfo *PNGInfo);
static bool     validate_header(const uint8_t *buf);
static uint32_t getBigEndian(uint8_t *lenbuf);
static uint32_t CRC_check(uint8_t *buf, int len);
static uint32_t adler32_checksum(uint8_t *buffer, uint32_t len);
static uint8_t  paethPredictor(uint8_t a, uint8_t b, uint8_t c);

void            background_handler(uint8_t *buffer, uint32_t len);
void            pixelXY_handler(uint8_t *buffer, uint32_t len);
void            palette_generator(uint8_t *buffer, uint32_t len);

// LMAO .. Redefiniton 
uint8_t         average(uint8_t a, uint8_t b);
void            reverse_filter(uint8_t *data, uint32_t len, ImageInfo *PNGInfo);


extern int      deflate(uint8_t *buffer, uint32_t inlen, uint8_t *out, uint32_t *outlen);

// unsigned char *LoadPNGFile(const char *img_path, unsigned *width, unsigned *height, unsigned *no_of_channels,
//                           unsigned *bit_depth)
uint8_t *LoadPNGFromMemory(uint8_t *buffer, uint32_t length, uint32_t *width, uint32_t *height, uint32_t *no_channels,
                           uint32_t *bit_depth);
uint8_t *LoadPNGFromFile(const char *image_path, uint32_t *width, uint32_t *height, uint32_t *no_channels,
                         uint32_t *bit_depth)
{
    FILE *image_file = NULL;
#if _MSC_VER
    fopen_s(&image_file, image_path, "rb");
#else
    image_file = fopen(image_path, "rb");
#endif
    if (!image_file)
    {
        fprintf(stderr, "\nFailed to open %s.. Exiting..", image_path);
        return NULL;
    }
    // Allocate space and read from file
    uint8_t *buf  = malloc(sizeof(uint8_t) * MAX_SIZE);
    #if _MSC_VER
    int size = fread_s(buf, MAX_SIZE, sizeof(*buf), MAX_SIZE, image_file);
    #else
    int      size = fread(buf, sizeof(*buf), MAX_SIZE, image_file);
    #endif
    fclose(image_file);

    if (size == MAX_SIZE)
    {
        fprintf(stderr, "Not enough memory allocated for PNG file\n");
        free(buf);
        return NULL;
    }
    else 
    {
        if(ferror(image_file))
        {
            perror("Something's wrong..");
        }
        else if (feof(image_file))
        {
            fprintf(stderr, "Unexpected end'0");
        }
    }

    uint8_t *image_data = LoadPNGFromMemory(buf, size, width, height, no_channels, bit_depth);
    free(buf);
    return image_data;
}

uint8_t *LoadPNGFromMemory(uint8_t *buffer, uint32_t length, uint32_t *width, uint32_t *height, uint32_t *no_channels,
                           uint32_t *bit_depth)
{
    const struct handler handlers[] = {
        {"bKGD", background_handler}, {"pHYs", pixelXY_handler}, {"PLTE", palette_generator}, {NULL, NULL}};

    // Allocate buffer for deflate stream
    // This stream is formed by concatenating chunk of IDAT in a sequential manner
    uint8_t *deflate_stream = malloc(sizeof(uint8_t) * MAX_SIZE);
    uint32_t deflate_len    = 0;

    validate_header(buffer);
    // Advance stream to the 8th bit where header finished

    uint32_t  pos = 8;
    uint32_t  len = 0;

    ImageInfo PNGInfo;

    while (1)
    {
        unsigned char lengthbuf[4];
        memcpy(lengthbuf, buffer + pos, 4);

        char chunkbuf[5] = {'\0'};
        len              = getBigEndian(lengthbuf);
        memcpy(chunkbuf, buffer + pos + 4, 4);

        if (!strcmp(chunkbuf, "IHDR"))
        {
            header_handler(buffer + pos + 8, len, &PNGInfo);
        }
        else if (!strcmp(chunkbuf, "IDAT"))
        {
            memcpy(deflate_stream + deflate_len, (buffer + pos + 8), len);
            deflate_len += len;
        }
        else
            for (int i = 0; handlers[i].type != NULL; ++i)
            {
                if (!strcmp(handlers[i].type, chunkbuf))
                {
                    handlers[i].func(buffer + pos + 8, len);
                    break;
                }
            }

        if (!(CRC_check(buffer + pos + 4, len + 4) == getBigEndian(buffer + pos + 8 + len)))
        {
            fprintf(stderr, "Failed to verify CRC checksum...\n");
            free(deflate_stream);
            return NULL;
        }

        pos += len + 12;
        if (!strcmp(chunkbuf, "IEND"))
            break;
    }

    // Allocate space that will be used by the output uncompressed stream with max size SIZE
    uint8_t *decomp_data = malloc(sizeof(uint8_t) * MAX_SIZE);
    uint32_t decomp_len  = MAX_SIZE;

    // It decompresses the deflate stream, writes it into decomp_data and writes total length of output stram in
    // decomp_len integer
    // skip first and second bytes and pass it to the deflate decompressor
    if (deflate(deflate_stream + 2, deflate_len, decomp_data, &decomp_len))
    {
        fprintf(stderr, "Failed the deflate decompression...");
        free(deflate_stream);
        free(decomp_data);
        return NULL;
    }

    fprintf(stderr, "\nDecomped len is : %ud.", decomp_len);

    // Apply adler32 checksum on the decomp_data and vertify it
    unsigned char *at_last        = deflate_stream + deflate_len;
    uint32_t       stored_adler32 = at_last[-4] << 24 | at_last[-3] << 16 | at_last[-2] << 8 | at_last[-1]; // :D :D
    uint32_t       calc_adler32   = adler32_checksum(decomp_data, decomp_len);

    if (stored_adler32 != calc_adler32)
    {
        fprintf(stderr, "Adler32 check not passed i.e failed.\n");
        return NULL;
    }

    reverse_filter(decomp_data, decomp_len, &PNGInfo);

    // image_info have everything that is needed by other programs
    free(deflate_stream);
    free(decomp_data);

    // Fill in the parameters required values
    *width  = PNGInfo.image_width;
    *height = PNGInfo.image_height;

    if (PNGInfo.color_type == 2)
        *no_channels = 3;
    else if (PNGInfo.color_type == 6)
        *no_channels = 4;
    else // Not implemented other channels filtering
        *no_channels = 0;

    *bit_depth = PNGInfo.bit_depth;
    return PNGInfo.image_data;
}

bool validate_header(const uint8_t *buf)
{
    // Validate first 4 bytes of the header.
    return buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G';
}

// uses upcoming 4 bytes to form it into 32 bit integer with network order
uint32_t getBigEndian(uint8_t *lenbuf)
{
    uint32_t v = 0, temp = 0;
    uint32_t ch = 0;
    int      k  = 24;
    for (int i = 0; i < 4; ++i)
    {
        ch   = lenbuf[i];
        temp = ch << k;
        k    = k - 8;
        v    = v | temp;
    }
    return v;
}

void header_handler(uint8_t *buffer, uint32_t len, ImageInfo *PNGInfo)
{
    assert(len == 13);
    PNGInfo->image_width  = getBigEndian(buffer);
    PNGInfo->image_height = getBigEndian(buffer + 4);
    PNGInfo->bit_depth    = buffer[8];
    PNGInfo->color_type   = buffer[9];
}

void background_handler(uint8_t *buffer, uint32_t len)
{
    printf("Background Color info :  \n");
    int32_t r = 0, g = 0, b = 0;
    r |= (buffer[0] << 8);
    r |= buffer[1];
    g |= buffer[2] << 8;
    g |= buffer[3];
    b |= buffer[4] << 8;
    b |= buffer[5];
    printf("\tRed color is : %02x.\n", r);
    printf("\tGreen color is : %02x.\n", g);
    printf("\tBlue color is %02x.\n", b);
}

void pixelXY_handler(uint8_t *buffer, uint32_t len)
{
    printf("\tPixel per unit, X axis : %u.\n", getBigEndian(buffer));
    printf("\tPixel per unit, Y axis : %u.\n", getBigEndian(buffer + 4));
    printf("\tUnit specifier : %u.\n", (unsigned char)*(buffer + 8));
}

void palette_generator(uint8_t *buffer, uint32_t len)
{
    for (int i = 0; i < len; i += 3)
    {
        printf("%02x %02x %02x   ", (uint8_t)buffer[i], (uint8_t)buffer[i + 1],
               (uint8_t)buffer[i + 2]);
    }
}

uint32_t CRC_check(uint8_t *buf, int len)
{
    const uint32_t POLY   = 0xEDB88320; // Straight copied
    const uint8_t *buffer = buf;
    uint32_t       crc    = -1;

    while (len--)
    {
        crc = crc ^ *buffer++;
        for (int bit = 0; bit < 8; bit++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ POLY;
            else
                crc = (crc >> 1);
        }
    }
    return ~crc;
}

uint32_t adler32_checksum(uint8_t *buffer, uint32_t len)
{
    const uint32_t adler_mod = 65521; // smallest prime less than 2^16-1
    uint16_t       a         = 1;
    uint16_t       b         = 0;
    for (int i = 0; i < len; ++i)
    {
        a = (a + *(buffer + i)) % adler_mod;
        b = (b + a) % adler_mod;
    }
    return (b << 16) | a;
}

void reverse_filter(uint8_t *data, uint32_t len, ImageInfo *PNGInfo)
{
    // Demo for only color_type 6 i.e true color with alpha channel
    assert(PNGInfo->color_type == 2 || PNGInfo->color_type == 6);

    int byteDepth; // For true color with alpha channel

    if (PNGInfo->color_type == 2)
        byteDepth = 3;
    else
        byteDepth = 4;

    unsigned char *prev_scanline = malloc(sizeof(char) * PNGInfo->image_width * byteDepth);
    for (int i = 0; i < PNGInfo->image_width; ++i)
        prev_scanline[i] = 0;

    unsigned char *      final_data = malloc(sizeof(char) * PNGInfo->image_width * PNGInfo->image_height * byteDepth);
    unsigned char *      original_data = final_data;

    const unsigned char *filter_data   = data;

    unsigned char        a = 0, b = 0, c = 0, x;
    int                  pos       = 0;
    int                  line_byte = PNGInfo->image_width * byteDepth;
    unsigned char        ch;

    int                  count = 0;

    for (int height = 0; height < PNGInfo->image_height; ++height)
    {
        ch  = *filter_data;
        pos = 1;
        a = b = c = x = 0;
        for (int seek = 0; seek < line_byte; ++seek)
        {
            count++;
            if (pos <= byteDepth)
            {
                c = 0;
                a = 0;
            }
            else
            {
                c = prev_scanline[pos - byteDepth - 1];
                a = *(original_data - byteDepth);
            }
            b = prev_scanline[pos - 1];
            x = filter_data[pos];
            if (ch == 0)
                x = x;
            else if (ch == 1)
                x = x + a;
            else if (ch == 2)
                x = x + b;
            else if (ch == 3)
                x = x + average(a, b);
            else if (ch == 4)
                x = x + paethPredictor(a, b, c);
            else
            {
                fprintf(stderr, "Invalid filter type");
                exit(1);
            }
            pos++;
            *(original_data++) = x;
        }
        memcpy(prev_scanline, original_data - line_byte, line_byte);
        filter_data += line_byte + 1;
    }

    PNGInfo->image_data = final_data;
    fprintf(stderr, "Total count here is %u.", count);
    free(prev_scanline);
}

uint8_t average(uint8_t a, uint8_t b)
{
    uint16_t c = a + b;
    return c / 2;
}

uint8_t paethPredictor(uint8_t a, uint8_t b, uint8_t c)
{
    uint32_t p  = a + b - c;
    uint32_t pa = abs(p - a);
    uint32_t pb = abs(p - b);
    uint32_t pc = abs(p - c);
    uint8_t  pr;

    if (pa <= pb && pa <= pc)
        pr = a;
    else if (pb <= pc)
        pr = b;
    else
        pr = c;
    return pr;
}

void DrawImage(const char* img,uint8_t* target_buffer,uint32_t target_width, uint32_t target_height, uint32_t target_channels)
{
    uint32_t width, height, channels, bit_depth; 
    uint8_t *mem = LoadPNGFromFile(img, &width, &height, &channels, &bit_depth);

    if (target_width < width || target_height < height) 
        return ; 
    // Else copy the content of image into the buffer 
    // From top left corner 
    int stride = target_width * target_channels;
    uint8_t *imgloc = target_buffer;

    for (int h = 0; h < height; ++h)
    {
        imgloc = target_buffer + h * stride;    
        for (int w = 0; w < width; ++w)
        {
            *imgloc++ = mem[2];
            *imgloc++ = mem[1]; 
            *imgloc++ = mem[0];
            imgloc++;
            mem += 3; 
        }
    }
}