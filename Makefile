CC=gcc
INC_DIR = include
CFLAGS=-Wall -Werror -Wextra -I$(INC_DIR)

FOR_RELEASE?=1
DEBUG?=0
RELEASE?=1
TEST?=0

ifeq (${DEBUG}, 1)
	CFLAGS += -DDEBUG -g -O0
else 
	CFLAGS += -O2
endif

ifeq (${TEST}, 1)
	CFLAGS += -DTEST
endif

SRCS := src/main.o \
		src/general.o \
		src/netinterfaces.o \
		src/tests.o
OBJ = $(SRCS:.c=.o)
BUILD_OBJ = $(addprefix build/,$(notdir $(OBJ)))
OUTPUT=netman

all: main

main: $(SRCS)
	$(CC) $(CFLAGS) $(INC) $(OBJ) -o $(OUTPUT)

.PHONY: install
install:
	sudo cp ./netman /usr/bin/netman

.PHONY: uninstall
uninstall:
	sudo mv /usr/bin/netman ./netman

.PHONY: clean
clean:
	rm src/*.o
	rm ./netman
