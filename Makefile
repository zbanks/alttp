$(shell mkdir -p build >/dev/null)

C_SRC = \
	alttp.c \
	ap_map.c \
	ap_snes.c \
	ap_plan.c \
	ap_req.c \
	ap_item.c \
	pm.c \
	pq.c \
	lb.c \

OBJECTS = $(C_SRC:%.c=build/%.o)
TARGET = build/libalttp.a

INC = -I.
LIB = -L/usr/local/lib

DEPS = $(OBJECTS:%.o=%.d)
-include $(DEPS)

# compile and generate dependency info;
build/%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o build/$*.o
	$(CC) -MM $(CFLAGS) -MT build/$*.o $*.c > build/$*.d

# Assembler, compiler, and linker flags
override CFLAGS += $(INC) -O3 -ggdb3 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused -Wwrite-strings -Wshadow -std=c11 -D_DEFAULT_SOURCE #-flto
override LFLAGS += $(LIB)
LIBS =
AR=gcc-ar

# Targets
.PHONY: clean all
.DEFAULT_GOAL = all
all: $(TARGET)

clean:
	-rm -rf build/

$(TARGET): $(OBJECTS)
	$(AR) cr $@ $^

lb_test: lb.c lb_test.c
	$(CC) $(CFLAGS) -DLB_DEBUG $+ -o $@
