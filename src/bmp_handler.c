#include <bmp_handler.h>
#include <stdio.h>
#include <stdlib.h>

#define BMP_SIG 0x4D42 // Bitmap file identification

void bmp_free_channels(BMP_FILE **bmp); // Function to free memory used by channels

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
    unsigned char **r, **g, **b;
};

struct t_bmp_file
{
    BMP_HEADER header;
    BMP_CHANNELS channels;
};

BMP_FILE *bmp_read_file(const char *file_name)
{
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
                    bmp->channels.r = (unsigned char **) malloc(sizeof(unsigned char *) * bmp->header.info_header.bmpHeight);
                    bmp->channels.g = (unsigned char **) malloc(sizeof(unsigned char *) * bmp->header.info_header.bmpHeight);
                    bmp->channels.b = (unsigned char **) malloc(sizeof(unsigned char *) * bmp->header.info_header.bmpHeight);
                    for(int i = 0; i < bmp->header.info_header.bmpHeight; i++)
                    {
                        bmp->channels.r[i] = (unsigned char *) malloc(sizeof(unsigned char) * bmp->header.info_header.bmpWidth);
                        bmp->channels.g[i] = (unsigned char *) malloc(sizeof(unsigned char) * bmp->header.info_header.bmpWidth);
                        bmp->channels.b[i] = (unsigned char *) malloc(sizeof(unsigned char) * bmp->header.info_header.bmpWidth);
                    }

                    // Read pixels
                    fseek(arq, bmp->header.bmpPixelDataOffset, SEEK_SET);
                    for(int i = 0; i < bmp->header.info_header.bmpHeight; i++)
                    {
                        for(int j = 0; j < bmp->header.info_header.bmpWidth; j++)
                        {
                            fread(&bmp->channels.b[i][j], sizeof(unsigned char), 1, arq);
                            fread(&bmp->channels.g[i][j], sizeof(unsigned char), 1, arq);
                            fread(&bmp->channels.r[i][j], sizeof(unsigned char), 1, arq);
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
                        fwrite(&bmp->channels.b[i][j], sizeof(unsigned char), 1, arq);
                        fwrite(&bmp->channels.g[i][j], sizeof(unsigned char), 1, arq);
                        fwrite(&bmp->channels.r[i][j], sizeof(unsigned char), 1, arq);
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

BMP_CHANNELS *bmp_get_channels()
{

}

void bmp_free_channels(BMP_FILE **bmp)
{
    if((*bmp) != NULL)
    {
        for(int i = 0; i < (*bmp)->header.info_header.bmpHeight; i++)
        {
            free((*bmp)->channels.r[i]);
            free((*bmp)->channels.g[i]);
            free((*bmp)->channels.b[i]);
        }
        free((*bmp)->channels.r);
        free((*bmp)->channels.g);
        free((*bmp)->channels.b);
        (*bmp)->channels.r = NULL;
        (*bmp)->channels.g = NULL;
        (*bmp)->channels.b = NULL;
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