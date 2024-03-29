SOURCES=main.c parse.c net.c sd.c gpio.c
OBJECTS=$(SOURCES:.c=.o)
EXEC=sdserver
MY_CFLAGS += -Wall -Werror -O0 -g
MY_LIBS += 

all: $(OBJECTS)
	$(CC) $(LIBS) $(LDFLAGS) $(OBJECTS) $(MY_LIBS) -o $(EXEC)

clean:
	rm -f $(EXEC) $(OBJECTS)

.c.o:
	$(CC) -c $(CFLAGS) $(MY_CFLAGS) $< -o $@
