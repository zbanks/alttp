$(shell mkdir -p build >/dev/null)

C_SRC = \
	alttp.c \
	ap_map.c \
	ap_snes.c \
	ap_plan.c \
	ap_graph.c \
	pq.c \

OBJECTS = $(C_SRC:%.c=build/%.o)
TARGET = build/libalttp.a

INC = -I.
LIB = -L/usr/local/lib

DEPS = $(OBJECTS:.o=build/.d)
-include $(DEPS)

# compile and generate dependency info;
build/%.o: %.c | $(wildcard *.h)
	$(CC) -c $(CFLAGS) $*.c -o build/$*.o
	$(CC) -MM $(CFLAGS) $*.c > build/$*.d

# Assembler, compiler, and linker flags
override CFLAGS += $(INC) -O3 -ggdb3 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused -Wwrite-strings -std=c11 -D_POSIX_C_SOURCE=201810L -fPIC 
override LFLAGS += $(LIB)
LIBS =

# Targets
.PHONY: clean all
.DEFAULT_GOAL = all
all: $(TARGET)

clean:
	-rm -rf build/

$(TARGET): $(OBJECTS)
	$(AR) cr $@ $^
