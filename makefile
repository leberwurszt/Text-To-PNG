all:	main.cpp
		g++ -c main.cpp
		g++ -o tti main.o -lSDL2 -lSDL2_image
		rm main.o