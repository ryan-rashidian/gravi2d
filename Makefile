CC = gcc
TARGET = gravi2d

DIR_BUILD = build
DIR_SRC = src

CFLAGS = -MMD -MP -Wall -Wextra -pedantic -std=c99 -I$(DIR_SRC)
CFLAGS += $(shell pkg-config --cflags raylib)
DEBUG ?= 0
ifeq ($(DEBUG),1)
CFLAGS += -g -Og
else
CFLAGS += -O2
endif

LIBS = $(shell pkg-config --libs raylib) -lm

SRC = $(wildcard $(DIR_SRC)/*.c)
OBJS = $(patsubst $(DIR_SRC)/%.c,$(DIR_BUILD)/%.o,$(SRC))
DEPS = $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(DIR_BUILD)/%.o: $(DIR_SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

.PHONY: all clean
clean:
	rm -rf $(DIR_BUILD) $(TARGET)

