#ifndef ERROR_HANDLER_H
    #define ERROR_HANDLER_H

        #define ERR_EMPTY_FILE_NAME 100
        #define ERR_COULD_NOT_OPEN_FILE 150
        #define ERR_ALLOCATE_MEMORY 200
        #define ERR_NOT_BITMAP 250
        #define ERR_CREATE_BITMAP 300
        #define ERR_BMP_NOT_EXIST 350

        void error_catch(unsigned int err_code);
#endif