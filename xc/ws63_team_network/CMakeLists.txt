#===============================================================================
# WS63 SLE team network sample.
# Copy this directory under BearPi SDK application/samples/products/sle_team_network.
#===============================================================================

if(NOT DEFINED SLE_MESH_ROOT)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../include/sle_team_packet.h)
        set(SLE_MESH_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
    elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party/sle_mesh/include/sle_team_packet.h)
        set(SLE_MESH_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party/sle_mesh)
    else()
        message(FATAL_ERROR "SLE_MESH_ROOT is not set and sle_mesh protocol sources were not found")
    endif()
endif()

set(SLE_TEAM_SDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(SLE_TEAM_UART_SERVER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/sle_uart_server)
set(SLE_TEAM_UART_CLIENT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/sle_uart_client)

set(SOURCES_LIST
    ${SLE_MESH_ROOT}/src/sle_team_packet.c
    ${SLE_MESH_ROOT}/src/sle_team_node.c
    ${SLE_MESH_ROOT}/src/sle_team_cli.c
    ${SLE_MESH_ROOT}/src/sle_team_web_api.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sle_uart_server/sle_uart_server_adv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sle_uart_server/sle_uart_server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sle_uart_client/sle_uart_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ws63_st7789_display.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ws63_team_network_app.c
)

include_directories(${SLE_TEAM_UART_SERVER_HEADERS} ${SLE_TEAM_UART_CLIENT_HEADERS})

set(SOURCES "${SOURCES}" ${SOURCES_LIST} PARENT_SCOPE)
set(PUBLIC_HEADER
    "${PUBLIC_HEADER}"
    ${SLE_MESH_ROOT}/include
    ${SLE_TEAM_UART_SERVER_HEADERS}
    ${SLE_TEAM_UART_CLIENT_HEADERS}
    PARENT_SCOPE
)
