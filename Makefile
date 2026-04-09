# Makefile for the Standard Cell Legalizer
CXX = g++

CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET = legalizer

SRCS = main.cpp Parser.cpp Diffusion.cpp Legalizer.cpp Writer.cpp

HDRS = Parser.h Diffusion.h Legalizer.h Writer.h

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

