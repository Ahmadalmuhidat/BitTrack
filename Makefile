CXX = g++
CXXFLAGS = -std=c++17

OS := $(shell uname)

# Detect OS and set OpenSSL include/lib paths
ifeq ($(OS),Darwin) # macOS
  OPENSSL_PREFIX = $(shell brew --prefix openssl)
  INCLUDE = -I$(OPENSSL_PREFIX)/include
  LIBS = -L$(OPENSSL_PREFIX)/lib
else # Linux (Ubuntu/Debian for CI)
  INCLUDE =
  LIBS =
endif

# Libraries (order matters for linker)
LINK_LIBS = -lcurl -lssl -lcrypto -lz
TEST_LIBS = -lgtest -lgtest_main -pthread -lssl -lcrypto

SRC = libs/miniz/miniz.c src/main.cpp
TEST_SRC = tests/main.test.cpp
OUT = build/bittrack
TEST_OUT = build/RunTests

build:
	mkdir -p build

compile: build
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(LIBS) $(SRC) -o $(OUT) $(LINK_LIBS)

test: build
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(LIBS) $(TEST_SRC) -o $(TEST_OUT) $(TEST_LIBS)
	./$(TEST_OUT)

clean:
	rm -rf build
	rm -rf .bittrack
