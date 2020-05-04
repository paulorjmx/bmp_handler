#include <stdio.h>
#include <error_handler.h>

void error_catch(unsigned int err_code)
{
    switch(err_code)
    {
        case ERR_EMPTY_FILE_NAME:
            printf("ERROR: Type a file name!\n");
            break;

        case ERR_COULD_NOT_OPEN_FILE:
            printf("ERROR: Some error ocurred on open the file!\n");
            break;

        case ERR_ALLOCATE_MEMORY:
            printf("ERROR: Was not possible to allocate memory\n");
            break;
        
        case ERR_NOT_BITMAP:
            printf("ERROR: The file is not a bmp file!\n");
            break;

        case ERR_CREATE_BITMAP:
            printf("ERROR: Was not possible to create new bmp file!\n");
            break;
        
        case ERR_BMP_NOT_EXIST:
            printf("ERROR: You must write a valid bmp file!\n");
            break;

        default:
            break;
    }
}