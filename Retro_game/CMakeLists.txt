set( TARGET_NAME Retro_game )
				  
file( GLOB SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} sources/* )
file( GLOB SHADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} shaders/* )

add_executable( ${TARGET_NAME} ${SRCS} ${SHADERS} )

source_group( " " REGULAR_EXPRESSION .* )
source_group( shaders FILES ${SHADERS} )

target_include_directories( ${TARGET_NAME} PRIVATE sources )

set_target_properties( ${TARGET_NAME} PROPERTIES OUTPUT_NAME ${TARGET_NAME}_$<CONFIG> )

target_link_libraries( ${TARGET_NAME} PRIVATE Engine )
target_link_libraries( ${TARGET_NAME} PRIVATE Renderer )
target_link_libraries( ${TARGET_NAME} PRIVATE WindowHandler )
target_link_libraries( ${TARGET_NAME} PRIVATE Utils )