VERSE_INC=../verse-trunk/include
VERSE_LIB=../verse-trunk/lib/libverse.a

CC=gcc
CFLAGS=-D_REETRANT -ggdb -I$(VERSE_INC) -Wall -Wextra -pedantic
LFLAGS=-D_REETRANT -ggdb -lssl -lcrypto -lpthread -lGL -lGLU -lglut -lm
INCLUDE=./include

OBJ=./obj
BIN=./bin

TARGETS=$(BIN)/verse_particle

all: $(TARGETS)

VERSE_CLIENT_OBJ=$(OBJ)/client.o \
					$(OBJ)/lu_table.o \
					$(OBJ)/client_particle_sender.o \
					$(OBJ)/client_particle_receiver.o \
					$(OBJ)/particle_data.o \
					$(OBJ)/display_glut.o \
					$(OBJ)/math_lib.o \
					$(OBJ)/particle_scene_node.o \
					$(OBJ)/particle_sender_node.o \
					$(OBJ)/particle_node.o \
					$(OBJ)/timer.o \
					$(OBJ)/sender.o

$(OBJ)/%.o: ./src/%.c
	$(CC) -c -o $@ $(CFLAGS) -I$(INCLUDE) $<

$(BIN)/verse_particle: $(VERSE_CLIENT_OBJ)
	$(CC) -o $@ $^ $(VERSE_LIB) $(LFLAGS)

clean:
	rm -f $(TARGETS) $(OBJ)/*.o
