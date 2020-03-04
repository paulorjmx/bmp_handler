CC = gcc
SRC_DIR = ./src
INC_DIR = ./inc
DEST_DIR = ./bin
BIN = main

$(BIN): main.o bmp_handler.o
	$(CC) $^ -o $(DEST_DIR)/$(BIN)

main.o: $(SRC_DIR)/main.c $(INC_DIR)/bmp_handler.h
	$(CC) -I$(INC_DIR) -c $< -o main.o

bmp_handler.o: $(SRC_DIR)/bmp_handler.c $(INC_DIR)/bmp_handler.h
	$(CC) -I$(INC_DIR) -c $< -o bmp_handler.o

run: $(BIN)
	$(DEST_DIR)/$(BIN)

clean:
	rm -rf *.o $(DEST_DIR)/$(BIN)