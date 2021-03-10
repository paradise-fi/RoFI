idf_component_register(SRCS "main.cpp"
    INCLUDE_DIRS ""
    REQUIRES RoFIDriver)
    set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 14)