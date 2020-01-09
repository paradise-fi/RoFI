.SECONDARY:
export PATH := ../../util:$(PATH)

CONTROL_BOARDS = manu/batteryBoard.kicad_pcb manu/powerBoard.kicad_pcb manu/cpuBoard.kicad_pcb manu/uiBoard.kicad_pcb

.PHONY: ele_manu clean controlBoard slipRing slipRingBrush

define build_gerber
	kicadExportGerber.py $(1) manu/$(notdir $(1:.kicad_pcb=-gerber))
	zip -j manu/$(notdir $(1:.kicad_pcb=-gerber.zip)) `find manu/$(notdir $(1:.kicad_pcb=-gerber))`
endef

define build_stencil
	kicadExportDxf.py $(1)
	openscad -o $(1:.kicad_pcb=-stencilTop.stl) -Doutline=\"$(1:.kicad_pcb=-EdgeCuts.dxf)\" -Dmask=\"$(1:.kicad_pcb=-PasteTop.dxf)\" -Dfront=1 ../../util/pcbStencilGenerator.scad
	admesh --write-binary-stl=$(1:.kicad_pcb=-stencilTop.stl) $(1:.kicad_pcb=-stencilTop.stl)
	openscad -o $(1:.kicad_pcb=-stencilBottom.stl) -Doutline=\"$(1:.kicad_pcb=-EdgeCuts.dxf)\" -Dmask=\"$(1:.kicad_pcb=-PasteBottom.dxf)\" -Dfront=0 ../../util/pcbStencilGenerator.scad
	admesh --write-binary-stl=$(1:.kicad_pcb=-stencilBottom.stl) $(1:.kicad_pcb=-stencilBottom.stl)
endef

ele_manu: controlBoard slipRing slipRingBrush

clean:
	rm -rf manu

manu:
	mkdir -p manu

controlBoard:
	mkdir -p manu/controlBoard
	splitBoard.py controlBoard/controlBoard.kicad_pcb 206 76 70 85 manu/batteryBoard.kicad_pcb
	$(call build_gerber, manu/batteryBoard.kicad_pcb)
	$(call build_stencil, $(abspath manu/batteryBoard.kicad_pcb))
	splitBoard.py controlBoard/controlBoard.kicad_pcb 130 76 70 85 manu/powerBoard.kicad_pcb
	$(call build_gerber, manu/powerBoard.kicad_pcb)
	$(call build_stencil, $(abspath manu/powerBoard.kicad_pcb))
	splitBoard.py controlBoard/controlBoard.kicad_pcb 55 76 70 85 manu/cpuBoard.kicad_pcb
	$(call build_gerber, manu/cpuBoard.kicad_pcb)
	$(call build_stencil, $(abspath manu/cpuBoard.kicad_pcb))
	splitBoard.py controlBoard/controlBoard.kicad_pcb 20 90 35 60 manu/uiBoard.kicad_pcb
	$(call build_gerber, manu/uiBoard.kicad_pcb)
	$(call build_stencil, $(abspath manu/uiBoard.kicad_pcb))

slipRing:
	$(call build_gerber, slipRing/slipRing.kicad_pcb)

slipRingBrush:
	$(call build_gerber, slipRingBrush/slipRingBrush.kicad_pcb)


