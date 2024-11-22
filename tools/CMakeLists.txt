add_executable(gonzo gonzo.cpp)
target_link_libraries(gonzo PRIVATE gonzo::libgonzo PkgConfig::libedit)

include(GNUInstallDirs)
install(
	TARGETS gonzo
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
