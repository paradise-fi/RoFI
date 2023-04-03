cmake_minimum_required(VERSION 3.12)

set_semicolon_safe(SINGLE_PANEL_OPTS
    -p :jlcTooling
    --framing "railstb\; width: 5mm\; space: 3mm"
    --tabs "fixed\; width: 5mm\; hcount: 1\; vcount: 1\;"
    --cuts "mousebites\; drill: 0.3mm\; spacing: 0.6mm\; offset: 0.15mm\; prolong: 0.8mm"
    --tooling "3hole\; hoffset: 2.5mm\; voffset: 2.5mm"
    --post "millradius: 0.5mm"
    )

add_pcb(roficom_controlBoard
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/control_board/control_board.kicad_pcb
    PANELIZE --source "annotation\; ref: BOARD1\; tolerance: 6mm"
             ${SINGLE_PANEL_OPTS}
             --layout "grid\; rows: 2\; cols: 2\; rotation: 45deg\; space: 2mm"
    FAB jlcpcb --assembly --no-drc
    SCHEMATICS ${CMAKE_CURRENT_SOURCE_DIR}/control_board/control_board.kicad_sch
)

add_pcb(roficom_controlBoard3DModel
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/control_board/control_board.kicad_pcb
    SEPARATE --source "annotation\; ref: BOARD1\; tolerance: 6mm"
    STEPMODEL
)

add_pcb(roficom_skirt
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/control_board/control_board.kicad_pcb
    PANELIZE --source "annotation\; ref: BOARD2\; tolerance: 6mm\; stack: 4layer"
             --layout "grid\; rows: 3\; cols: 4\; space: 2mm"
             -p :jlcTooling
             --framing "railstb\; width: 5mm\; space: 3mm"
             --tabs "fixed\; width: 3mm\; hcount: 1\; vcount: 1\;"
             --cuts "mousebites\; drill: 0.3mm\; spacing: 0.6mm\; offset: 0.15mm\; prolong: 0.8mm"
             --tooling "3hole\; hoffset: 2.5mm\; voffset: 2.5mm"
             --post "millradius: 1mm"
    FAB jlcpcb --assembly --no-drc
    SCHEMATICS ${CMAKE_CURRENT_SOURCE_DIR}/control_board/control_board.kicad_sch
)

add_pcb(roficom_roficom_skirt3DModel
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/control_board/control_board.kicad_pcb
    SEPARATE --source "annotation\; ref: BOARD2\; tolerance: 6mm"
    STEPMODEL
)

add_pcb(roficom_interconnector
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/control_board/control_board.kicad_pcb
    PANELIZE --source "annotation\; ref: BOARD3\; tolerance: 6mm\; stack: 2layer"
             --layout "grid\; rows: 2\; cols: 4\; space: 2mm"
             -p :jlcTooling
             --framing "railstb\; width: 5mm\; space: 3mm"
             --tabs "fixed\; width: 3mm\; hcount: 1\; vcount: 1\;"
             --cuts "mousebites\; drill: 0.3mm\; spacing: 0.6mm\; offset: 0.15mm\; prolong: 0.8mm"
             --tooling "3hole\; hoffset: 2.5mm\; voffset: 2.5mm"
             --post "millradius: 1mm"
    FAB jlcpcb --assembly --no-drc
    SCHEMATICS ${CMAKE_CURRENT_SOURCE_DIR}/control_board/control_board.kicad_sch
)

add_pcb(roficom_cable
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/control_board/control_board.kicad_pcb
    SEPARATE --source "annotation\; ref: BOARD4\; tolerance: 8mm\; stack: 2layer"
    FAB jlcpcb --no-drc
    SCHEMATICS ${CMAKE_CURRENT_SOURCE_DIR}/control_board/control_board.kicad_sch
)


