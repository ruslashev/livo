all:
	g++ *.cc -o livo -std=c++0x -g -lglfw -lGLEW -lGL -lfreetype -I/usr/include/freetype2
	./livo

