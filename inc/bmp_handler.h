#ifndef BMP_HANDLER_H
	#define BMP_HANDLER_H

		typedef struct t_bmp_channels BMP_CHANNELS;
		typedef struct t_bmp_file BMP_FILE;

		BMP_FILE *bmp_read_file(const char *);
		int bmp_write_file(const char *, BMP_FILE *);
		BMP_CHANNELS *bmp_get_channels();
		void bmp_destroy(BMP_FILE **);
#endif
