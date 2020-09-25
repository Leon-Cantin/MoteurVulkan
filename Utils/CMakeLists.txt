set( TARGET_NAME Utils )
				  
file( GLOB SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} source/* )
file( GLOB INCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/* )

add_library( ${TARGET_NAME} STATIC ${SRCS} ${INCS} )

source_group( " " REGULAR_EXPRESSION .* )
source_group( asset_library REGULAR_EXPRESSION .*/asset_library.* )
source_group( console_commands_handler REGULAR_EXPRESSION .*/console_command.* )
source_group( containers REGULAR_EXPRESSION .*/bitfield.* )
source_group( memory REGULAR_EXPRESSION .*/memory.* )
source_group( window_handler_vk REGULAR_EXPRESSION .*/window_handler_vk.* )

target_include_directories( ${TARGET_NAME} PUBLIC include )

target_link_libraries( ${TARGET_NAME} PUBLIC Vulkan_Layer )
target_link_libraries( ${TARGET_NAME} PUBLIC WindowHandler )
target_link_libraries( ${TARGET_NAME} PUBLIC Renderer )