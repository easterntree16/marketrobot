include_directories(./component)
set(MRLibSrc
    ./component/config.h
    ./component/config.cpp
    ./component/logger.h
    ./component/mylogger.h
    ./component/mylogger.cpp
    )

add_library(marketrobot ${MRLibSrc})
set_target_properties(marketrobot PROPERTIES COMPILE_FLAGS "-fPIC")

IF (WIN32)
	MESSAGE(STATUS "###########################OS is windows###########################")
	target_link_libraries(marketrobot  libyaml-cppmdd)

ELSEIF (UNIX)	
	MESSAGE(STATUS "###########################OS is UNIX-like OS's.###########################")
	target_link_libraries(marketrobot  yaml-cpp)
ENDIF ()


