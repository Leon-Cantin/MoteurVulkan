cmake_minimum_required(VERSION 3.15)

project( Projet_Vulkan VERSION 1.0
                  DESCRIPTION "Moteur Vulkan"
                  LANGUAGES CXX)
				  
add_definitions( -DUNICODE -D_UNICODE )
set( CMAKE_CXX_STANDARD 20 )

#Components
add_subdirectory( Engine )
add_subdirectory( VulkanLayer )
add_subdirectory( Renderer )
add_subdirectory( Utils )
add_subdirectory( WindowHandler )

#Exec
add_subdirectory( Classic_2D )

set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Classic_2D )

#Packaging
set_target_properties( Classic_2D PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Classic_2D_output/$<0:> )