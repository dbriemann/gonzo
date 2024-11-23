add_library(libgonzo process.cpp)
add_library(gonzo::libgonzo ALIAS libgonzo)

set_target_properties(
	libgonzo
	PROPERTIES OUTPUT_NAME gonzo
)

target_compile_features(libgonzo PUBLIC cxx_std_17)

target_include_directories(libgonzo
	PUBLIC 
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_SOURCE_DIR}/src/include
)

include(GNUInstallDirs)
install(TARGETS libgonzo
	EXPORT gonzo-targets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
	DIRECTORY ${PROJECT_SOURCE_DIR}/include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
	EXPORT gonzo-targets
	FILE gonzo-config.cmake
	NAMESPACE gonzo::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gonzo
)
