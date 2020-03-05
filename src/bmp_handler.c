#include <bmp_handler.h>

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
	unsigned short bmpReserver2; // Reserved for application
	struct t_bmp_info_header info_header; // Rest of BMP Header
} BMP_CHANNELS;

struct t_bmp_channels
{
    char **r, **g, **b;
};

struct t_bmp_file
{
    struct t_bmp_info_header;
    struct t_bmp_channels;
};