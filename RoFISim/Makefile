default: build-all

GZ_ARGS ?= --verbose

NAME ?=
ARGS ?=

.PHONY: build-all clean run format-all example

build:
	mkdir -p build
	cd build && cmake ..

build-all: build
	cd build && make -j8
	@echo "\nBuild completed\n\n"

clean:
	rm -rf build

run: build-all
	./runGazebo.sh ${GZ_ARGS}

format-all:
	find ./ ! \( -path "./build/*" \) -a \( -type f -iname *.hpp -o -type f -iname *.cpp \) | xargs clang-format --style=file -i

example: build-all
	./build/examples/${NAME}/${NAME} ${ARGS}
