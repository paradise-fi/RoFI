default: build-all

GAZEBO ?= gazebo
GAZEBO_ARGS ?= --verbose
WORLD ?=

EXAMPLE ?=
ARGS ?=

SUBFOLDERS = demo examples hal plugins tools

.PHONY: build-all clean run tests-roficom format-all example-server example-client help

build:
	mkdir -p build
	cd build && cmake ..

build-all: build
	cd build && make -j8
	@echo "\nBuild completed\n\n"

${SUBFOLDERS} : build
	cd build/$@ && make -j8
	@echo "\nBuild $@ completed\n\n"

clean:
	rm -rf build

run: plugins
	./setGazeboVariables.sh ${GAZEBO} ${GAZEBO_ARGS} ${WORLD}

tests-roficom: plugins
	./build/plugins/roficom/tests/tests_roficom

format-all:
	find ./ ! \( -path "./build/*" \) -a \( -type f -iname *.hpp -o -type f -iname *.cpp \) | xargs clang-format --style=file -i

example-server: plugins
ifndef EXAMPLE
	$(error EXAMPLE is not set. Call 'make help' for more info)
endif
	@test -f ./examples/${EXAMPLE}/${EXAMPLE}.world || \
		( echo "ERROR: No example world for '${EXAMPLE}' found\nCall 'make help' for more info\n" ; exit 1 )
	./setGazeboVariables.sh ${GAZEBO} ./examples/${EXAMPLE}/${EXAMPLE}.world ${GAZEBO_ARGS}

example-client: build
ifndef EXAMPLE
	$(error EXAMPLE is not set. Call 'make help' for more info)
endif
	cd build && make -j8 ${EXAMPLE}
	@test -f ./build/examples/${EXAMPLE}/${EXAMPLE} || \
		( echo "\nERROR: No example '${EXAMPLE}' found\nCall 'make help' for more info\n" ; exit 1 )
	./build/examples/${EXAMPLE}/${EXAMPLE} ${ARGS}

world-creator: build
	cd build && make worldCreator -j8
	@echo "\nBuild worldCreator completed\n\n"
	./setGazeboVariables.sh ./build/tools/worldCreator/worldCreator ${ARGS}

help:
	@echo "Available targets:"
	@echo "\n\t[build-all]"
	@echo "\t\tBuilds the project."
	@echo "\n\tclean"
	@echo "\t\tRemoves the build directory."
	@echo "\n\trun [GAZEBO=<gazebo_program>] [GAZEBO_ARGS=<gazebo_args>] [WORLD=<path_to_world>]"
	@echo "\t\tRuns gazebo, args default to '--verbose'."
	@echo "\n\ttests-roficom"
	@echo "\t\tRuns tests for roficom."
	@echo "\n\tformat-all"
	@echo "\t\tFormats all '.hpp' and '.cpp' files."
	@echo "\n\texample-server EXAMPLE=<example_name> [GAZEBO=<gazebo_program>] [GAZEBO_ARGS=<gazebo_args>]"
	@echo "\t\tRuns gazebo for the example, args default to '--verbose'."
	@echo "\n\texample-client EXAMPLE=<example_name> [ARGS=<example_args>]"
	@echo "\t\tRuns the RoFISim client for the example."
	@echo "\n\tworld-creator ARGS=\"[-i <input_file>] [-o <output_file>] [-w <world_file>]\""
	@echo "\t\tRuns the World creator tool. (Note the quotation marks around ARGS value.)"
	@echo "\t\tIf not set, uses the standard input/output."
	@echo "\n\thelp"
	@echo "\t\tPrints this help."
