#include <bmp_handler.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define BMP_SIG 0x4D42 // Bitmap file identification

// Cosine table for fast DCT calculation
const double COS[8][8] = { { 1.000000,1.000000,1.000000,1.000000,1.000000,1.000000,1.000000,1.000000 },
                            { 0.980785,0.831470,0.555570,0.195090,-0.195090,-0.555570,-0.831470,-0.980785 },
                            { 0.923880,0.382683,-0.382683,-0.923880,-0.923880,-0.382683,0.382683,0.923880 },
                            { 0.831470,-0.195090,-0.980785,-0.555570,0.555570,0.980785,0.195090,-0.831470 },
                            { 0.707107,-0.707107,-0.707107,0.707107,0.707107,-0.707107,-0.707107,0.707107 },
                            { 0.555570,-0.980785,0.195090,0.831470,-0.831470,-0.195090,0.980785,-0.555570 },
                            { 0.382683,-0.923880,0.923880,-0.382683,-0.382683,0.923880,-0.923880,0.382683 },
                            { 0.195090,-0.555570,0.831470,-0.980785,0.980785,-0.831470,0.555570,-0.195090 } };

void bmp_free_channels(BMP_FILE **); // Function to free memory used by channels
void foward_dct(double **);

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
                    printf("ERROR: The file is not a bmp file!\n");
                }
            }
            else 
            {
                printf("ERROR: Was not possible to allocate memory\n");
            }
        }
        else 
        {
            printf("ERROR: Some error ocurred on open the file!\n");
        }
        fclose(arq);
    }
    else 
    {
        printf("ERROR: Type a file name!\n");
    }
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
                printf("ERROR: Was not possible to create new bmp file!\n");
                err = -3;
            }
            fclose(arq);
        }
        else 
        {
            printf("ERROR: You must write a valid bmp file!\n");
            err = -1;
        }
    }
    else 
    {
        printf("ERROR: Type a name to new file");
        err = -2;
    }
    return err;
}

void bmp_dct(BMP_FILE *bmp)
{
    if(bmp != NULL)
    {
        // for(int i = 0; i < (bmp->header.info_header.bmpHeight / 8); i += 8)
        // {
            // for(int j = 0; j < (bmp->header.info_header.bmpWidth / 8); j += 8)
            // {
                // foward_dct((bmp->channels.r + i + j));
                foward_dct((bmp->channels.y));
                // foward_dct((bmp->channels.g + i + j));
                // foward_dct((bmp->channels.b + i + j));
            // }
        // }

    }
    else 
    {
        //Error bmp empty
    }
}

void foward_dct(double **channel)
{
    double sum = 0.0, ci = 0.0, cj = 0.0;
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
            if(i == 0) ci = (1.0 / sqrt(2.0)); else ci = 1;
            if(j == 0) cj = (1.0 / sqrt(2.0)); else cj = 1;
            channel[i][j] = (1.0 / 4.0) * ci * cj * sum;
            // printf("%8.2lf ", channel[i][j]);
        }
        // printf("\n");
    }
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