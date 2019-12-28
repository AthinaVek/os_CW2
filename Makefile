objects = ergasia2.o functions2.o

all: $(objects)
	gcc -g $(objects) -o ergasia2

ergasia2.o: ergasia2.c
	gcc -g -c ergasia2.c

functions2.o: functions2.c
	gcc -g -c functions2.c

clean:
	rm -f $(objects) ergasia2