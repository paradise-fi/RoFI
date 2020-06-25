default: build-all

GAZEBO ?= gazebo
GZ_ARGS ?= --verbose
WORLD ?=

NAME ?=
ARGS ?=

SUBFOLDERS = demos examples hal plugins tools worlds

.PHONY: build-all clean run tests-roficom format-all demo example world-creator help ${SUBFOLDERS}

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

run: plugins worlds
	./setGazeboVariables.sh ${GAZEBO} ${GZ_ARGS} ${WORLD}

tests-roficom: plugins
	./build/plugins/roficom/tests/tests_roficom

format-all:
	find ./ ! \( -path "./build/*" \) -a \( -type f -iname *.hpp -o -type f -iname *.cpp \) | xargs clang-format --style=file -i

demo example: build
ifndef NAME
	$(error NAME is not set. Call 'make help' for more info)
endif
	cd build && make -j8 ${NAME}
	@test -f ./build/$@s/${NAME}/${NAME} || \
		( echo "\nERROR: No $@ '${NAME}' found\nCall 'make help' for more info\n" ; exit 1 )
	./build/$@s/${NAME}/${NAME} ${ARGS}

world-creator: build
	cd build && make worldCreator -j8
	@echo "\nBuild worldCreator completed\n\n"
	./setGazeboVariables.sh ./build/tools/worldCreator/worldCreator ${ARGS}

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
	@echo "\n\ttests-roficom"
	@echo "\t\tRuns tests for roficom."
	@echo "\n\tformat-all"
	@echo "\t\tFormats all '.hpp' and '.cpp' files."
	@echo "\n\tdemo NAME=<demo_name> [ARGS=<demo_args>]"
	@echo "\t\tRuns the RoFISim client for the demo."
	@echo "\n\texample NAME=<example_name> [ARGS=<example_args>]"
	@echo "\t\tRuns the RoFISim client for the example."
	@echo "\n\tworld-creator ARGS=\"[-i <input_file>] [-o <output_file>] [-w <empty_world_file>]\""
	@echo "\t\tRuns the World creator tool. (Note the quotation marks around ARGS value.)"
	@echo "\t\tIf not set, uses the standard input/output."
	@echo "\n\thelp"
	@echo "\t\tPrints this help."
