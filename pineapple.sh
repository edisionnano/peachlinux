#!/usr/bin/env bash

mkdir -p ~/.config/pineapple && cd ~/.config/pineapple
base64 -d <<<"ICAgICAgICAgICAvJCQgICAgICAgICAgIC8kJCQkJCQkJCAgLyQkJCQkJCAgICAgICAgICAgICAg
ICAgICAgICAvJCQgICAgICAgICAgCiAgICAgICAgICB8X18vICAgICAgICAgIHwgJCRfX19fXy8g
LyQkX18gICQkICAgICAgICAgICAgICAgICAgICB8ICQkICAgICAgICAgIAogIC8kJCQkJCQgIC8k
JCAvJCQkJCQkJCB8ICQkICAgICAgfCAkJCAgXCAkJCAgLyQkJCQkJCAgIC8kJCQkJCQgfCAkJCAg
LyQkJCQkJCAKIC8kJF9fICAkJHwgJCR8ICQkX18gICQkfCAkJCQkJCAgIHwgJCQkJCQkJCQgLyQk
X18gICQkIC8kJF9fICAkJHwgJCQgLyQkX18gICQkCnwgJCQgIFwgJCR8ICQkfCAkJCAgXCAkJHwg
JCRfXy8gICB8ICQkX18gICQkfCAkJCAgXCAkJHwgJCQgIFwgJCR8ICQkfCAkJCQkJCQkJAp8ICQk
ICB8ICQkfCAkJHwgJCQgIHwgJCR8ICQkICAgICAgfCAkJCAgfCAkJHwgJCQgIHwgJCR8ICQkICB8
ICQkfCAkJHwgJCRfX19fXy8KfCAkJCQkJCQkL3wgJCR8ICQkICB8ICQkfCAkJCQkJCQkJHwgJCQg
IHwgJCR8ICQkJCQkJCQvfCAkJCQkJCQkL3wgJCR8ICAkJCQkJCQkCnwgJCRfX19fLyB8X18vfF9f
LyAgfF9fL3xfX19fX19fXy98X18vICB8X18vfCAkJF9fX18vIHwgJCRfX19fLyB8X18vIFxfX19f
X19fLwp8ICQkICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIHwgJCQgICAg
ICB8ICQkICAgICAgICAgICAgICAgICAgICAKfCAkJCAgICAgICAgICAgICAgICAgICAgICAgICAg
ICAgICAgICAgICAgICB8ICQkICAgICAgfCAkJCAgICAgICAgICAgICAgICAgICAgCnxfXy8gICAg
ICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgfF9fLyAgICAgIHxfXy8gICAgICAg
ICAgICAgICAgIA=="
printf "\n"
printf "on pizza\n"
printf "Choose linux distro family:\n"
printf '\e[1;36m%-6s\e[m' "[1]Arch"
printf "\n"
printf '\e[1;31m%-6s\e[m' "[2]Debian"
printf "\n"
printf '\e[1;34m%-6s\e[m' "[3]Fedora"
printf "\n"
printf '\e[1;35m%-6s\e[m' "[4]Gentoo"
printf "\n"
printf "Or anything else to skip installing the dependencies\n"
printf "[1-4]:"
read distro
if [ "$distro" = "1" ];
then
sudo pacman -S --needed git base-devel ninja cmake sdl2 qt5 python2 python-pip boost catch2 fmt libzip lz4 mbedtls nlohmann-json openssl opus zlib zstd && yay -S conan
elif  [ "$distro" = "2" ];
then
sudo apt-get install git build-essential ninja-build cmake libsdl2-dev qtbase5-dev libqt5opengl5-dev qtwebengine5-dev qtbase5-private-dev python python3-pip libboost-dev libboost-context-dev libzip-dev liblz4-dev libmbedtls-dev libssl-dev libopus-dev zlib1g-dev libzstd-dev
wget https://dl.bintray.com/conan/installers/conan-ubuntu-64_1_27_1.deb
sudo dpkg -i conan*
rm conan*
elif [ "$distro" = "3" ];
then
sudo dnf install git gcc ninja-build cmake SDL2-devel qt5-qtbase-devel python2 python-pip boost-devel fmt-devel libzip-devel libzstd-devel lz4-devel mbedtls-devel openssl-devel opus-devel zlib-devel && pip install --user conan
elif [ "$distro" = "4" ];
then
emerge dev-vcs/git =sys-devel/gcc-7.1.0 dev-util/ninja dev-util/cmake media-libs/libsdl2 dev-qt/qtcore dev-qt/qtopengl && pip install --user conan
fi
curl -s https://pineappleea.github.io/ | sed -e '0,/^			<!--link-goes-here-->$/d' -e '/div/q;p'| head -n -2 > version.txt
printf "Latest version is "
head -n 1 version.txt | grep -o 'EA .*' | tr -d '</a><br>'
printf "Type y to download it, n to download an older version, r to uninstall or anything else to exit:"
read option
if [ "$option" = "y" ]
then
curl -s $(head -n 1 version.txt | grep -o 'https.*7z') > version.txt
elif [ "$option" = "n" ]
then
printf "Available versions:\n"
uniq version.txt | grep -o 'EA .*' | tr -d '</a><br>'
printf "Choose version number:"
read version
curl -s $(grep "YuzuEA-$version" version.txt | grep -o 'https.*7z') > version.txt
elif [ "$option" = "r" ]
printf "\nUninstalling...\n"
sudo rm /usr/local/bin/yuzu
sudo rm /usr/share/icons/hicolor/scalable/yuzu.svg
sudo rm /usr/share/pixmaps/yuzu.svg
sudo rm /usr/share/applications/yuzu.desktop
sudo update-desktop-database
printf "Uninstalled successfully\n"
exit
else
printf "Exiting...\n"
exit
fi
wget $(cat version.txt | grep -o 'https://cdn-.*.7z')
mkdir -p executable
rm -rf executable
mkdir executable
7z x Yuzu*
rm Yuzu*
rm version.txt
cd yuzu-windows-msvc-early-access
tar -xf yuzu-windows-msvc-source-* 
rm yuzu-windows-msvc-source-*.tar.xz
cd $(ls -d yuzu-windows-msvc-source-*)
find -type f -exec sed -i 's/\r$//' {} ';'
mkdir build && cd build
cmake .. -GNinja
ninja
mv bin/yuzu ~/.config/pineapple/executable/yuzu
cd ~/.config/pineapple
rm -rf yuzu-windows-msvc-early-access
rm YuzuEA*
rm wget-log
printf '\e[1;32m%-6s\e[m' "Compilation completed, type your password bellow to install it."
printf "\n"
sudo mv ~/.config/pineapple/executable/yuzu /usr/local/bin/yuzu
cd /usr/share/pixmaps
sudo sh -c "curl -s https://raw.githubusercontent.com/edisionnano/peachlinux/master/yuzu.svg > yuzu.svg"
sudo cp /usr/share/pixmaps/yuzu.svg /usr/share/icons/hicolor/scalable/yuzu.svg
sudo sh -c "curl -s https://pastebin.com/raw/pCLPtz0A > /usr/share/applications/yuzu.desktop"
sudo update-desktop-database
printf '\e[1;32m%-6s\e[m' "Installation completed."
printf "\n"
