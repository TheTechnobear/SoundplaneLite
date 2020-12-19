# basic testapp
set(SOUNDPLANETEST_SRC "soundplanetest.cpp")
include_directories ("${PROJECT_SOURCE_DIR}/..")
add_executable(soundplanetest ${SOUNDPLANETEST_SRC})
#target_link_libraries (soundplanetest splite portaudio)
target_link_libraries (soundplanetest splite)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(soundplanetest atomic)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
	target_link_libraries(soundplanetest  "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(soundplanetest pthread libusb)
endif()


# test touchtracker app
set(TOUCHTRACKERTEST_SRC "touchtrackertest.cpp")
include_directories ("${PROJECT_SOURCE_DIR}/..")
add_executable(touchtrackertest ${TOUCHTRACKERTEST_SRC})
target_link_libraries (touchtrackertest splite)
#target_link_libraries (touchtrackertest splite portaudio)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(touchtrackertest atomic)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
	target_link_libraries(touchtrackertest  "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(touchtrackertest pthread libusb)
endif()


# splitedev test app
set(SPLITEDEVTEST_SRC "splitedevtest.cpp")
include_directories ("${PROJECT_SOURCE_DIR}/..")
add_executable(splitedevtest ${SPLITEDEVTEST_SRC})
target_link_libraries (splitedevtest splite )
#target_link_libraries (splitedevtest splite portaudio)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(splitedevtest atomic)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
  target_link_libraries(splitedevtest  "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(splitedevtest pthread libusb)
endif()
