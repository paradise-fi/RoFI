idf_component_register(SRCS "main.cpp"
    INCLUDE_DIRS ""
    REQUIRES rofiHal)
    set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)