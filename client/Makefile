CC = g++
CFLAGS = -Wall
DEPS = client_functions.h
OBJ = client.o utils.o server_utils.o client_utils.o
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

client: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -ljsoncpp -lcrypto
 
clean:
	rm -rf *o client