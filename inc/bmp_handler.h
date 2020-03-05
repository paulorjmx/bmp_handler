#ifndef BMP_HANDLER_H
	#define BMP_HANDLER_H

		typedef struct t_bmp_channels BMP_CHANNELS;
		typedef struct t_bmp_file BMP_FILE;

		void bmp_read_file(const char *file_name);
		BMP_CHANNELS *bmp_get_channels();
#endif
