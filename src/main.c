#include <stdio.h>
#include <string.h>
#include <bmp_handler.h>

int main(int argc, char *argv[])
{
	BMP_FILE *bmp = NULL;
	char in_file[100], out_file[100];
	if(argc == 4)
	{
		if(strcmp(argv[1], "-c") == 0)
		{
			strncpy(in_file, argv[2], sizeof(in_file));
			strncpy(out_file, argv[3], sizeof(out_file));
			bmp = bmp_read_file(in_file);
			bmp_dct(bmp, 0);
			bmp_quantization(bmp);
			bmp_diff_encode(bmp);
			bmp_compress(bmp, out_file);
		}
		else if(strcmp(argv[1], "-d") == 0)
		{
			strncpy(in_file, argv[2], sizeof(in_file));
			strncpy(out_file, argv[3], sizeof(out_file));
			bmp = bmp_decompress(in_file);
			bmp_diff_decode(bmp);
			bmp_inverse_quantization(bmp);
			bmp_dct(bmp, -1);
			bmp_write_file(out_file, bmp);
		}
		else 
		{
			printf("Invalid arguments!\n");
			printf("For use: ./main [-c | -d] <input_file_name> <output_file_name>\n");
			printf("IMPORTANT: For -c argument, the input file must have .bmp extension, and for -d argument, output file must have .bmp extension\n");
		}
		bmp_destroy(&bmp);
	}
	else 
	{
		printf("Few arguments!\n");
		printf("For use: ./main [-c | -d] <input_file_name> <output_file_name>\n");
		printf("IMPORTANT: For -c argument, the input file must have .bmp extension, and for -d argument, output file must have .bmp extension\n");
	}
	return 0;
}
