# Initialise repo

    git submodule update --init --recursive


# Dependancies
CMake
libusb

# Building linux (or mac)

    sudo apt install git
    sudo apt install cmake

    // libaries used
    sudo apt-get install libusb-1.0-0-dev
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..


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
    cmake -DCMAKE_BUILD_TYPE=Release -DBELA=on ..


# Building on Windows
Not supported at this time
