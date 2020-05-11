
default: build-all

WORLD ?=
GAZEBO_ARGS ?= --verbose

EXAMPLE_ARGS ?=

.PHONY: build-all clean run tests-roficom format-all example-server example-client help

build:
	mkdir -p build
	cd build && cmake ..

build-all: build
	cd build && make -j8
	@echo "\nBuild completed\n\n"

clean:
	rm -rf build

run: build-all
	./runGazebo.sh ${GAZEBO_ARGS} ${WORLD}

tests-roficom: build-all
	./build/plugins/roficom/tests/tests_roficom

format-all:
	find ./ ! \( -path "./build/*" \) -a \( -type f -iname *.hpp -o -type f -iname *.cpp \) | xargs clang-format --style=file -i

example-server: build-all
	@test -f ./examples/${EXAMPLE}/${EXAMPLE}.world || \
		( echo "ERROR: No example world for '${EXAMPLE}' found\nCall 'make help' for more info\n" ; exit 1 )
	./runGazebo.sh ./examples/${EXAMPLE}/${EXAMPLE}.world ${GAZEBO_ARGS}

example-client: build-all
	@test -f ./build/examples/${EXAMPLE}/${EXAMPLE} || \
		( echo "ERROR: No example '${EXAMPLE}' found\nCall 'make help' for more info\n" ; exit 1 )
	./build/examples/${EXAMPLE}/${EXAMPLE} ${EXAMPLE_ARGS}

help:
	@echo "Available targets:"
	@echo "\n\t[build-all]"
	@echo "\t\tBuilds the project."
	@echo "\n\tclean"
	@echo "\t\tRemoves the build directory."
	@echo "\n\trun [GAZEBO_ARGS=<gazebo_args>] [WORLD=<path_to_world>]"
	@echo "\t\tRuns gazebo, args default to '--verbose'."
	@echo "\n\ttests-roficom"
	@echo "\t\tRuns tests for roficom."
	@echo "\n\tformat-all"
	@echo "\t\tFormats all '.hpp' and '.cpp' files."
	@echo "\n\texample-server EXAMPLE=<example_name> [GAZEBO_ARGS=<gazebo_args>]"
	@echo "\t\tRuns gazebo for the example, args default to '--verbose'."
	@echo "\n\texample-client EXAMPLE=<example_name> [EXAMPLE_ARGS=<example_args>]"
	@echo "\t\tRuns the RoFISim client for the example."
	@echo "\n\thelp"
	@echo "\t\tPrints this help."
