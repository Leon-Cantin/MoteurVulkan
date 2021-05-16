set( TARGET_NAME Engine )
				  
file( GLOB SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} sources/* )
file( GLOB INCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} includes/* )

add_library( ${TARGET_NAME} STATIC ${SRCS} ${INCS} ${SHADERS} )

source_group( " " REGULAR_EXPRESSION .* )

target_include_directories( ${TARGET_NAME} PUBLIC includes )

target_link_libraries( ${TARGET_NAME} PUBLIC Renderer )
target_link_libraries( ${TARGET_NAME} PUBLIC Utils )

target_include_directories( ${TARGET_NAME} PUBLIC ../Bullet3/src )
target_link_libraries( ${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Bullet3/build/lib/Debug/BulletCollision_Debug.lib )
target_link_libraries( ${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Bullet3/build/lib/Debug/LinearMath_Debug.lib )