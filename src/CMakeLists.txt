
set (verse_particles_src
        client.c
		lu_table.c
		client_particle_sender.c
		client_particle_receiver.c
		particle_data.c
		display_glut.c
		math_lib.c
		particle_scene_node.c
		particle_sender_node.c
		particle_node.c
		timer.c
		sender.c)

include_directories (../include)
include_directories (${VERSE_INCLUDE_DIR})
include_directories (${OPENSSL_INCLUDE_DIR})
include_directories (${OPENGL_INCLUDE_DIR})
include_directories (${GLUT_INCLUDE_DIR})

# Make build flags compiler specific
if (CMAKE_COMPILER_IS_GNUCC)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set (CMAKE_C_FLAGS "-D_REETRANT -ggdb -fPIC -Wall -Wextra -pedantic -O0")
	elseif( CMAKE_BUILD_TYPE STREQUAL "Release" )
		set (CMAKE_C_FLAGS "-D_REETRANT -DNDEBUG -fPIC -Wall -Wextra -pedantic -O3")
	endif ()
endif (CMAKE_COMPILER_IS_GNUCC)

add_executable (verse_particle ${verse_particles_src})
target_link_libraries (verse_particle
		${VERSE_LIBRARIES}
		${CMAKE_THREAD_LIBS_INIT}
		${OPENSSL_LIBRARIES}
		${OPENGL_LIBRARIES}
		${GLUT_LIBRARIES})