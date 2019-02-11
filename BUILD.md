# Initialise repo

to grab just the soundplane and madronalib submodules (preferred)

    git submodule update --init splite

alternatively, you could use 

    git submodule update --init --recursive

this will however also bring in Juce which we do not use at this time.

# Dependancies
CMake
libusb

# Building linux (or mac)

    sudo apt install git
    sudo apt install cmake

    // libaries used
    sudo apt-get install libusb-1.0-0-dev
    sudo apt-get install libasound2-dev (linux)
    mkdir build
    cd build
    cmake .. 


# Building Mac (with xcode)

    brew install git
    brew install cmake
 
    brew install libusb --universal

    mkdir build
    cd build
    cmake .. -GXcode 

    xcodebuild -project SoundplaneLite.xcodeproj -target ALL_BUILD -configuration MinSizeRel

or

    cmake --build .


# Building within Sublime Text

you can also build within sublime

    mkdir build
    cd build
    cmake -G "Sublime Text 2 - Unix Makefiles" .. 


# Building on Bela
the bela web ui does not support a subdirectories, so the way install is
  
    mkdir -p ~/projects
    cd ~/projects 
    // git clone SoundplaneLite.git here
    apt-get install libusb-1.0-0-dev
    mkdir build
    cd build
    cmake .. 
    cd ~/Bela/projects
    ln ~/projects/SoundplaneLite

this means we now have the SoundplaneLite project in the 'normal' bela projects directory, but it refers to ~/projects/SoundplaneLite, so you should push from ~/Projects/SoundplaneLite


# Building on Windows
Not supported at this time