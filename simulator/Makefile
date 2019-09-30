
default: build-all
.PHONY: clean build-all

build:
	mkdir -p build
	cd build && cmake ..

build-all: build
	cd build && make -j8

clean:
	rm -rf build