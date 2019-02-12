project(splite)

include_directories(
        "${PROJECT_SOURCE_DIR}/../../../external/libusb"
#        "${PROJECT_SOURCE_DIR}/../../../external/portaudio"
#        "${PROJECT_SOURCE_DIR}/../../../external/cJSON"
#        "${PROJECT_SOURCE_DIR}/../../../external/oscpack"
)

set(SPL_H
        splib/SoundplaneDriver.h
        splib/SoundplaneModelA.h
        splib/ThreadUtility.h
        splib/Unpacker.h
        splib/SensorFrame.h
        splib/TouchTracker.h
        splib/Touch.h
        SPLiteDevice.h 
        )

set(SPL_SRC
        splib/SoundplaneDriver.cpp
        splib/SoundplaneModelA.cpp
        splib/ThreadUtility.cpp
        splib/SensorFrame.cpp
        splib/TouchTracker.cpp
	SPLiteDevice.cpp
	)

if (APPLE)
    list(APPEND SPL_SRC
            splib/MacSoundplaneDriver.cpp
            splib/MacSoundplaneDriver.h
            )
elseif (UNIX)
    list(APPEND SPL_SRC
            splib/LibusbSoundplaneDriver.cpp
            splib/LibusbSoundplaneDriver.h
            )
endif (APPLE)

add_library(splite SHARED ${SPL_SRC} ${SPL_H})

#target_link_libraries(splite oscpack portaudio cjson)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(splite atomic)
endif ()

if (APPLE)
    target_link_libraries(splite "-framework Cocoa -framework IOKit")
elseif (UNIX)
    target_link_libraries(splite "libusb" "dl" "pthread")
endif (APPLE)

target_include_directories(splite PUBLIC
        .
        splib
        )

add_subdirectory(tests)


