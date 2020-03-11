#include <stdio.h>
#include <bmp_handler.h>

int main(int argc, char *argv[])
{
	BMP_FILE *bmp = bmp_read_file("asd.bmp");
	bmp_write_file("teste.bmp", bmp);
	bmp_destroy(&bmp);
	return 0;
}
