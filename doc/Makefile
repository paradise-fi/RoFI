
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
DOC_DIR       = .
BUILDDIR      = build

driver_DEPS = $(realpath $(shell find ../RoFIDriver/ -type f -not -path '*build*'))
roficom_DEPS = $(realpath $(shell find ../RoFICoM/software/control_board/ -type f -not -path '*build*'))
lib_DEPS = $(realpath $(shell find ../RoFILib/ -type f -not -path '*build*'))
sim_DEPS = $(realpath $(shell find ../RoFISim/ -type f -not -path '*build*'))

.PHONY: all clean

all: sphinx

build:
	mkdir build

sphinx: Makefile build/doxygen/driver/xml build/doxygen/roficom/xml build/doxygen/lib/xml build/doxygen/sim/xml
	@$(SPHINXBUILD) -M html "$(DOC_DIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	touch $(BUILDDIR)/html/.nojekyll

clean:
	@$(SPHINXBUILD) -M clean "$(DOC_DIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	rm -rf "$(BUILDDIR)"

build/doxygen/driver/xml: $(driver_DEPS)
	./extractDoxygen.sh driver ../RoFIDriver/ build/doxygen/driver
build/doxygen/roficom/xml: $(roficom_DEPS)
	./extractDoxygen.sh roficom ../RoFICoM/software/control_board/ build/doxygen/roficom
build/doxygen/lib/xml: $(lib_DEPS)
	./extractDoxygen.sh lib ../RoFILib/ build/doxygen/lib
build/doxygen/sim/xml: $(sim_DEPS)
	./extractDoxygen.sh sim ../RoFISim/ build/doxygen/sim
