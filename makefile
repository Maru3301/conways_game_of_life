TARGET = main
CFLAGS = -g -Wextra
PARAMS = 255 130
LIBS = -lm -lpthread -lSDL2

HEADERS = $(wildcard *.h)
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

.PHONY: all run clean debug memcheck

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

memcheck: $(TARGET)
	valgrind --leak-check=full ./$(TARGET) $(PARAMS)
