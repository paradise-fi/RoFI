default: build-all

GAZEBO ?= gazebo
GZ_ARGS ?= --verbose
WORLD ?=

NAME ?=
ARGS ?=

SUBFOLDERS = examples hal plugins tools

.PHONY: build-all clean run format-all example help ${SUBFOLDERS}

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
	./setGazeboVariables.sh ${GAZEBO} ${GZ_ARGS} ${WORLD}

format-all:
	find ./ ! \( -path "./build/*" \) -a \( -type f -iname *.hpp -o -type f -iname *.cpp \) | xargs clang-format --style=file -i

example: build
ifndef NAME
	$(error NAME is not set. Call 'make help' for more info)
endif
	cd build && make -j8 ${NAME}
	@test -f ./build/examples/${NAME}/${NAME} || \
		( echo "\nERROR: No example '${NAME}' found\nCall 'make help' for more info\n" ; exit 1 )
	./build/examples/${NAME}/${NAME} ${ARGS}

help:
	@echo "Available targets:"
	@echo "\n\t[build-all]"
	@echo "\t\tBuilds the project."
	@echo "\n\t(${SUBFOLDERS})"
	@echo "\t\tBuilds the given folder in project."
	@echo "\t\t(Target 'worlds' creates all worlds in the project.)"
	@echo "\n\tclean"
	@echo "\t\tRemoves the build directory."
	@echo "\n\trun [GAZEBO=<gazebo_program>] [GZ_ARGS=<gazebo_args>] [WORLD=<world_file>]"
	@echo "\t\tRuns gazebo, gazebo_args default to '--verbose'."
	@echo "\n\tformat-all"
	@echo "\t\tFormats all '.hpp' and '.cpp' files."
	@echo "\n\texample NAME=<example_name> [ARGS=<example_args>]"
	@echo "\t\tRuns the RoFISim client for the example."
	@echo "\n\thelp"
	@echo "\t\tPrints this help."
