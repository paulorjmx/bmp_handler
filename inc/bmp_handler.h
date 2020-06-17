#ifndef BMP_HANDLER_H
	#define BMP_HANDLER_H

		typedef struct t_bmp_channels BMP_CHANNELS; // Channels of a BMP file (YCbCr)
		typedef struct t_bmp_file BMP_FILE; // The BMP file representation

		BMP_FILE *bmp_read_file(const char *); // Read a BMP file and return the content stored
		int bmp_write_file(const char *, BMP_FILE *); // Write a BMP file in the disk
		void bmp_dct(BMP_FILE *, char); // Calculates the DCT-II 
		void bmp_quantization(BMP_FILE *); // Apply the quantization in all channels
		void bmp_inverse_quantization(BMP_FILE *); // Apply the inverse quantization in all channels
		void bmp_diff_encode(BMP_FILE *); // Calculate delta encoding for every image 8x8 block
		void bmp_diff_decode(BMP_FILE *); // Decodes delta encoding for every image 8x8 block
		BMP_CHANNELS *bmp_get_channels();
		void bmp_destroy(BMP_FILE **); // Free the memory used by BMP file 
#endif
