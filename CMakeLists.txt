cmake_minimum_required(VERSION 3.15)

project( Projet_Vulkan VERSION 1.0
                  DESCRIPTION "Moteur Vulkan"
                  LANGUAGES CXX)

#Components
add_subdirectory( VulkanLayer )
add_subdirectory( Renderer )
add_subdirectory( Utils )
add_subdirectory( WindowHandler )

#exec
add_subdirectory( MoteurVulkan )