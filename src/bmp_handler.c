#include <bmp_handler.h>
#include <error_handler.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define BMP_SIG 0x4D42 // Bitmap file identification
#define SQRT_2 1.414214

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
void foward_dct(double **); // Calculates the foward DCT-II in 8x8 blocks
void inverse_dct(double **); // Calculates the inverse DCT-II in 8x8 blocks
void quantization_luminance(double **); // Apply the quantization in luminance channel
void inverse_quantization_luminance(double **); // Apply the inverse quantization in luminance channel
void quantization_chrominance(double **); // Apply the quantization in chrominance channel
void inverse_quantization_chrominance(double **); // Apply the inverse quantization in chrominance channel
void calculate_difference(double **);

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
    double **y, **cb, **cr;
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
                                        
                    // Alloc pixels
                    bmp->channels.y = (double **) malloc(sizeof(double *) * bmp->header.info_header.bmpHeight);
                    bmp->channels.cb = (double **) malloc(sizeof(double *) * bmp->header.info_header.bmpHeight);
                    bmp->channels.cr = (double **) malloc(sizeof(double *) * bmp->header.info_header.bmpHeight);
                    for(int i = 0; i < bmp->header.info_header.bmpHeight; i++)
                    {
                        bmp->channels.y[i] = (double *) malloc(sizeof(double) * bmp->header.info_header.bmpWidth);
                        bmp->channels.cb[i] = (double *) malloc(sizeof(double) * bmp->header.info_header.bmpWidth);
                        bmp->channels.cr[i] = (double *) malloc(sizeof(double) * bmp->header.info_header.bmpWidth);
                    }

                    // Read pixels
                    fseek(arq, bmp->header.bmpPixelDataOffset, SEEK_SET);
                    for(int i = 0; i < bmp->header.info_header.bmpHeight; i++)
                    {
                        for(int j = 0; j < bmp->header.info_header.bmpWidth; j++)
                        {
                            fread(&b, sizeof(unsigned char), 1, arq);
                            fread(&g, sizeof(unsigned char), 1, arq);
                            fread(&r, sizeof(unsigned char), 1, arq);
                            bmp->channels.y[i][j] = (0.299 * r) + (0.587 * g) + (0.114 * b);
                            bmp->channels.cb[i][j] = 0.564 * (b - bmp->channels.y[i][j]);
                            bmp->channels.cr[i][j] = 0.713 * (r - bmp->channels.y[i][j]);
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
                for(int i = 0; i < bmp->header.info_header.bmpHeight; i++)
                {
                    for(int j = 0; j < bmp->header.info_header.bmpWidth; j++)
                    {
                        r = (unsigned char) (bmp->channels.y[i][j] + (1.402 * bmp->channels.cr[i][j]));
                        g = (unsigned char) (bmp->channels.y[i][j] - (0.344 * bmp->channels.cb[i][j]) - (0.714 * bmp->channels.cr[i][j]));
                        b = (unsigned char) (bmp->channels.y[i][j] + (1.772 * bmp->channels.cb[i][j]));
                        fwrite(&b, sizeof(unsigned char), 1, arq);
                        fwrite(&g, sizeof(unsigned char), 1, arq);
                        fwrite(&r, sizeof(unsigned char), 1, arq);
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
        // for(int i = 0; i < 8; i++)
        // {
        //     for(int j = 0; j < 8; j++)
        //     {
        //         printf("%8.1lf ", bmp->channels.y[i][j]);
        //     }
        //     printf("\n");
        // }

        // printf("\n");
        if(type == 0) // If it is the foward DCT-II
        {
            for(int i = 0; i < (bmp->header.info_header.bmpHeight / 8); i += 8)
            {
                for(int j = 0; j < (bmp->header.info_header.bmpWidth / 8); j += 8)
                {
                    foward_dct((bmp->channels.y + i + j));
                    foward_dct((bmp->channels.cb + i + j));
                    foward_dct((bmp->channels.cr + i + j));
                }
            }
            // foward_dct(bmp->channels.y);
        }
        else if(type == -1) // If it is the inversed DCT-II
        {
            for(int i = 0; i < (bmp->header.info_header.bmpHeight / 8); i += 8)
            {
                for(int j = 0; j < (bmp->header.info_header.bmpWidth / 8); j += 8)
                {
                    inverse_dct((bmp->channels.y + i + j));
                    inverse_dct((bmp->channels.cb + i + j));
                    inverse_dct((bmp->channels.cr + i + j));
                }
            }
            // inverse_dct(bmp->channels.y);
        }
        // for(int i = 0; i < 8; i++)
        // {
        //     for(int j = 0; j < 8; j++)
        //     {
        //         printf("%8.1lf ", bmp->channels.y[i][j]);
        //     }
        //     printf("\n");
        // }
        // printf("\n");
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
        for(int i = 0; i < (bmp->header.info_header.bmpHeight / 8); i += 8)
        {
            for(int j = 0; j < (bmp->header.info_header.bmpWidth / 8); j += 8)
            {
                quantization_luminance((bmp->channels.y + i + j));
                quantization_chrominance((bmp->channels.cb + i + j));
                quantization_chrominance((bmp->channels.cr + i + j));
            }
        }
        // quantization_luminance(bmp->channels.y);
        // for(int i = 0; i < 8; i++)
        // {
        //     for(int j = 0; j < 8; j++)
        //     {
        //         printf("%8.1lf ", bmp->channels.y[i][j]);
        //     }
        //     printf("\n");
        // }
        // printf("\n");
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
        for(int i = 0; i < (bmp->header.info_header.bmpHeight / 8); i += 8)
        {
            for(int j = 0; j < (bmp->header.info_header.bmpWidth / 8); j += 8)
            {
                inverse_quantization_luminance((bmp->channels.y + i + j));
                inverse_quantization_chrominance((bmp->channels.cb + i + j));
                inverse_quantization_chrominance((bmp->channels.cr + i + j));
            }
        }

        // inverse_quantization_luminance(bmp->channels.y);
        // for(int i = 0; i < 8; i++)
        // {
        //     for(int j = 0; j < 8; j++)
        //     {
        //         printf("%8.1lf ", bmp->channels.y[i][j]);
        //     }
        //     printf("\n");
        // }
        // printf("\n");
    }
    else 
    {
        ERROR = ERR_BMP_NOT_EXIST;
    }
    error_catch(ERROR);
}

void bmp_diff_encode(BMP_FILE *bmp)
{
    if(bmp != NULL)
    {

    }
    else
    {
        ERROR = ERR_BMP_NOT_EXIST;
    }
    error_catch(ERROR);
    
}

void calculate_difference(double **channel)
{
    int i = 0, j = 1;
    double current = channel[0][0];
}

BMP_CHANNELS *bmp_get_channels()
{

}

void bmp_free_channels(BMP_FILE **bmp)
{
    if((*bmp) != NULL)
    {
        for(int i = 0; i < (*bmp)->header.info_header.bmpHeight; i++)
        {
            free((*bmp)->channels.y[i]);
            free((*bmp)->channels.cb[i]);
            free((*bmp)->channels.cr[i]);
        }
        free((*bmp)->channels.y);
        free((*bmp)->channels.cb);
        free((*bmp)->channels.cr);
        (*bmp)->channels.y = NULL;
        (*bmp)->channels.cb = NULL;
        (*bmp)->channels.cr = NULL;
    }
}

void bmp_destroy(BMP_FILE **bmp)
{
    if((*bmp) != NULL)
    {
        bmp_free_channels(bmp);
        free((*bmp));
        (*bmp) = NULL;
    }
}