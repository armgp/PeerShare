CC = g++
CFLAGS = -W
DEPS = tracker_functions.h
OBJ = tracker.o utils.o server_utils.o client_utils.o
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

tracker: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -ljsoncpp -lcrypto
 
clean:
	rm -rf *o tracker