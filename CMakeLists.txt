cmake_minimum_required(VERSION 3.16...3.27)
project(GEANT4_TEMPLATE)

option(WITH_GEANT4_UIVIS "Build example with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
    find_package(Geant4 REQUIRED ui_all vis_all)
else()
    find_package(Geant4 REQUIRED)
endif()

include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(geant4_template main.cc ${sources} ${headers})
target_link_libraries(geant4_template ${Geant4_LIBRARIES})

set(GEANT4_TEMPLATE_SCRIPTS
    run.mac
    init_vis.mac
    vis.mac
)

foreach(_script ${GEANT4_TEMPLATE_SCRIPTS})
    configure_file(
        ${PROJECT_SOURCE_DIR}/${_script}
        ${PROJECT_BINARY_DIR}/${_script}
        COPYONLY
    )
endforeach()
