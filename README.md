# Image compression/decompression project (utilizing JPEG approach)

### Contributors

+ Paulo Ricardo J. Miranda
+ Victor Luiz França


### Installation
#### NOTE
To execute the commands below, make sure you have the GNU Compiler Collection (GCC). In Linux, probaly you have it.
In order to compile in Windows, the example below suppose that you have added GCC in the Path environment variable.

+ Linux  
    If you have a Linux distribution, do:

    ```sh
    $ make
    ```

    The command above, just compile the project and install it in the ```bin``` folder

    To run it, run the command:

    ```sh
    $ ./bin/main [-c | -d] <input_file_name.extension> <output_file_name.extension>
    ```

+ Windows  
In case if you have a Makefile installed on Windows, just follow the same steps in the Linux section.
However, if don't you have a Makefile installed, run the ```cmd``` inside the folder of project, an type:

    ```sh
    $ gcc -Iinc src\main.c src\bmp_handler.c -o bin\main
    ```
    To run it:

    ```sh
    $ bin/main [-c | -d] <input_file_name.extension> <output_file_name.extension>
    ```

### About
The programa is organized in folders:
+ src: is the folder holding the .c files
+ inc: is the folder holding the .h files

In the src folder we have:
+ bmp_handler.c: file wich contains code to manipulate BMP_FILE data structure 
+ error_handler.c: this file contains manipulation of errors
+ main.c: the file which contains the main function.

The program creates a data structure called BMP_FILE which contains the header of the .bmp file and the channels YCbCr. The entire process in the pipeline will apply transformations to this structure, specifically in the 8x8 YCbCr blocks.

If the compression argument is choosed (the -c in the argument), the program will open the input file to get information about the file (reading the header) and get the RGB colorspace, dimension of the image, among other things. In this step, the program will convert the RGB colorspace to YCbCr colorspace. After that, the program will apply the DCT transformation, Quantization, Diff Encode, and compress. In the compress stage, the program will take all the blocks and compute, for each element in the block, the huffman code for that element and puts in a 8 bytes long buffer.

If the decompression argument is choosed (the -d in the argument), the program will open the input file to get information about the file (reading the header). The programa will make all inverse steps taken in the previous description.

## IMPORTANT
+ All the 8x8 blocks are the dynamic double 3d array (triple pointers)
+ The program have a compression rate between 30% to 50%
+ The decompression does not work as expected, that is, most of the images tested are not uncompressed, causing a "total loss".