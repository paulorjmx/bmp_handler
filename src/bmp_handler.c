#include <bmp_handler.h>
#include <error_handler.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define BMP_SIG 0x4D42 // Bitmap file identification
#define SQRT_2 1.414214 // Calculated square root of 2
#define EOB -3000 // End Of Block Macro

// Structure used like a buffer to write in a file
typedef struct t_buffer
{
    unsigned long buffer; // Buffer size of 8 bytes long
    unsigned char remaining_bits; // Quantity of free bits in buffer
} BUFFER;

unsigned int ERROR = 0x00;

// Cosine table for fast DCT calculation
const double COS[8][8] = { { 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000 },
                           { 0.980785, 0.831470, 0.555570, 0.195090, -0.195090, -0.555570, -0.831470, -0.980785 },
                           { 0.923880, 0.382683, -0.382683, -0.923880, -0.923880, -0.382683, 0.382683, 0.923880 },
                           { 0.831470, -0.195090, -0.980785, -0.555570, 0.555570, 0.980785, 0.195090, -0.831470 },
                           { 0.707107, -0.707107, -0.707107, 0.707107, 0.707107, -0.707107, -0.707107, 0.707107 },
                           { 0.555570, -0.980785, 0.195090, 0.831470, -0.831470, -0.195090, 0.980785, -0.555570 },
                           { 0.382683, -0.923880, 0.923880, -0.382683, -0.382683, 0.923880, -0.923880, 0.382683 },
                           { 0.195090, -0.555570, 0.831470, -0.980785, 0.980785, -0.831470, 0.555570, -0.195090 } };

// Quantization table used in luminance channel (Y)
const unsigned char QUANT_LUMINANCE[8][8] = { { 4, 3, 3, 4, 6, 7, 8, 10 },
                                              { 3, 3, 3, 4, 5, 6, 8, 10 },
                                              { 3, 3, 3, 4, 6, 9, 12, 12 },
                                              { 4, 4, 4, 7, 9, 12, 12, 17 },
                                              { 6, 5, 6, 9, 12, 13, 17, 20 },
                                              { 7, 6, 9, 12, 13, 17, 20, 20 },
                                              { 8, 8, 12, 12, 17, 20, 20, 20 },
                                              { 10, 10, 12, 17, 20, 20, 20, 20 } };

// Quantization table used in chrominance channel (Cb and Cr)
const unsigned char QUANT_CHROMI[8][8] = { { 4, 5, 8, 15, 20, 20, 20, 20 },
                                           { 5, 7, 10, 14, 20, 20, 20, 20 },
                                           { 8, 10, 14, 20, 20, 20, 20, 20 },
                                           { 15, 14, 20, 20, 20, 20, 20, 20 },
                                           { 20, 20, 20, 20, 20, 20, 20, 20 },
                                           { 20, 20, 20, 20, 20, 20, 20, 20 },
                                           { 20, 20, 20, 20, 20, 20, 20, 20 },
                                           { 20, 20, 20, 20, 20, 20, 20, 20 } };

void bmp_free_channels(BMP_FILE **); // Function to free memory used by channels
void bmp_free_huff_channels(BMP_FILE **); // Function to free memory used by encoded channels by huffman code
void bmp_alloc_huff_blocks(BMP_FILE *); // Allocate blocks for huffman code
void foward_dct(double **); // Calculates the foward DCT-II in 8x8 blocks
void inverse_dct(double **); // Calculates the inverse DCT-II in 8x8 blocks
void quantization_luminance(double **); // Apply the quantization in luminance channel
void inverse_quantization_luminance(double **); // Apply the inverse quantization in luminance channel
void quantization_chrominance(double **); // Apply the quantization in chrominance channel
void inverse_quantization_chrominance(double **); // Apply the inverse quantization in chrominance channel
void calculate_difference(double **); // Auxiliary function to delta encoding
void calculate_inv_difference(double **); // Auxiliary function do delta decoding
void print8x8block(double **); // Print the content of an 8x8 block
int category(unsigned int); // Given then huffman code 'code', returns the category of the bit stream
unsigned int huffman_code(int); // Returns the correspondent huffman code of value 'value'
unsigned int inverse_huffman_code(unsigned int); // Calculates the inversed of huffman code
void write_in(unsigned int **, FILE *); // Writes a 8x8 block in a file
int fill_buffer(BUFFER *, unsigned int, int); // Function to fill 8 byte buffer
unsigned long extract_value(unsigned long *); // Consume the buffer based in huffman code and computes his inverse
void read_of(FILE *, double **); // Read
void print_zigzag(double **);


typedef struct t_bmp_info_header
{
	unsigned int bmpHeaderSize;
    unsigned int bmpWidth;
    unsigned int bmpHeight;
    unsigned int bmpCompression;
    unsigned int bmpImageSize;
    unsigned int bmpXPixelsPerMeter;
    unsigned int bmpYPixelsPerMeter;
    unsigned int bmpTotalColors;
    unsigned int bmpImportantColors;
    unsigned short bmpBitsPerPixel;
    unsigned short bmpPlanes;
} BMP_INFO_HEADER;

typedef struct t_bmp_header
{
	unsigned int bmpFileSize; // Size of BMP file in bytes
	unsigned int bmpPixelDataOffset; // Offset to the pixel data
	unsigned short bmpSignature; // Signature of BMP file
	unsigned short bmpReserverd1; // Reserved for application
	unsigned short bmpReserverd2; // Reserved for application
	struct t_bmp_info_header info_header; // Rest of BMP Header
} BMP_HEADER;

struct t_bmp_channels
{
    unsigned int qt_blocks;
    double ***y, ***cb, ***cr;
    unsigned int ***huff_y, ***huff_cb, ***huff_cr;
};

struct t_bmp_file
{
    BMP_HEADER header;
    BMP_CHANNELS channels;
};

BMP_FILE *bmp_read_file(const char *file_name)
{
    unsigned char r = 0x00, g = 0x00, b = 0x00;
    BMP_FILE *bmp = NULL;
    if(file_name != NULL)
    {
        FILE *arq = fopen(file_name, "rb");
        if(arq != NULL)
        {
            bmp = (BMP_FILE *) malloc(sizeof(BMP_FILE));
            if(bmp != NULL)
            {
                bmp->channels.huff_y = NULL;
                bmp->channels.huff_cb = NULL;
                bmp->channels.huff_cr = NULL;

                fread(&bmp->header.bmpSignature, sizeof(unsigned short), 1, arq);
                if(bmp->header.bmpSignature == BMP_SIG) // Verify if it's a BMP file
                {
                    fread(&bmp->header.bmpFileSize, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.bmpReserverd1, sizeof(unsigned short), 1, arq);
                    fread(&bmp->header.bmpReserverd2, sizeof(unsigned short), 1, arq);
                    fread(&bmp->header.bmpPixelDataOffset, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpHeaderSize, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpWidth, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpHeight, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpPlanes, sizeof(unsigned short), 1, arq);
                    fread(&bmp->header.info_header.bmpBitsPerPixel, sizeof(unsigned short), 1, arq);
                    fread(&bmp->header.info_header.bmpCompression, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpImageSize, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpXPixelsPerMeter, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpYPixelsPerMeter, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpTotalColors, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpImportantColors, sizeof(unsigned int), 1, arq);
                    
                    bmp->channels.qt_blocks = (bmp->header.info_header.bmpHeight * bmp->header.info_header.bmpWidth) / 64;
                    // Alloc pixels
                    bmp->channels.y = (double ***) malloc(sizeof(double **) * bmp->channels.qt_blocks);
                    bmp->channels.cb = (double ***) malloc(sizeof(double **) * bmp->channels.qt_blocks);
                    bmp->channels.cr = (double ***) malloc(sizeof(double **) * bmp->channels.qt_blocks);
                    for(int i = 0; i < bmp->channels.qt_blocks; i++)
                    {
                        bmp->channels.y[i] = (double **) malloc(sizeof(double *) * 8);
                        bmp->channels.cb[i] = (double **) malloc(sizeof(double *) * 8);
                        bmp->channels.cr[i] = (double **) malloc(sizeof(double *) * 8);
                        for(int j = 0; j < 8; j++)
                        {
                            bmp->channels.y[i][j] = (double *) malloc(sizeof(double) * 8);
                            bmp->channels.cb[i][j] = (double *) malloc(sizeof(double) * 8);
                            bmp->channels.cr[i][j] = (double *) malloc(sizeof(double) * 8);
                        }
                    }

                    // Read pixels
                    fseek(arq, bmp->header.bmpPixelDataOffset, SEEK_SET);
                    for(int k = 0; k < bmp->channels.qt_blocks; k++)
                    {
                        for(int i = 0; i < 8; i++)
                        {
                            for(int j = 0; j < 8; j++)
                            {
                                fread(&b, sizeof(unsigned char), 1, arq);
                                fread(&g, sizeof(unsigned char), 1, arq);
                                fread(&r, sizeof(unsigned char), 1, arq);
                                bmp->channels.y[k][i][j] = (0.299 * r) + (0.587 * g) + (0.114 * b);
                                bmp->channels.cb[k][i][j] = 0.564 * (b - bmp->channels.y[k][i][j]);
                                bmp->channels.cr[k][i][j] = 0.713 * (r - bmp->channels.y[k][i][j]);
                            }
                        }
                    }
                }
                else 
                {
                    ERROR = ERR_NOT_BITMAP;
                }
            }
            else 
            {
                ERROR = ERR_ALLOCATE_MEMORY;
            }
        }
        else 
        {
            ERROR = ERR_COULD_NOT_OPEN_FILE;
        }
        fclose(arq);
    }
    else 
    {
        ERROR = ERR_EMPTY_FILE_NAME;
    }
    error_catch(ERROR);
    return bmp;
}

int bmp_write_file(const char *file_name, BMP_FILE *bmp)
{
    unsigned char r = 0x00, g = 0x00, b = 0x00;
    int err = 0;
    if(file_name != NULL)
    {
        if(bmp != NULL)
        {
            FILE *arq = fopen(file_name, "wb");
            if(arq != NULL)
            {
                fwrite(&bmp->header.bmpSignature, sizeof(unsigned short), 1, arq);
                fwrite(&bmp->header.bmpFileSize, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.bmpReserverd1, sizeof(unsigned short), 1, arq);
                fwrite(&bmp->header.bmpReserverd2, sizeof(unsigned short), 1, arq);
                fwrite(&bmp->header.bmpPixelDataOffset, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpHeaderSize, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpWidth, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpHeight, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpPlanes, sizeof(unsigned short), 1, arq);
                fwrite(&bmp->header.info_header.bmpBitsPerPixel, sizeof(unsigned short), 1, arq);
                fwrite(&bmp->header.info_header.bmpCompression, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpImageSize, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpXPixelsPerMeter, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpYPixelsPerMeter, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpTotalColors, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpImportantColors, sizeof(unsigned int), 1, arq);
                
                fseek(arq, bmp->header.bmpPixelDataOffset, SEEK_SET);
                for(int k = 0; k < bmp->channels.qt_blocks; k++)
                {
                    for(int i = 0; i < 8; i++)
                    {
                        for(int j = 0; j < 8; j++)
                        {
                            r = (unsigned char) (bmp->channels.y[k][i][j] + (1.402 * bmp->channels.cr[k][i][j]));
                            g = (unsigned char) (bmp->channels.y[k][i][j] - (0.344 * bmp->channels.cb[k][i][j]) - (0.714 * bmp->channels.cr[k][i][j]));
                            b = (unsigned char) (bmp->channels.y[k][i][j] + (1.772 * bmp->channels.cb[k][i][j]));
                            fwrite(&b, sizeof(unsigned char), 1, arq);
                            fwrite(&g, sizeof(unsigned char), 1, arq);
                            fwrite(&r, sizeof(unsigned char), 1, arq);
                        }
                    }
                }
            }
            else 
            {
                ERROR = ERR_CREATE_BITMAP;
                err = -1;
            }
            fclose(arq);
        }
        else 
        {
            ERROR = ERR_BMP_NOT_EXIST;
            err = -1;
        }
    }
    else 
    {
        ERROR = ERR_EMPTY_FILE_NAME;
        err = -1;
    }
    error_catch(ERROR);
    return err;
}

void bmp_dct(BMP_FILE *bmp, char type)
{
    if(bmp != NULL)
    {
        if(type == 0) // If it is the foward DCT-II
        {
            for(int i = 0; i < bmp->channels.qt_blocks; i++)
            {
                foward_dct(bmp->channels.y[i]);
                foward_dct(bmp->channels.cb[i]);
                foward_dct(bmp->channels.cr[i]);
            }
        }
        else if(type == -1) // If it is the inversed DCT-II
        {
            for(int i = 0; i < bmp->channels.qt_blocks; i++)
            {
                inverse_dct(bmp->channels.y[i]);
                inverse_dct(bmp->channels.cb[i]);
                inverse_dct(bmp->channels.cr[i]);
            }
        }
    }
    else 
    {
        ERROR = ERR_BMP_NOT_EXIST;
    }
    error_catch(ERROR);
}

void foward_dct(double **channel)
{
    double sum = 0.0, ci = 0.0, cj = 0.0;
    double out[8][8];
    for(int i = 0; i < 8; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            sum = 0.0;
            for(int x = 0; x < 8; x++)
            {
                for(int y = 0; y < 8; y++)
                {
                    sum += channel[x][y] * COS[i][x] * COS[j][y];
                }
            }
            if(i == 0) ci = (1.0 / SQRT_2); else ci = 1;
            if(j == 0) cj = (1.0 / SQRT_2); else cj = 1;
            out[i][j] = (1.0 / 4.0) * ci * cj * sum;
        }
    }

    for(int i = 0; i < 8; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            channel[i][j] = out[i][j];
        }
    }
}

void inverse_dct(double **channel)
{
    double out[8][8];
    double sum = 0.0, ci = 0.0, cj = 0.0;
    for(int x = 0; x < 8; x++)
    {
        for(int y = 0; y < 8; y++)
        {
            sum = 0.0;
            for(int i = 0; i < 8; i++)
            {
                if(i == 0) ci = (1.0 / SQRT_2); else ci = 1;
                for(int j = 0; j < 8; j++)
                {
                    if(j == 0) cj = (1.0 / SQRT_2); else cj = 1;
                    sum += ci * cj * channel[i][j] * COS[i][x] * COS[j][y];
                }
            }
            out[x][y] = (1.0 / 4.0) * sum;
        }
    }

    for(int i = 0; i < 8; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            channel[i][j] = out[i][j];
        }
    }
}

void bmp_quantization(BMP_FILE *bmp)
{
    if(bmp != NULL)
    {
        for(int i = 0; i < bmp->channels.qt_blocks; i++)
        {
            quantization_luminance(bmp->channels.y[i]);
            quantization_chrominance(bmp->channels.cb[i]);
            quantization_chrominance(bmp->channels.cr[i]);
        }
    }
    else 
    {
        ERROR = ERR_BMP_NOT_EXIST;
    }
    error_catch(ERROR);
}

void quantization_luminance(double **channel)
{
    for(int i = 0; i < 8; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            channel[i][j] = round(channel[i][j] / QUANT_LUMINANCE[i][j]);
        }
    }
}

void quantization_chrominance(double **channel)
{
    for(int i = 0; i < 8; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            channel[i][j] = round(channel[i][j] / QUANT_CHROMI[i][j]);
        }
    }
}

void inverse_quantization_luminance(double **channel)
{
    for(int i = 0; i < 8; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            channel[i][j] *= QUANT_LUMINANCE[i][j];
        }
    }
}

void inverse_quantization_chrominance(double **channel)
{
    for(int i = 0; i < 8; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            channel[i][j] *= QUANT_CHROMI[i][j];
        }
    }
}

void bmp_inverse_quantization(BMP_FILE *bmp)
{
    if(bmp != NULL)
    {
        for(int i = 0; i < bmp->channels.qt_blocks; i++)
        {
            inverse_quantization_luminance(bmp->channels.y[i]);
            inverse_quantization_chrominance(bmp->channels.cb[i]);
            inverse_quantization_chrominance(bmp->channels.cr[i]);
        }
    }
    else 
    {
        ERROR = ERR_BMP_NOT_EXIST;
    }
    error_catch(ERROR);
}

void print8x8block(double **channel)
{
    for(int x = 0; x < 8; x++)
    {
        for(int y = 0; y < 8; y++)
        {
            printf("%8.1lf", channel[x][y]);
        }
        printf("\n");
    }
}

void bmp_diff_encode(BMP_FILE *bmp)
{
    if(bmp != NULL)
    {
        for(int i = 0; i < bmp->channels.qt_blocks; i++)
        {
            calculate_difference(bmp->channels.y[i]);
            calculate_difference(bmp->channels.cb[i]);
            calculate_difference(bmp->channels.cr[i]);
        }
        print_zigzag(bmp->channels.y[0]);
    }
    else
    {
        ERROR = ERR_BMP_NOT_EXIST;
    }
    error_catch(ERROR);
}

void bmp_diff_decode(BMP_FILE *bmp)
{
    int i = 0, j = 0, x = 0, y = 0;
    if(bmp != NULL)
    {
        for(int i = 0; i < bmp->channels.qt_blocks; i++)
        {
            calculate_inv_difference(bmp->channels.y[i]);
            calculate_inv_difference(bmp->channels.cb[i]);
            calculate_inv_difference(bmp->channels.cr[i]);
        }
    }
    else
    {
        ERROR = ERR_BMP_NOT_EXIST;
    }
    error_catch(ERROR);
}

void calculate_difference(double **channel)
{
    int x = 0, y = 1, max = 2;
    double current = 0.0, last = channel[0][0];

    // First half
    for(int i = 1; i < 8; i++, max++)
    {
        for(int j = 1; j < max; j++)
        {
            current = channel[x][y];
            channel[x][y] = current - last;
            last = current;
            if((i % 2) == 0)
            {
                x--;
                y++;
            }
            else 
            {
                x++;
                y--;
            }

        }
        
        current = channel[x][y];
        channel[x][y] = current - last;
        last = current;
        if((i % 2) == 0)
        {
            y++;
        }
        else 
        {
            x++;
        }
    }
    // Second half
    max = 6;
    x = 7;
    y = 1;
    for(int i = 1; i < 7; i++, max--)
    {
        for(int j = 0; j < max; j++)
        {
            current = channel[x][y];
            channel[x][y] = current - last;
            last = current;
            if((i % 2) == 0)
            {
                x++;
                y--;
            }
            else 
            {
                x--;
                y++;
            }
        }
        current = channel[x][y];
        channel[x][y] = current - last;
        last = current;
        if((i % 2) == 0)
        {
            y++;
        }
        else 
        {
            x++;
        }
    }
    current = channel[x][y];
    channel[x][y] = current - last;
    last = current;
}

void calculate_inv_difference(double **channel)
{
    int x = 0, y = 1, max = 2;
    double current = 0.0, last = channel[0][0];

    // First half
    for(int i = 1; i < 8; i++, max++)
    {
        for(int j = 1; j < max; j++)
        {
            current = channel[x][y];
            channel[x][y] = current + last;
            last = channel[x][y];
            if((i % 2) == 0)
            {
                x--;
                y++;
            }
            else 
            {
                x++;
                y--;
            }

        }
        
        current = channel[x][y];
        channel[x][y] = current + last;
        last = channel[x][y];
        if((i % 2) == 0)
        {
            y++;
        }
        else 
        {
            x++;
        }
    }
    // Second half
    max = 6;
    x = 7;
    y = 1;
    for(int i = 1; i < 7; i++, max--)
    {
        for(int j = 0; j < max; j++)
        {
            current = channel[x][y];
            channel[x][y] = current + last;
            last = channel[x][y];
            if((i % 2) == 0)
            {
                x++;
                y--;
            }
            else 
            {
                x--;
                y++;
            }
        }
        current = channel[x][y];
        channel[x][y] = current + last;
        last = channel[x][y];
        if((i % 2) == 0)
        {
            y++;
        }
        else 
        {
            x++;
        }
    }
    current = channel[x][y];
    channel[x][y] = current + last;
    last = channel[x][y];
}

unsigned int huffman_code(int value)
{
    unsigned int code = 0;
    if(value == 0)
    {
        code = 2;
    }
    else if(value == -1 || value == 1)
    {
        if(value < 0)
        {
            value = (-1) * value;
            code = 0x06 | ((~value) & 0x01);
        }
        else 
        {
            code = 0x06 | (value & 0x01);
        }
    }
    else if(value == -3 || value == -2 || value == 2 || value == 3)
    {
        if(value < 0)
        {
            value = (-1) * value;
            code = 0x10 | ((~value) & 0x03);
        }
        else 
        {
            code = 0x10 | (value & 0x03);
        }
    }
    else if((value >= -7 && value <= -4) || (value >= 4 && value <= 7))
    {
        if(value < 0)
        {
            value = (-1) * value;
            code = (~value) & 0x07;
        }
        else 
        {
            code = value & 0x07;
        }
    }
    else if((value >= -15 && value <= -8) || (value >= 8 && value <= 15))
    {
        if(value < 0)
        {
            value = (-1) * value;
            code = 0x50 | ((~value) & 0x0F);
        }
        else 
        {
            code = 0x50 | (value & 0x0F);
        }
    }
    else if((value >= -31 && value <= -16) || (value >= 16 && value <= 31))
    {
        if(value < 0)
        {
            value = (-1) * value;
            code = 0xC0 | ((~value) & 0x1F);
        }
        else 
        {
            code = 0xC0 | (value & 0x1F);
        }
    }
    else if((value >= -63 && value <= -32) || (value >= 32 && value <= 63))
    {
        if(value < 0)
        {
            value = (-1) * value;
            code = 0x380 | ((~value) & 0x3F);
        }
        else 
        {
            code = 0x380 | (value & 0x3F);
        }
    }
    else if((value >= -127 && value <= -64) || (value >= 64 && value <= 127))
    {
        if(value < 0)
        {
            value = (-1) * value;
            code = 0xF00 | ((~value) & 0x7F);
        }
        else 
        {
            code = 0xF00 | (value & 0x7F);
        }
    }
    else if((value >= -255 && value <= -128) || (value >= 128 && value <= 255))
    {
        if(value < 0)
        {
            value = (-1) * value;
            code = 0x3E00 | ((~value) & 0xFF);
        }
        else 
        {
            code = 0x3E00 | (value & 0xFF);
        }
    }
    else if((value >= -511 && value <= -256) || (value >= 256 && value <= 511))
    {
        if(value < 0)
        {
            value = (-1) * value;
            code = (126 << 9) | ((~value) & 0x1FF);
        }
        else 
        {
            code = (126 << 9) | (value & 0x1FF);
        }
    }
    else if((value >= -1023 && value <= -512) || (value >= 512 && value <= 1023))
    {
        if(value < 0)
        {
            value = (-1) * value;
            code = 0x3F800 | ((~value) & 0x3FF);
        }
        else 
        {
            code = 0x3F800 | (value & 0x3FF);
        }
    }
    else if((value >= -2047 && value <= -1024) || (value >= 1024 && value <= 2047))
    {
        if(value < 0)
        {
            value = (-1) * value;
            code = 0x101000 | ((~value) & 0x7FF);
        }
        else 
        {
            code = 0x101000 | (value & 0x7FF);
        }
    }
    return code;
}

unsigned int inverse_huffman_code(unsigned int code)
{
    unsigned int value = 0, tmp_value = 0, prefix = 0;
    unsigned char signal = 0;
    prefix = code >> 1;
    if(code == 2) // The value is 0
    {
        value = 0;
    }
    else if((code == 6 || code == 7) && (prefix == 3)) // The value is -1 or 1
    {
        tmp_value = code & 0x01;
        if(tmp_value == 0) // Negative number
        {
            value = -1;
        }
        else 
        {
            value = 1;
        }
    }
    else if(code >= 16 && code <= 19) // The value is -3,-2 or 2,3
    {
        tmp_value = code & 0x03;
        signal = tmp_value & 0x02;
        if(signal == 0) // Negative number
        {
            value = (-1) * ((~tmp_value) & 0x03);
        }
        else 
        {
            value = tmp_value;
        }
    }
    else if(code >= 0 && code <= 7) // The value is -7..-4 or 4..7
    {
        tmp_value = code & 0x07;
        signal = tmp_value & 0x04;
        if(signal == 0)
        {
            value = (-1) * ((~tmp_value) & 0x07);
        }
        else 
        {
            value = tmp_value;
        }
    }
    else if(code >= 80 && code <= 95) // The value is -15..-8 or 8..15
    {
        tmp_value = code & 0x0F;
        signal = tmp_value & 0x08;
        if(signal == 0) // Negative number
        {
            value = (-1) * ((~tmp_value) & 0x0F);
        }
        else 
        {
            value = tmp_value;
        }
    }
    else if(code >= 192 && code <= 223) // The value is -31..-16 or 16..31
    {
        tmp_value = code & 0x1F;
        signal = tmp_value & 0x10;
        if(signal == 0)
        {
            value = (-1) * ((~tmp_value) & 0x1F);
        }
        else 
        {
            value = tmp_value;
        }
    }
    else if(code >= 896 && code <= 959) // The value -63..-32 or 32..63
    {
        tmp_value = code & 0x3F;
        signal = tmp_value & 0x20;
        if(signal == 0)
        {
            value = (-1) * ((~tmp_value) & 0x3F);
        }
        else 
        {
            value = tmp_value;
        }
    }
    else if(code >= 3840 && code <= 3967) // The value -127..-64 or 64..127
    {
        tmp_value = code & 0x7F;
        signal = tmp_value & 0x40;
        if(signal == 0)
        {
            value = (-1) * ((~tmp_value) & 0x7F);
        }
        else 
        {
            value = tmp_value;
        }
    }
    else if(code >= 15872 && code <= 16127) // The value -255..-128 or 128..255
    {
        tmp_value = code & 0xFF;
        signal = tmp_value & 0x80;
        if(signal == 0)
        {
            value = (-1) * ((~tmp_value) & 0xFF);
        }
        else 
        {
            value = tmp_value;
        }
    }
    else if(code >= 64512 && code <= 65023) // The value -511..-256 or 256..511
    {
        tmp_value = code & 0x1FF;
        signal = tmp_value & 0x100;
        if(signal == 0)
        {
            value = (-1) * ((~tmp_value) * 0x1FF);
        }
        else 
        {
            value = tmp_value;
        }
    }
    else if(code >= 260096 && code <= 261119) // The value -1023..-512 or 512..1023
    {
        tmp_value = code & 0x3FF;
        signal = tmp_value & 0x200;
        if(signal == 0)
        {
            value = (-1) * ((~tmp_value) * 0x3FF);
        }
        else 
        {
            value = tmp_value;
        }
    }
    return value;
}

unsigned long extract_value(unsigned long *buffer)
{
    unsigned long prefix = 0, value = 0;
    prefix = (*buffer) & 0xC000000000000000;
    if(prefix == 0) // Category 3
    {
        value = (*buffer) & 0xF800000000000000;
        value = (value >> 59);
        value = inverse_huffman_code(value);
        // printf("HEX_VALUE: %d\n", value);
        (*buffer) <<= 5;
    }
    else 
    {
        prefix = (*buffer) & 0xE000000000000000; // Mask for 3 bits
        if(prefix == 0x4000000000000000) // Category 0 (for DC)
        {
            value = (*buffer) & 0xE000000000000000;
            value = (value >> 61);
            value = inverse_huffman_code(value);
            // printf("HEX_VALUE: %d\n", value);
            (*buffer) <<= 3;
        }
        else if(prefix == 0x6000000000000000) // Category 1
        {
            value = (*buffer) & 0xF000000000000000;
            value = (value >> 60);
            value = inverse_huffman_code(value);
            // printf("HEX_VALUE: %d\n", value);
            (*buffer) <<= 4;
        }
        else if(prefix == 0x8000000000000000) // Category 2
        {
            value = (*buffer) & 0xF800000000000000;
            value = (value >> 59);
            value = inverse_huffman_code(value);
            // printf("HEX_VALUE: %d\n", value);
            (*buffer) <<= 5;
        }
        else if(prefix == 0xA000000000000000) // Category 4
        {
            value = (*buffer) & 0xFE00000000000000;
            value = (value >> 57);
            value = inverse_huffman_code(value);
            // printf("HEX_VALUE: %d\n", value);
            (*buffer) <<= 7;
        }
        else if(prefix == 0xC000000000000000) // Category 5
        {
            value = (*buffer) & 0xFF00000000000000;
            value = (value >> 56);
            value = inverse_huffman_code(value);
            // printf("HEX_VALUE: %d\n", value);
            (*buffer) <<= 8;
        }
        else 
        {
            prefix = (*buffer) & 0xF000000000000000;
            if(prefix == 0xE000000000000000) // Category 6
            {
                value = (*buffer) & 0xFFC0000000000000;
                value = (value >> 54);
                value = inverse_huffman_code(value);
                // printf("HEX_VALUE: %d\n", value);
                (*buffer) <<= 10;
            }
            else
            {
                prefix = (*buffer) & 0xF800000000000000;
                if(prefix == 0xF000000000000000) // Category 7
                {
                    value = (*buffer) & 0xFFF0000000000000;
                    value = (value >> 52);
                    value = inverse_huffman_code(value);
                    // printf("HEX_VALUE: %d\n", value);
                    (*buffer) <<= 12;
                }
                else 
                {
                    prefix = (*buffer) & 0xFC00000000000000;
                    if(prefix == 0xF800000000000000) // Category 8
                    {
                        value = (*buffer) & 0xFFFC000000000000;
                        value = (value >> 50);
                        value = inverse_huffman_code(value);
                        // printf("HEX_VALUE: %d\n", value);
                        (*buffer) <<= 14;
                    }
                    else 
                    {
                        prefix = (*buffer) & 0xFE00000000000000;
                        if(prefix == 0xFC00000000000000) // Category 9
                        {
                            value = (*buffer) & 0xFFFF000000000000;
                            value = (value >> 48);
                            value = inverse_huffman_code(value);
                            // printf("HEX_VALUE: %d\n", value);
                            (*buffer) <<= 16;
                        }
                        else 
                        {
                            prefix = (*buffer) & 0xFF00000000000000;
                            if(prefix == 0xFE00000000000000) // Category A or 10
                            {
                                value = (*buffer) & 0xFFFFC00000000000;
                                value = (value >> 46);
                                value = inverse_huffman_code(value);
                                // printf("HEX_VALUE: %d\n", value);
                                (*buffer) <<= 18;
                            }
                            else if(prefix == 0xFF00000000000000) // EOB
                            {
                                value = EOB;
                                (*buffer) <<= 8;
                            }
                        }
                    }
                }
            }
        }
    }
    return value;
}

int category(unsigned int code)
{
    int value = -1, prefix = 0;
    prefix = code >> 1;
    if(code == 2) // The value is 0
    {
        value = 0;
    }
    else if((code == 6 || code == 7) && (prefix == 3)) // The value is -1 or 1
    {
        value = 1;
    }
    else if(code >= 16 && code <= 19) // The value is -3,-2 or 2,3
    {
        value = 2;
    }
    else if(code >= 0 && code <= 7) // The value is -7..-4 or 4..7
    {
        value = 3;
    }
    else if(code >= 80 && code <= 95) // The value is -15..-8 or 8..15
    {
        value = 4;
    }
    else if(code >= 192 && code <= 223) // The value is -31..-16 or 16..31
    {
        value = 5;
    }
    else if(code >= 896 && code <= 959) // The value -63..-32 or 32..63
    {
        value = 6;
    }
    else if(code >= 3840 && code <= 3967) // The value -127..-64 or 64..127
    {
        value = 7;
    }
    else if(code >= 15872 && code <= 16127) // The value -255..-128 or 128..255
    {
        value = 8;
    }
    else if(code >= 64512 && code <= 65023) // The value -511..-256 or 256..511
    {
        value = 9;
    }
    else if(code >= 260096 && code <= 261119) // The value -1023..-512 or 512..1023
    {
        value = 10;
    }
    return value;
}

void bmp_alloc_huff_blocks(BMP_FILE *bmp)
{
    if(bmp != NULL)
    {
        // Alloc pixels for huffman code
        bmp->channels.huff_y = (unsigned int ***) malloc(sizeof(unsigned int **) * bmp->channels.qt_blocks);
        bmp->channels.huff_cb = (unsigned int ***) malloc(sizeof(unsigned int **) * bmp->channels.qt_blocks);
        bmp->channels.huff_cr = (unsigned int ***) malloc(sizeof(unsigned int **) * bmp->channels.qt_blocks);
        for(int i = 0; i < bmp->channels.qt_blocks; i++)
        {
            bmp->channels.huff_y[i] = (unsigned int **) malloc(sizeof(unsigned int *) * 8);
            bmp->channels.huff_cb[i] = (unsigned int **) malloc(sizeof(unsigned int *) * 8);
            bmp->channels.huff_cr[i] = (unsigned int **) malloc(sizeof(unsigned int *) * 8);
            for(int j = 0; j < 8; j++)
            {
                bmp->channels.huff_y[i][j] = (unsigned int *) malloc(sizeof(unsigned int) * 8);
                bmp->channels.huff_cb[i][j] = (unsigned int *) malloc(sizeof(unsigned int) * 8);
                bmp->channels.huff_cr[i][j] = (unsigned int *) malloc(sizeof(unsigned int) * 8);
            }
        }
    }
}

void bmp_compute_huffman_code(BMP_FILE *bmp)
{
    if(bmp != NULL)
    {
        bmp_alloc_huff_blocks(bmp);
        for(int i = 0; i < bmp->channels.qt_blocks; i++)
        {
            for(int j = 0; j < 8; j++)
            {
                for(int k = 0; k < 8; k++)
                {
                    bmp->channels.huff_y[i][j][k] = huffman_code((int) bmp->channels.y[i][j][k]);
                    bmp->channels.huff_cb[i][j][k] = huffman_code((int) bmp->channels.cb[i][j][k]);
                    bmp->channels.huff_cr[i][j][k] = huffman_code((int) bmp->channels.cr[i][j][k]);
                }
            }
        }
    }
}

void bmp_compress(BMP_FILE *bmp, const char *file_name)
{
    FILE *arq = NULL;
    if(bmp != NULL)
    {
        if(file_name != NULL)
        {
            arq = fopen(file_name, "wb");
            if(arq != NULL)
            {
                fwrite(&bmp->header.bmpSignature, sizeof(unsigned short), 1, arq);
                fwrite(&bmp->header.bmpFileSize, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.bmpReserverd1, sizeof(unsigned short), 1, arq);
                fwrite(&bmp->header.bmpReserverd2, sizeof(unsigned short), 1, arq);
                fwrite(&bmp->header.bmpPixelDataOffset, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpHeaderSize, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpWidth, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpHeight, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpPlanes, sizeof(unsigned short), 1, arq);
                fwrite(&bmp->header.info_header.bmpBitsPerPixel, sizeof(unsigned short), 1, arq);
                fwrite(&bmp->header.info_header.bmpCompression, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpImageSize, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpXPixelsPerMeter, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpYPixelsPerMeter, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpTotalColors, sizeof(unsigned int), 1, arq);
                fwrite(&bmp->header.info_header.bmpImportantColors, sizeof(unsigned int), 1, arq);
                
                fseek(arq, bmp->header.bmpPixelDataOffset, SEEK_SET);
                for(int i = 0; i < bmp->channels.qt_blocks; i++)
                {
                    write_in(bmp->channels.huff_y[i], arq);
                    write_in(bmp->channels.huff_cb[i], arq);
                    write_in(bmp->channels.huff_cr[i], arq);
                }
            }
            fclose(arq);
        }
    }
}

BMP_FILE *bmp_decompress(const char *file_name)
{
    FILE *arq = NULL;
    BMP_FILE *bmp = NULL;
    if(file_name != NULL)
    {
        arq = fopen(file_name, "rb");
        if(arq != NULL)
        {
            bmp = (BMP_FILE *) malloc(sizeof(BMP_FILE));
            if(bmp != NULL)
            {
                bmp->channels.huff_y = NULL;
                bmp->channels.huff_cb = NULL;
                bmp->channels.huff_cr = NULL;

                fread(&bmp->header.bmpSignature, sizeof(unsigned short), 1, arq);
                if(bmp->header.bmpSignature == BMP_SIG) // Verify if it's a BMP file
                {
                    fread(&bmp->header.bmpFileSize, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.bmpReserverd1, sizeof(unsigned short), 1, arq);
                    fread(&bmp->header.bmpReserverd2, sizeof(unsigned short), 1, arq);
                    fread(&bmp->header.bmpPixelDataOffset, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpHeaderSize, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpWidth, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpHeight, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpPlanes, sizeof(unsigned short), 1, arq);
                    fread(&bmp->header.info_header.bmpBitsPerPixel, sizeof(unsigned short), 1, arq);
                    fread(&bmp->header.info_header.bmpCompression, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpImageSize, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpXPixelsPerMeter, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpYPixelsPerMeter, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpTotalColors, sizeof(unsigned int), 1, arq);
                    fread(&bmp->header.info_header.bmpImportantColors, sizeof(unsigned int), 1, arq);
                    
                    bmp->channels.qt_blocks = (bmp->header.info_header.bmpHeight * bmp->header.info_header.bmpWidth) / 64;

                    // Alloc pixels
                    bmp->channels.y = (double ***) malloc(sizeof(double **) * bmp->channels.qt_blocks);
                    bmp->channels.cb = (double ***) malloc(sizeof(double **) * bmp->channels.qt_blocks);
                    bmp->channels.cr = (double ***) malloc(sizeof(double **) * bmp->channels.qt_blocks);
                    for(int i = 0; i < bmp->channels.qt_blocks; i++)
                    {
                        bmp->channels.y[i] = (double **) malloc(sizeof(double *) * 8);
                        bmp->channels.cb[i] = (double **) malloc(sizeof(double *) * 8);
                        bmp->channels.cr[i] = (double **) malloc(sizeof(double *) * 8);
                        for(int j = 0; j < 8; j++)
                        {
                            bmp->channels.y[i][j] = (double *) malloc(sizeof(double) * 8);
                            bmp->channels.cb[i][j] = (double *) malloc(sizeof(double) * 8);
                            bmp->channels.cr[i][j] = (double *) malloc(sizeof(double) * 8);
                        }
                    }

                    fseek(arq, bmp->header.bmpPixelDataOffset, SEEK_SET);
                    // for(int k = 0; k < bmp->channels.qt_blocks; k++)
                    // {
                        read_of(arq, bmp->channels.y[0]);
                        // read_of(arq, bmp->channels.cb[0]);
                        // read_of(arq, bmp->channels.cr[0]);
                    // }
                }
            }
        }
        fclose(arq);
    }
    return bmp;
}

void write_in(unsigned int **block, FILE *arq)
{
    BUFFER b;
    b.buffer = 0;
    b.remaining_bits = 64;
    unsigned long aux = 0;
    unsigned char zero_qt = 0, tmp_byte = 0x00;
    const unsigned char zero_symbol = 0x02;
    int x = 0, y = 1, max = 2;
    fill_buffer(&b, block[0][0], category(block[0][0]));

    for(int i = 1; i < 8; i++, max++)
    {
        for(int j = 1; j < max; j++)
        {
            if(block[x][y] == zero_symbol)
            {
                zero_qt++;
            }
            else 
            {
                if(zero_qt > 0)
                {
                    if(b.remaining_bits >= 11)
                    {
                        fill_buffer(&b, huffman_code(0), category(huffman_code(0)));
                        if(fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt))) != 0)
                        {
                            b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                            b.remaining_bits -= 8;
                            b.buffer <<= b.remaining_bits;
                            fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                            b.buffer = 0;
                            b.remaining_bits = 64;
                            fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt)));
                        }
                    }
                    else 
                    {
                        b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                        b.remaining_bits -= 8;
                        b.buffer = b.buffer << b.remaining_bits;
                        fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                        b.buffer = 0;
                        b.remaining_bits = 64;
                        fill_buffer(&b, huffman_code(0), category(huffman_code(0)));
                        fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt)));
                    }
                    zero_qt = 0;
                }
                if(fill_buffer(&b, block[x][y], category(block[x][y])) != 0)
                {
                    b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                    b.remaining_bits -= 8;
                    b.buffer <<= b.remaining_bits;
                    fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                    b.buffer = 0;
                    b.remaining_bits = 64;
                    fill_buffer(&b, block[x][y], category(block[x][y]));
                }
            }
            
            printf("BUFFER: %lX\n", b.buffer);
            if((i % 2) == 0)
            {
                x--;
                y++;
            }
            else 
            {
                x++;
                y--;
            }
        }
        
        if(block[x][y] == zero_symbol)
        {
            zero_qt++;
        }
        else 
        {
            if(zero_qt > 0)
            {
                if(b.remaining_bits >= 11)
                {
                    fill_buffer(&b, huffman_code(0), category(huffman_code(0)));
                    if(fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt))) != 0)
                    {
                        b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                        b.remaining_bits -= 8;
                        b.buffer <<= b.remaining_bits;
                        fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                        b.buffer = 0;
                        b.remaining_bits = 64;
                        fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt)));
                    }
                }
                else 
                {
                    b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                    b.remaining_bits -= 8;
                    b.buffer = b.buffer << b.remaining_bits;
                    fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                    b.buffer = 0;
                    b.remaining_bits = 64;
                    fill_buffer(&b, huffman_code(0), category(huffman_code(0)));
                    fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt)));
                }
                zero_qt = 0;
            }
            if(fill_buffer(&b, block[x][y], category(block[x][y])) != 0)
            {
                b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                b.remaining_bits -= 8;
                b.buffer <<= b.remaining_bits;
                fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                b.buffer = 0;
                b.remaining_bits = 64;
                fill_buffer(&b, block[x][y], category(block[x][y]));
            }
        }
        printf("BUFFER: %lX\n", b.buffer);
        if((i % 2) == 0)
        {
            y++;
        }
        else 
        {
            x++;
        }
    }
    // Second half
    max = 6;
    x = 7;
    y = 1;
    for(int i = 1; i < 7; i++, max--)
    {
        for(int j = 0; j < max; j++)
        {
            if(block[x][y] == zero_symbol)
            {
                zero_qt++;
            }
            else 
            {
                if(zero_qt > 0)
                {
                    if(b.remaining_bits >= 11)
                    {
                        fill_buffer(&b, huffman_code(0), category(huffman_code(0)));
                        if(fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt))) != 0)
                        {
                            b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                            b.remaining_bits -= 8;
                            b.buffer <<= b.remaining_bits;
                            fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                            b.buffer = 0;
                            b.remaining_bits = 64;
                            fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt)));
                        }
                    }
                    else 
                    {
                        b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                        b.remaining_bits -= 8;
                        b.buffer = b.buffer << b.remaining_bits;
                        fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                        b.buffer = 0;
                        b.remaining_bits = 64;
                        fill_buffer(&b, huffman_code(0), category(huffman_code(0)));
                        fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt)));
                    }
                    zero_qt = 0;
                }
                if(fill_buffer(&b, block[x][y], category(block[x][y])) != 0)
                {
                    b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                    b.remaining_bits -= 8;
                    b.buffer <<= b.remaining_bits;
                    fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                    b.buffer = 0;
                    b.remaining_bits = 64;
                    fill_buffer(&b, block[x][y], category(block[x][y]));
                }
            }

            printf("BUFFER: %lX\n", b.buffer);
            if((i % 2) == 0)
            {
                x++;
                y--;
            }
            else 
            {
                x--;
                y++;
            }
        }

        if(block[x][y] == zero_symbol)
        {
            zero_qt++;
        }
        else 
        {
            if(zero_qt > 0)
            {
                if(b.remaining_bits >= 11)
                {
                    fill_buffer(&b, huffman_code(0), category(huffman_code(0)));
                    if(fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt))) != 0)
                    {
                        b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                        b.remaining_bits -= 8;
                        b.buffer <<= b.remaining_bits;
                        fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                        b.buffer = 0;
                        b.remaining_bits = 64;
                        fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt)));
                    }
                }
                else 
                {
                    b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                    b.remaining_bits -= 8;
                    b.buffer = b.buffer << b.remaining_bits;
                    fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                    b.buffer = 0;
                    b.remaining_bits = 64;
                    fill_buffer(&b, huffman_code(0), category(huffman_code(0)));
                    fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt)));
                }
                zero_qt = 0;
            }
            if(fill_buffer(&b, block[x][y], category(block[x][y])) != 0)
            {
                b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                b.remaining_bits -= 8;
                b.buffer <<= b.remaining_bits;
                fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
                b.buffer = 0;
                b.remaining_bits = 64;
                fill_buffer(&b, block[x][y], category(block[x][y]));
            }
        }

        printf("BUFFER: %lX\n", b.buffer);
        if((i % 2) == 0)
        {
            y++;
        }
        else 
        {
            x++;
        }
    }

    if(block[x][y] == zero_symbol)
    {
        zero_qt++;
        if(b.remaining_bits >= 11)
        {
            fill_buffer(&b, huffman_code(0), category(huffman_code(0)));
            if(fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt))) != 0)
            {
                b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
                b.remaining_bits -= 8;
                b.buffer <<= b.remaining_bits;
                fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);  
                b.buffer = 0;
                b.remaining_bits = 64;
                fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt)));
            }
        }
        else 
        {
            b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
            b.remaining_bits -= 8;
            b.buffer <<= b.remaining_bits;
            fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);  
            b.buffer = 0;
            b.remaining_bits = 64;
            fill_buffer(&b, huffman_code(0), category(huffman_code(0)));
            fill_buffer(&b, huffman_code(zero_qt), category(huffman_code(zero_qt)));
        }
    }
    else
    {
        if(fill_buffer(&b, block[x][y], category(block[x][y])) != 0)
        {
            b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
            b.remaining_bits -= 8;
            b.buffer <<= b.remaining_bits;
            fwrite(&(b.buffer), sizeof(unsigned long), 1, arq);
            b.buffer = 0;
            b.remaining_bits = 64;
            fill_buffer(&b, block[x][y], category(block[x][y]));
        }
    }
    printf("BITS REMAINING: %hhu\n", b.remaining_bits);
    b.buffer = (b.buffer << 8) | 0xFF; // Put the EOB prefix
    b.remaining_bits -= 8;
    b.buffer = b.buffer << b.remaining_bits;
    printf("FINAL BUFFER: %lX\n", b.buffer);
    fwrite(&b.buffer, sizeof(unsigned long), 1, arq);
}

void read_of(FILE *arq, double **vet)
{
    unsigned long buffer = 0;
    int x = 0, y = 1, max = 2, rec_block[64], value = EOB, zero_qt = 0, ptr_rec_block = 0;
    fread(&buffer, sizeof(unsigned long), 1, arq);
    // printf("BUFFER: %lX\n", buffer);
    while(1)
    {
        // printf("PTR REC BLOCK: %d\n", ptr_rec_block);
        if(feof(arq) != 0 || ptr_rec_block >= 63)
        {
            break;
        }
        else 
        {
            if(buffer == 0 && ptr_rec_block < 64)
            {
                fread(&buffer, sizeof(unsigned long), 1, arq);
                // printf("BUFFER: %lX\n", buffer);
            }
            else 
            {
                value = extract_value(&buffer);
                if(value == 0)
                {
                    zero_qt = extract_value(&buffer);
                    // if(zero_qt == EOB)
                    while(zero_qt > 0)
                    {
                        printf("%d ", value, zero_qt);
                        ptr_rec_block++;
                        zero_qt--;
                    }
                }
                else if(value != EOB)
                {
                    printf("%d ", value);
                    ptr_rec_block++;
                }
                // if(ptr_rec_block < 64)
                // {
                //     value = extract_value(&buffer);
                //     if(value != EOB)
                //     {
                //         if(value == 0)
                //         {
                //             zero_qt = extract_value(&buffer);
                //             while (zero_qt > 0)
                //             {
                //                 rec_block[ptr_rec_block++] = 0;
                //                 zero_qt--;
                //             }
                //         }
                //         else 
                //         {
                //             rec_block[ptr_rec_block++] = value;
                //         }
                //     }
                // }
            }
        }
    }
    printf("\n");
    // for(int i = 0; i < 64; i++)
    // {
    //     printf("%d ", rec_block[i]);
    // }
    // printf("\n");

    // ptr_rec_block = 0;
    // vet[0][0] = rec_block[ptr_rec_block++];
    // for(int i = 1; i < 8; i++, max++)
    // {
    //     for(int j = 1; j < max; j++)
    //     {
    //         vet[x][y] = rec_block[ptr_rec_block++];
    //         if((i % 2) == 0)
    //         {
    //             x--;
    //             y++;
    //         }
    //         else 
    //         {
    //             x++;
    //             y--;
    //         }
    //     }
        
    //     vet[x][y] = rec_block[ptr_rec_block++];
    //     // Logic Here
    //     if((i % 2) == 0)
    //     {
    //         y++;
    //     }
    //     else 
    //     {
    //         x++;
    //     }
    // }

    // // // Second half
    // max = 6;
    // x = 7;
    // y = 1;
    // for(int i = 1; i < 7; i++, max--)
    // {
    //     for(int j = 0; j < max; j++)
    //     {
    //         vet[x][y] = rec_block[ptr_rec_block++];
    //         // Logic Here
    //         if((i % 2) == 0)
    //         {
    //             x++;
    //             y--;
    //         }
    //         else 
    //         {
    //             x--;
    //             y++;
    //         }
    //     }
    //     vet[x][y] = rec_block[ptr_rec_block++];
    //     // Logic Here
    //     if((i % 2) == 0)
    //     {
    //         y++;
    //     }
    //     else 
    //     {
    //         x++;
    //     }
    // }
    // vet[x][y] = rec_block[ptr_rec_block++];
    // Logic Here
}

int fill_buffer(BUFFER *buffer, unsigned int data, int cat)
{
    int status = -1;
    if(cat == 0)
    {
        if(buffer->remaining_bits >= 11) // The necessary bits plus EOB prefix
        {
            buffer->buffer = (buffer->buffer << 3) | data;
            buffer->remaining_bits -= 3;
            status = 0;
        }
    }
    else if(cat == 1)
    {
        if(buffer->remaining_bits >= 12)
        {
            buffer->buffer = (buffer->buffer << 4) | data;
            buffer->remaining_bits -= 4;
            status = 0;
        }
    }
    else if(cat == 2)
    {
        if(buffer->remaining_bits >= 13)
        {
            buffer->buffer = (buffer->buffer << 5) | data;
            buffer->remaining_bits -= 5;
            status = 0;
        }
    }
    else if(cat == 3)
    {
        if(buffer->remaining_bits >= 13)
        {
            buffer->buffer = (buffer->buffer << 5) | data;
            buffer->remaining_bits -= 5;
            status = 0;
        }
    }
    else if(cat == 4)
    {
        if(buffer->remaining_bits >= 15)
        {
            buffer->buffer = (buffer->buffer << 7) | data;
            buffer->remaining_bits -= 7;
            status = 0;
        }
    }
    else if(cat == 5)
    {
        if(buffer->remaining_bits >= 16)
        {
            buffer->buffer = (buffer->buffer << 8) | data;
            buffer->remaining_bits -= 8;
            status = 0;
        }
    }
    else if(cat == 6)
    {
        if(buffer->remaining_bits >= 18)
        {
            buffer->buffer = (buffer->buffer << 10) | data;
            buffer->remaining_bits -= 10;
            status = 0;
        }
    }
    else if(cat == 7)
    {
        if(buffer->remaining_bits >= 20)
        {
            buffer->buffer = (buffer->buffer << 12) | data;
            buffer->remaining_bits -= 12;
            status = 0;
        }
    }
    else if(cat == 8)
    {
        if(buffer->remaining_bits >= 22)
        {
            buffer->buffer = (buffer->buffer << 14) | data;
            buffer->remaining_bits -= 14;
            status = 0;
        }
    }
    else if(cat == 9)
    {
        if(buffer->remaining_bits >= 24)
        {
            buffer->buffer = (buffer->buffer << 16) | data;
            buffer->remaining_bits -= 16;
            status = 0;
        }
    }
    else if(cat == 10)
    {
        if(buffer->remaining_bits >= 26)
        {
            buffer->buffer = (buffer->buffer << 18) | data;
            buffer->remaining_bits -= 18;
            status = 0;
        }
    }
    return status;
}

BMP_CHANNELS *bmp_get_channels()
{

}

void bmp_free_channels(BMP_FILE **bmp)
{
    if((*bmp) != NULL)
    {
        for(int i = 0; i < (*bmp)->channels.qt_blocks; i++)
        {
            for(int j = 0; j < 8; j++)
            {
                free((*bmp)->channels.y[i][j]);
                free((*bmp)->channels.cb[i][j]);
                free((*bmp)->channels.cr[i][j]);
            }
            free((*bmp)->channels.y[i]);
            free((*bmp)->channels.cb[i]);
            free((*bmp)->channels.cr[i]);
            (*bmp)->channels.y[i] = NULL;
            (*bmp)->channels.cb[i] = NULL;
            (*bmp)->channels.cr[i] = NULL;
        }
        free((*bmp)->channels.y);
        free((*bmp)->channels.cb);
        free((*bmp)->channels.cr);
    }
}

void bmp_free_huff_channels(BMP_FILE **bmp)
{
    if((*bmp) != NULL)
    {
        for(int i = 0; i < (*bmp)->channels.qt_blocks; i++)
        {
            for(int j = 0; j < 8; j++)
            {
                free((*bmp)->channels.huff_y[i][j]);
                free((*bmp)->channels.huff_cb[i][j]);
                free((*bmp)->channels.huff_cr[i][j]);
            }
            free((*bmp)->channels.huff_y[i]);
            free((*bmp)->channels.huff_cb[i]);
            free((*bmp)->channels.huff_cr[i]);
            (*bmp)->channels.huff_y[i] = NULL;
            (*bmp)->channels.huff_cb[i] = NULL;
            (*bmp)->channels.huff_cr[i] = NULL;
        }
        free((*bmp)->channels.huff_y);
        free((*bmp)->channels.huff_cb);
        free((*bmp)->channels.huff_cr);
    }
}

void bmp_destroy(BMP_FILE **bmp)
{
    if((*bmp) != NULL)
    {
        bmp_free_channels(bmp);
        if((*bmp)->channels.huff_y != NULL && (*bmp)->channels.huff_cb != NULL && (*bmp)->channels.huff_cr != NULL)
        {
            bmp_free_huff_channels(bmp);
        }
        free((*bmp));
        (*bmp) = NULL;
    }
}


void print_zigzag(double **arr)
{
    int x = 0, y = 1, max = 2;

    // First half
    printf("%.0lf\n", arr[0][0]);
    for(int i = 1; i < 8; i++, max++)
    {
        for(int j = 1; j < max; j++)
        {
            printf("%.0lf ", arr[x][y]);
            if((i % 2) == 0)
            {
                x--;
                y++;
            }
            else 
            {
                x++;
                y--;
            }

        }
        
        printf("%.0lf\n", arr[x][y]);
        if((i % 2) == 0)
        {
            y++;
        }
        else 
        {
            x++;
        }
    }
    // Second half
    max = 6;
    x = 7;
    y = 1;
    for(int i = 1; i < 7; i++, max--)
    {
        for(int j = 0; j < max; j++)
        {
            printf("%.0lf ", arr[x][y]);
            if((i % 2) == 0)
            {
                x++;
                y--;
            }
            else 
            {
                x--;
                y++;
            }
        }
        printf("%.0lf\n", arr[x][y]);
        if((i % 2) == 0)
        {
            y++;
        }
        else 
        {
            x++;
        }
    }
    printf("%.0lf\n", arr[x][y]);
    // printf("X: %d \t Y: %d\n", x, y);
    printf("\n");
}
