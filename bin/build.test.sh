g++ -std=c++17 \
    -I"$(brew --prefix openssl)/include" \
    -L"$(brew --prefix openssl)/lib" \
    $(ls src/*.cpp | grep -v 'main.cpp') libs/miniz/miniz.c tests/*.cpp \
    -lgtest -lgtest_main -pthread -lssl -lcrypto -lcurl -lz \
    -o build/run_tests
