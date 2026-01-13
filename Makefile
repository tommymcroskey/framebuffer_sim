CC	= gcc
CFLAGS	= -Wall -O2
LFLAGS = -lm
TARGET	= sim

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
