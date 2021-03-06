TARGET = main
CFLAGS = -g -Wextra
PARAMS = 240 120 perlin 125
LIBS = -lm -lSDL2

HEADERS = $(wildcard *.h)
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

.PHONY: all run clean debug valgrind

all: $(TARGET)

%.o: %.c $(HEADERS)
	gcc $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	gcc $(OBJECTS) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET) $(PARAMS)

debug: $(TARGET)
	gdb -ex="break main" -ex="layout next" -ex="run" --args ./$(TARGET) $(PARAMS)

valgrind: $(TARGET)
	valgrind --leak-check=full ./$(TARGET) $(PARAMS)
