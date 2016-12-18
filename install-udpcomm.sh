
#!/bin/bash

iconName="doomlauncher.png"
skeletonDesktopFileName=".doomlauncher.desktop.skel"
desktopFileName="doomlauncher.desktop"
programName="udpcomm"

absolutePath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
fileName="${absolutePath##*/}"
filePath="$(dirname "$absolutePath")/"
sourceDir="$filePath/src/"
globalBinDir="/usr/bin"
#echo "$absolutePath"
#echo "$fileName"
#echo "$filePath"

buildDir="build"

function bailout() {
    rm -rf "$buildDir"
}

function generateDesktopFile() {
    cp "$utilityDir/$skeletonDesktopFileName" "$buildDir/$desktopFileName" || { echo "Failed to generate desktop file, exiting"; exit 1; }
    cp "$iconPath" "$buildDir/"
    echo "Exec=$buildDir$programName" >> "$buildDir/$desktopFileName"
    echo "Icon=$buildDir$iconName" >> "$buildDir/$desktopFileName"
}

trap bailout INT QUIT TERM
if [[ $# -gt 0 ]]; then
    if [[ "$1" == "--uninstall" ]]; then
        echo "Success"
        exit 0
    fi
    buildDir="$1"
    if ! [[ -d "$buildDir" ]]; then
        mkdir "$buildDir" || { echo "Unable to create build directory \"$buildDir\", exiting"; exit 1; }
    fi
else
    buildDir="$filePath/$buildDir"
    if ! [[ -d "$buildDir" ]]; then
        mkdir "$buildDir" || { echo "Unable to make build directory \"$buildDir\", exiting"; exit 1; }
    fi
fi

cd "$buildDir" || { echo "Unable to enter build directory \"$buildDir\""; exit 1; }
cmake "$filePath" || { echo "cmake failed, bailing out"; exit 1; }
make || { echo "make failed, bailing out"; exit 1; }


sudo ln -s -f "$buildDir/$programName" "$globalBinDir"

echo
echo "*********************************************"
echo "**UDP Communication Installed Successfully!**"
echo "*********************************************"
echo 
exit 0
