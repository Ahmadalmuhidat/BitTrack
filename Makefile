build:
	mkdir -p build

compile: build
	./bin/build.production.sh

test: build
	./bin/build.test.sh
	./build/run_tests

clean:
	rm -rf build
	rm -rf .bittrack
