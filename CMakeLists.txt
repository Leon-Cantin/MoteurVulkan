cmake_minimum_required(VERSION 3.15)

project( Projet_Vulkan VERSION 1.0
                  DESCRIPTION "Moteur Vulkan"
                  LANGUAGES CXX)
				  
add_definitions( -DUNICODE -D_UNICODE )
set( CMAKE_CXX_STANDARD 20 )

#Components
add_subdirectory( VulkanLayer )
add_subdirectory( Renderer )
add_subdirectory( Utils )
add_subdirectory( WindowHandler )

#Exec
add_subdirectory( PBR_3D_Renderer )

set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT PBR_3D_Renderer )

#Packaging
set_target_properties( PBR_3D_Renderer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/output/$<0:> )