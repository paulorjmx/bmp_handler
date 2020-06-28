#include <stdio.h>
#include <string.h>
#include <bmp_handler.h>

int main(int argc, char *argv[])
{
	BMP_FILE *bmp = NULL;
	if(strcmp(argv[1], "-c") == 0)
	{
		bmp = bmp_read_file("samples/rainbowgirl.bmp");
		bmp_dct(bmp, 0);
		bmp_quantization(bmp);
		bmp_diff_encode(bmp);
		// bmp_compute_huffman_code(bmp);
		// bmp_compress(bmp, "teste.co");
	}
	else 
	{
		bmp = bmp_decompress("teste.co");
		// bmp_diff_decode(bmp);
		// bmp_inverse_quantization(bmp);
		// bmp_dct(bmp, -1);
		// bmp_write_file("saida.bmp", bmp);
	}
	bmp_destroy(&bmp);
	return 0;
}
