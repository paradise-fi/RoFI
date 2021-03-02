# .SECONDARY:
# export PATH := ../../util:$(PATH)

# BOARDS = $(shell find . -name *.kicad_pcb ! -name _autosave*)

# .PHONY: ele_manu clean

# ele_manu: $(BOARDS:.kicad_pcb=-gerber.zip)

# clean:
# 	echo "Clean run"
# 	rm -rf $(BOARDS:.kicad_pcb=-gerber)
# 	rm -rf $(BOARDS:.kicad_pcb=-gerber.zip)

# %-gerber: %.kicad_pcb
# 	kicadExportGerber.py $< $@

# %-gerber.zip: %-gerber
# 	zip -j $@ `find $<`


all: control_board debug_interface pogo_pad skirt_connector rofi_shoe

build:
	mkdir -p build

# Control board
control_board: build/control_board build/control_board_panel

build/control_board: control_board/control_board.kicad_pcb build
	kikit fab jlcpcb $< $@

build/control_board_panel.kicad_pcb: control_board/control_board.kicad_pcb build
	kikit panelize grid \
		--space 1 --gridsize 2 2 --rotation 45 \
		--tabwidth 8 --tabheight 8 --mousebites 0.6 0.8 0.25 \
		$< $@

build/control_board_panel: build/control_board_panel.kicad_pcb build
	kikit fab jlcpcb $< $@

# Debug interface
debug_interface: build/debug_interface

build/debug_interface: debug_interface/debug_interface.kicad_pcb build
	kikit fab jlcpcb $< $@

# Pogo pad
pogo_pad: build/pogo_pad build/pogo_pad_panel

build/pogo_pad: pogo_pad/pogo_pad.kicad_pcb build
	kikit fab jlcpcb $< $@

build/pogo_pad_panel.kicad_pcb: pogo_pad/pogo_pad.kicad_pcb build
	kikit panelize tightgrid \
		--space 6 --slotwidth 2 --gridsize 3 3 --panelsize 100 80 \
		--tabwidth 16 --tabheight 8 --vcuts \
		--radius 1 \
		$< $@

build/pogo_pad_panel: build/pogo_pad_panel.kicad_pcb build
	kikit fab jlcpcb $< $@

# Skirt connector
skirt_connector: build/skirt_connector build/skirt_connector_panel

build/skirt_connector: skirt_connector/skirt_connector.kicad_pcb build
	kikit fab jlcpcb $< $@

build/skirt_connector_panel.kicad_pcb: skirt_connector/skirt_connector.kicad_pcb build
	kikit panelize tightgrid \
		--space 6 --slotwidth 2 --gridsize 3 3 --panelsize 100 100 \
		--tabwidth 5 --tabheight 5 --vcuts --vcutcurves \
		--radius 0.5 --tolerance 30 \
		$< $@

build/skirt_connector_panel: build/skirt_connector_panel.kicad_pcb build
	kikit fab jlcpcb $< $@

# RoFI shoe
rofi_shoe: build/rofi_shoe_left build/rofi_shoe_right build/rofi_shoe_center

build/rofi_shoe_left: build/rofi_shoe_left.kicad_pcb
	kikit fab jlcpcb $< $@

build/rofi_shoe_right: build/rofi_shoe_right.kicad_pcb
	kikit fab jlcpcb $< $@

build/rofi_shoe_center: build/rofi_shoe_center.kicad_pcb
	kikit fab jlcpcb $< $@

build/rofi_shoe_left.kicad_pcb: rofi_shoe/rofi_shoe.kicad_pcb build
	kikit panelize extractboard -s 48 68 60 65 $< $@

build/rofi_shoe_right.kicad_pcb: rofi_shoe/rofi_shoe.kicad_pcb build
	kikit panelize extractboard -s 184 68 60 65 $< $@

build/rofi_shoe_center.kicad_pcb: rofi_shoe/rofi_shoe.kicad_pcb build
	kikit panelize extractboard -s 117 68 60 65 $< $@