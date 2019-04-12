.SECONDARY:
export PATH := ../../util:$(PATH)

BOARDS = $(shell find . -name *.kicad_pcb ! -name _autosave*)

.PHONY: ele_manu clean

ele_manu: $(BOARDS:.kicad_pcb=-gerber.zip)

clean:
	echo "Clean run"
	rm -rf $(BOARDS:.kicad_pcb=-gerber)
	rm -rf $(BOARDS:.kicad_pcb=-gerber.zip)

%-gerber: %.kicad_pcb
	export_gerber.py $< $@

%-gerber.zip: %-gerber
	zip -j $@ `find $<`
