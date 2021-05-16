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
add_subdirectory( PBR_3D_Renderer )
add_subdirectory( Retro_game )

set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Retro_game )

#Packaging
set_target_properties( Classic_2D PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Classic_2D_output/$<0:> )
set_target_properties( PBR_3D_Renderer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/output/$<0:> )
set_target_properties( Retro_game PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Retro_game_output/$<0:> )