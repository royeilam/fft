TARGET=fft.out
CC=gcc

CFLAGS=-Wall -Wextra -g
CFLAGS+=-I$(INC)
LIBS=-lm `pkg-config --libs portaudio-2.0`

OBJ_DIR=build

VPATH+=src
INC=inc

SRC_LIST=$(notdir $(wildcard $(VPATH)/*.c))
OBJ_LIST=$(addprefix $(OBJ_DIR)/,$(SRC_LIST:%.c=%.obj))

all: $(OBJ_LIST)
	@echo "Linking"
	@mkdir -p target
	@$(CC) $(CFLAGS) -o target/$(TARGET) $^ $(LIBS)

$(OBJ_DIR)/%.obj: %.c
	@echo "Compiling $(notdir $<)"
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $<

print:
	@echo "SRC_LIST = $(SRC_LIST)"
	@echo "OBJ_LIST = $(OBJ_LIST)"

clean:
	rm -rf $(OBJ_DIR)/*
