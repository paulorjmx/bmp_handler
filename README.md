# Image decompression/compression project

### Installation
#### NOTE
To execute the commands below, make sure you have the GNU Compiler Collection (GCC). In Linux, probaly you have it.
In order to compile in Windows, the example below suppose that you have added GCC in the Path environment variable.

+ Linux
If you have a Linux distribution, do:

    ```sh
    $ make
    ```

    The command above, just compile the project e install it in the ```bin``` folder

    To compile and run it, run the command:

    ```sh
    $ make run
    ```

+ Windows
In case if you have a Makefile installed on Windows, just follow the same steps in the Linux section.
However, if don't you have a Makefile installed, run the ```cmd``` inside the folder of project, an type:

    ```sh
    $ gcc -Iinc src\main.c src\bmp_handler.c -o bin\main
    ```
    To run it:

    ```sh
    bin/main
    ```