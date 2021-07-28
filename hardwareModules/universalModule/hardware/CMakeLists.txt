cmake_minimum_required(VERSION 3.12)

set_semicolon_safe(SINGLE_PANEL_OPTS
    -p :jlcTooling
    --framing "railstb\; width: 5mm\; space: 3mm"
    --tabs "fixed\; width: 3mm\; vcount: 2"
    --cuts "mousebites\; drill: 0.3mm\; spacing: 0.6mm\; offset: 0.15mm\; prolong: 0.8mm"
    --tooling "3hole\; hoffset: 2.5mm\; voffset: 2.5mm"
    --post "millradius: 1mm"
    )

add_pcb(um_controlBoard
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    PANELIZE --source "annotation\; ref: BODY3\; tolerance: 6mm" ${SINGLE_PANEL_OPTS}
    FAB jlcpcb --assembly
    SCHEMATICS ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.sch
)

add_pcb(um_controlBoard3DModel
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: BODY3\; tolerance: 6mm"
    STEPMODEL
)

add_pcb(um_batteryBoard
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    PANELIZE --source "annotation\; ref: BODY1\; tolerance: 6mm" ${SINGLE_PANEL_OPTS}
    FAB jlcpcb --assembly
    SCHEMATICS ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.sch
)

add_pcb(um_batteryBoard3DModel
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: BODY1\; tolerance: 6mm"
    STEPMODEL
)

add_pcb(um_displayBoard
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    PANELIZE --source "annotation\; ref: BODY2\; tolerance: 6mm\;stack: 2layer" ${SINGLE_PANEL_OPTS}
    FAB jlcpcb --assembly
    SCHEMATICS ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.sch
)

add_pcb(um_displayBoard3DModel
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: BODY2\; tolerance: 6mm\;stack: 2layer"
    STEPMODEL
)

add_pcb(um_slipRing
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: BODY5\; tolerance: 6mm\;stack: 2layer"
    FAB jlcpcb
)

add_pcb(um_slipRing3DModel
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: BODY5\; tolerance: 6mm\;stack: 2layer"
    STEPMODEL
)

add_pcb(um_servoBoard
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: BODY4\; tolerance: 3mm\;stack: 2layer"
    FAB jlcpcb
)

add_pcb(um_servoBoard3DModel
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: BODY4\; tolerance: 3mm\;stack: 2layer"
    STEPMODEL
)

add_pcb(um_debugBoard
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: DEBUG1\; tolerance: 3mm\;stack: 2layer"
    FAB jlcpcb
)

add_pcb(um_shoeLeft
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: SHOE1\; tolerance: 3mm\;stack: 2layer"
    FAB jlcpcb
)

add_pcb(um_shoeLeft3DModel
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: SHOE1\; tolerance: 3mm\;stack: 2layer"
    STEPMODEL
)

add_pcb(um_shoeMid
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: SHOE2\; tolerance: 8mm\;stack: 2layer"
    FAB jlcpcb
)

add_pcb(um_shoeMid3DModel
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: SHOE2\; tolerance: 3mm\;stack: 2layer"
    STEPMODEL
)

add_pcb(um_shoeRight
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: SHOE3\; tolerance: 3mm\;stack: 2layer"
    FAB jlcpcb
)

add_pcb(um_shoeRight3DModel
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: SHOE3\; tolerance: 3mm\;stack: 2layer"
    STEPMODEL
)

add_pcb(um_bodyCable
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: CABLE2\; tolerance: 3mm\;stack: 2layer"
    FAB jlcpcb
)

add_pcb(um_shoeCable
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/controlBoard/controlBoard.kicad_pcb
    SEPARATE --source "annotation\; ref: CABLE1\; tolerance: 3mm\;stack: 2layer"
    FAB jlcpcb
)
