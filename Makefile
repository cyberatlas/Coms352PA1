CXX = g++
CXXFLAGS = -Wall -g

myShell: main.o
	$(CXX) $(CXXFLAGS) main.o -o myShell

main.o: main.cpp
	g++ -c main.cpp

clean:
	rm *.o

