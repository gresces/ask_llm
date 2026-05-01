CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

SRCS = main.cc src/llm.cc src/config.cc
OBJS = bin/main.o bin/src_llm.o bin/src_config.o
TARGET = bin/ask

PREFIX ?= /usr/local
DESTDIR ?=

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/main.o: main.cc | bin
	$(CXX) $(CXXFLAGS) -c -o $@ $<

bin/src_llm.o: src/llm.cc | bin
	$(CXX) $(CXXFLAGS) -c -o $@ $<

bin/src_config.o: src/config.cc | bin
	$(CXX) $(CXXFLAGS) -c -o $@ $<

bin:
	mkdir -p bin

clean:
	rm -rf bin
	rm -f main.o src/llm.o src/config.o ask

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/ask
