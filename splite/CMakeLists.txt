project(splite)

include_directories(
        "${PROJECT_SOURCE_DIR}/../../../external/libusb"
        "${PROJECT_SOURCE_DIR}/../../../external/portaudio"
        "${PROJECT_SOURCE_DIR}/../../../external/cJSON"
        "${PROJECT_SOURCE_DIR}/../../../external/oscpack"
)

set(SPL_H
        soundplane/SoundplaneLib/SoundplaneDriver.h
        soundplane/SoundplaneLib/SoundplaneModelA.h
        soundplane/SoundplaneLib/ThreadUtility.h
        soundplane/SoundplaneLib/Unpacker.h
        soundplane/SoundplaneLib/SensorFrame.h
        soundplane/Source/TouchTracker.h
        soundplane/Source/Touch.h
        )

set(SPL_SRC
        soundplane/SoundplaneLib/SoundplaneDriver.cpp
        soundplane/SoundplaneLib/SoundplaneModelA.cpp
        soundplane/SoundplaneLib/ThreadUtility.cpp
        soundplane/SoundplaneLib/SensorFrame.cpp
        soundplane/Source/TouchTracker.cpp
        )

#TODO look at new fpu handling
if (APPLE)
    list(APPEND SPL_SRC
            soundplane/SoundplaneLib/MacSoundplaneDriver.cpp
            soundplane/SoundplaneLib/MacSoundplaneDriver.h
            )
elseif (UNIX)
    list(APPEND SPL_SRC
            soundplane/SoundplaneLib/LibusbSoundplaneDriver.cpp
            soundplane/SoundplaneLib/LibusbSoundplaneDriver.h
            )
endif (APPLE)

add_library(splite SHARED ${SPL_SRC} ${SPL_H})

target_link_libraries(splite oscpack portaudio cjson)

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
        soundplane/SoundplaneLib
        soundplane/Source
        )

add_subdirectory(tests)


