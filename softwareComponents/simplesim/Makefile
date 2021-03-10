default: build-all

.PHONY: build-all clean run format-all help

build:
	mkdir -p build
	cd build && cmake ..

build-all: build
	cd build && make -j8
	@echo "\nBuild completed\n\n"

clean:
	rm -rf build

format-all:
	find ./ ! \( -path "./build/*" \) -a \( -type f -iname "*.hpp" -o -type f -iname "*.cpp" \) | xargs clang-format --style=file -i

help:
	@echo "Available targets:"
	@echo "\n\t[build-all]"
	@echo "\t\tBuilds the project."
	@echo "\n\tclean"
	@echo "\t\tRemoves the build directory."
	@echo "\n\tformat-all"
	@echo "\t\tFormats all '.hpp' and '.cpp' files."
	@echo "\n\thelp"
	@echo "\t\tPrints this help."
