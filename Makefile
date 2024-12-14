CXX = g++
CXXFLAGS = -std=c++17
OPENSSL_PREFIX = $(shell brew --prefix openssl)
INCLUDE = -I$(OPENSSL_PREFIX)/include
LIBS = -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto -lcurl -lz
TEST_LIBS = -L$(OPENSSL_PREFIX)/lib -lgtest -lgtest_main -pthread -lssl -lcrypto
SRC = libs/miniz/miniz.c src/main.cpp
TEST_SRC = tests/main.test.cpp
OUT = build/bittrack
TEST_OUT = build/RunTests

build:
	mkdir -p build

compile: build
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(LIBS) $(SRC) -o $(OUT)

test: build
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(TEST_LIBS) $(TEST_SRC) -o $(TEST_OUT)
	./build/RunTests

clean:
	rm -rf build
	rm -rf .bittrack
