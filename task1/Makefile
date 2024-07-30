CXX=g++
OBJ=main.o eshell.o parser.o
CXXFLAGS=-Wall -std=c++11

eshell: $(OBJ)
	$(CXX) $(CXXFLAGS) -o eshell $(OBJ)

main.o: main.cpp eshell.h parser.h
	$(CXX) $(CXXFLAGS) -c main.cpp

eshell.o: eshell.cpp eshell.h parser.h
	$(CXX) $(CXXFLAGS) -c eshell.cpp

parser.o: parser.c parser.h
	$(CXX) $(CXXFLAGS) -c parser.c

clean:
	rm -f $(OBJ) eshell



