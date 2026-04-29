CC = gcc
CFLAGS = -Wall -Wextra -Iinclude `pkg-config --cflags gtk+-3.0`
LDFLAGS = -lcurl -lsqlite3 `pkg-config --libs gtk+-3.0`

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
DATA_DIR = data

TARGET = $(BIN_DIR)/aura_cli

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Créer les dossiers si nécessaire et compiler la cible
all: $(OBJ_DIR) $(BIN_DIR) $(DATA_DIR) $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(DATA_DIR):
	mkdir -p $(DATA_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyer les objets (.o) et l'exécutable (bin/)
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

.PHONY: all clean
