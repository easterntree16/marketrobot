include_directories(./component)
set(MRLibSrc
    ./component/config.h
    ./component/config.cpp
    ./component/logger.h
    )

add_library(marketrobot ${MRLibSrc})
set_target_properties(marketrobot PROPERTIES COMPILE_FLAGS "-fPIC")
target_link_libraries(marketrobot  
E:/Dev/MarketRobot/3rd_party/yaml-cpp/build/Debug/libyaml-cppmdd.lib)

