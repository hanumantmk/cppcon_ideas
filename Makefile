default: foo

pp: foo.cpp Makefile
	g++ -I/Users/jcarey/Git/mongo/src/third_party/boost-1.56.0 -std=c++14 -Wall -Werror -E foo.cpp

foo: foo.cpp Makefile
	g++ -I/Users/jcarey/Git/mongo/src/third_party/boost-1.56.0 -std=c++14 -Wall -Werror -ggdb3 -O2 -o foo foo.cpp
