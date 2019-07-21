
CC=g++
CFLAGS=-I.

LIBS=

DEPS = 

OBJ = main.o 

PROD = tcpserver

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(PROD): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(PROD) *.o *~ core 