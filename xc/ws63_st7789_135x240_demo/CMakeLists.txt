#===============================================================================
# WS63 ST7789 135x240 TFT animation demo.
# Copy this directory under BearPi SDK application/samples/products/.
#===============================================================================

if(DEFINED CONFIG_SAMPLE_SUPPORT_WS63_ST7789_135X240)
    set(SOURCES
        "${SOURCES}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ws63_st7789_demo.c"
        PARENT_SCOPE
    )
endif()
