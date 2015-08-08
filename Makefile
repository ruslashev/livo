all:
	g++ *.cc -o livo -std=c++0x -g -lglfw -lGLEW -lGL && ./livo

