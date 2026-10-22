find_path(SDL2_INCLUDE_DIR
	NAMES SDL.h
	HINTS
		$ENV{SDL2DIR}
		$ENV{SDL2_ROOT}
		${SDL2_ROOT}
	PATH_SUFFIXES
		include/SDL2
		SDL2
		include)

find_library(SDL2_LIBRARY
	NAMES SDL2 SDL2-static
	HINTS
		$ENV{SDL2DIR}
		$ENV{SDL2_ROOT}
		${SDL2_ROOT}
	PATH_SUFFIXES
		lib
		lib/x86
		lib64
		lib/win32
		lib/win64)

find_library(SDL2MAIN_LIBRARY
	NAMES SDL2main
	HINTS
		$ENV{SDL2DIR}
		$ENV{SDL2_ROOT}
		${SDL2_ROOT}
	PATH_SUFFIXES
		lib
		lib/x86
		lib64
		lib/win32
		lib/win64)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2
	REQUIRED_VARS SDL2_LIBRARY SDL2_INCLUDE_DIR)

if(SDL2_FOUND)
	get_filename_component(SDL2_LIBRARY_DIR "${SDL2_LIBRARY}" DIRECTORY)
	set(SDL2_LIBRARIES ${SDL2_LIBRARY})
	set(SDL2_LIBRARY_DIRS ${SDL2_LIBRARY_DIR})
	set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
endif()

mark_as_advanced(SDL2_INCLUDE_DIR SDL2_LIBRARY SDL2MAIN_LIBRARY)
