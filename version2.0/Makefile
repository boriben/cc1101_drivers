CXX = g++
CXXFLAGS = -Wall -Wextra -O2
LDFLAGS =

SRCS = main.cpp main_drivers.cpp helper_functions.cpp
OBJS = $(SRCS:.cpp=.o)

TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
