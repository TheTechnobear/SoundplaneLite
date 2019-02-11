project(splite)

include_directories(
        "${PROJECT_SOURCE_DIR}/../../../external/libusb"
        "${PROJECT_SOURCE_DIR}/../../../external/portaudio"
        "${PROJECT_SOURCE_DIR}/../../../external/cJSON"
        "${PROJECT_SOURCE_DIR}/../../../external/oscpack"
)

set(SPL_H
        madronalib/include/madronalib.h
        madronalib/include/MLDSP.h
        madronalib/include/MLCore.h

        madronalib/source/core/MLPath.h
        madronalib/source/core/MLSymbol.h
        madronalib/source/core/MLTimer.h
        madronalib/source/core/MLText.h

        madronalib/source/DSP/MLDSPUtils.h
        madronalib/source/DSP/MLScalarMath.h

        madronalib/source/matrix/MLSignal.h

        madronalib/source/model/MLModel.h
        madronalib/source/model/MLProperty.h
        madronalib/source/model/MLPropertySet.h


        soundplane/SoundplaneLib/SoundplaneDriver.h
        soundplane/SoundplaneLib/SoundplaneModelA.h
        soundplane/SoundplaneLib/ThreadUtility.h
        soundplane/SoundplaneLib/Unpacker.h
        soundplane/SoundplaneLib/SensorFrame.h

        soundplane/Source/TouchTracker.h

        # madronalib/source/networking/MLOSCListener.h
        # madronalib/source/JuceApp/MLAppState.h
        # soundplane/Source/Zone.h
        # SoundplaneModel.h
        # SoundplaneMECOutput.h

        #deprecated
        # madronalib/source/deprecated/MLParameter.h
        # madronalib/source/deprecated/MLDebug.h
        # madronalib/source/deprecated/MLTextStreamListener.h
        )

set(SPL_SRC
        madronalib/source/core/MLPath.cpp
        madronalib/source/core/MLSymbol.cpp
        madronalib/source/core/MLTimer.cpp
        madronalib/source/core/MLText.cpp

        madronalib/source/DSP/MLScalarMath.h
        madronalib/source/matrix/MLSignal.cpp
        madronalib/source/model/MLModel.cpp
        madronalib/source/model/MLProperty.cpp
        madronalib/source/model/MLPropertySet.cpp


        soundplane/SoundplaneLib/SoundplaneDriver.cpp
        soundplane/SoundplaneLib/SoundplaneModelA.cpp
        soundplane/SoundplaneLib/ThreadUtility.cpp
        soundplane/SoundplaneLib/SensorFrame.cpp

        soundplane/Source/TouchTracker.cpp

        # madronalib/source/networking/MLOSCListener.cpp
        # madronalib/source/JuceApp/MLAppState.cpp
        # soundplane/Source/Zone.cpp
        # SoundplaneModel.cpp
        # SoundplaneMECOutput.cpp

        #deprecated
        # madronalib/source/deprecated/MLParameter.cpp
        # madronalib/source/deprecated/MLDebug.cpp
        )

#TODO look at new fpu handling
if (APPLE)
    list(APPEND SPL_SRC
            soundplane/SoundplaneLib/MacSoundplaneDriver.cpp
            soundplane/SoundplaneLib/MacSoundplaneDriver.h
            #    madronalib/source/deprecated/MLDebugMac.mm
            )

    if (1)
        # NOFPU
        list(APPEND SPL_H
                # source/nofpu/MLVector.h
                )
        list(APPEND SPL_SRC
                # source/nofpu/MLVector.cpp
                # #source/nofpu/MLSignal.cpp
                )
    else ()
        # SSE
        add_definitions(-D ML_USE_SSE)
        list(APPEND SPL_H
                # source/sse/MLVector.h
                # source/sse/MLDSP.h
                )
        list(APPEND SPL_SRC
                # source/sse/MLVector.cpp
                # #source/sse/MLSignal.cpp
                )
    endif ()

elseif (UNIX)
    list(APPEND SPL_SRC
            soundplane/SoundplaneLib/LibusbSoundplaneDriver.cpp
            soundplane/SoundplaneLib/LibusbSoundplaneDriver.h
            )

    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
        # NEON
        add_definitions(-D ML_USE_NEON)
        list(APPEND SPL_H
                # source/neon/MLVector.h
                # source/neon/MLDSP.h
                )
        list(APPEND SPL_SRC
                # source/neon/MLVector.cpp
                # #source/neon/MLSignal.cpp
                )
    else ()
        # SSE
        add_definitions(-D ML_USE_SSE)
        list(APPEND SPL_H
                # source/sse/MLVector.h
                # source/sse/MLDSP.h
                )
        list(APPEND SPL_SRC
                # source/sse/MLVector.cpp
                # #source/sse/MLSignal.cpp
                )
    endif ()
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
        madronalib/external
        # madronalib/external/oscpack/osc
        # madronalib/external/oscpack/ip

        madronalib/source/core
        madronalib/source/DSP
        madronalib/source/matrix
        madronalib/source/model
        madronalib/source/networking

        soundplane/SoundplaneLib
        soundplane/Source
        )

#add_subdirectory(tests)


