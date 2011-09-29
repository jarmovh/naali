#!/bin/bash

export LANG=C

FINAL_INSTALL_DIR=/opt/realxtend
PACKAGE_NAME=realXtend
DATE=`date '+%y%m%d'`
TIME=`date '+%H%M'`

#PARAMETERS PASSED 
BRANCH=$1
ARCH=$2
REX_DIR=$3
TAG=$4
BUILDNUMBER=$5
VER=$6
LINUX_RELEASE=$7
SERVER=$8
USESTAMP=$9

#IN CASE ERROR HAPPENS, $?-VARIABLE IS != 0
function errorCheck {
    if [ $? -ne 0 ];
    then
        echo $1
        exit $?
    fi
}

#UPDATE SOURCES.LIST FOR REPOSITORY

cat > /etc/apt/sources.list << EOF
deb http://se.archive.ubuntu.com/ubuntu/ release main restricted
deb-src http://se.archive.ubuntu.com/ubuntu/ release main restricted

deb http://se.archive.ubuntu.com/ubuntu/ release-updates main restricted
deb-src http://se.archive.ubuntu.com/ubuntu/ release-updates main restricted

deb http://se.archive.ubuntu.com/ubuntu/ release universe
deb-src http://se.archive.ubuntu.com/ubuntu/ release universe
deb http://se.archive.ubuntu.com/ubuntu/ release-updates universe
deb-src http://se.archive.ubuntu.com/ubuntu/ release-updates universe

deb http://se.archive.ubuntu.com/ubuntu/ release multiverse
deb-src http://se.archive.ubuntu.com/ubuntu/ release multiverse
deb http://se.archive.ubuntu.com/ubuntu/ release-updates multiverse
deb-src http://se.archive.ubuntu.com/ubuntu/ release-updates multiverse

deb http://security.ubuntu.com/ubuntu release-security main restricted
deb-src http://security.ubuntu.com/ubuntu release-security main restricted
deb http://security.ubuntu.com/ubuntu release-security universe
deb-src http://security.ubuntu.com/ubuntu release-security universe
deb http://security.ubuntu.com/ubuntu release-security multiverse
deb-src http://security.ubuntu.com/ubuntu release-security multiverse
EOF

#echo "deb http://ppa.launchpad.net/ogre-team/ogre/ubuntu natty main" > /etc/apt/sources.list.d/ogre-team-ogre-natty.list
sed -i 's/release/'$LINUX_RELEASE'/' /etc/apt/sources.list
#echo "deb http://ppa.launchpad.net/guido-iodice/natty-quasi-rolling/ubuntu natty main " > /etc/apt/sources.list.d/guido-iocide-natty.list


#UPDATE REPOSITORY
apt-get update
apt-get -y --force-yes install git-core wget unzip cmake aptitude pkg-config python-software-properties
apt-add-repository ppa:ogre-team/ogre
apt-add-repository ppa:guido-iodice/natty-quasi-rolling
apt-get update
cd /$REX_DIR/naali

rm -fr /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH
rm -fr /$REX_DIR/$PACKAGE_NAME-$BRANCH-scenes-noarch

#UPDATE CORRECT VERSION NUMBERS + ARCHITECTURE
sed '/Architecture/c \Architecture: '$ARCH'' /$REX_DIR/config/control_$BRANCH > tmpfile ; mv tmpfile /$REX_DIR/config/control_$BRANCH
sed '/Version/c \Version: '$VER'' /$REX_DIR/config/control_$BRANCH > tmpfile ; mv tmpfile /$REX_DIR/config/control_$BRANCH

#CREATE STRUCTURE NEEDED FOR DEB PACKAGE && USE READY-MADE CONTROL FILE && DESKTOP ICON
mkdir -p /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/DEBIAN
mkdir -p /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH/
mkdir -p /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH/lib/

if [ $BRANCH == "tundra2" ];
then
	#CREATE STRUCTURE FOR SCENES PACKAGE
	sed '/Version/c \Version: '$VER'' /$REX_DIR/config/control_scenes > tmpfile ; mv tmpfile /$REX_DIR/config/control_scenes
	mkdir -p /$REX_DIR/$PACKAGE_NAME-$BRANCH-scenes-noarch/DEBIAN
	mkdir -p /$REX_DIR/$PACKAGE_NAME-$BRANCH-scenes-noarch/$FINAL_INSTALL_DIR-$BRANCH/
	cp /$REX_DIR/config/control_scenes /$REX_DIR/$PACKAGE_NAME-$BRANCH-scenes-noarch/DEBIAN/control
	cp /$REX_DIR/config/run-linux_${BRANCH}_server.sh /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH/run-server.sh
 
fi

cp -r /$REX_DIR/config/usr_$BRANCH /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/usr

cp /$REX_DIR/config/run-linux_${BRANCH}_viewer.sh /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH/run-viewer.sh
cp /$REX_DIR/config/control_$BRANCH /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/DEBIAN/control
#cp /$REX_DIR/config/postinst /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/DEBIAN/postinst
cp /$REX_DIR/config/postrm /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/DEBIAN/postrm

chmod 555 /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/DEBIAN/postinst
chmod 555 /$REX_DIR/config/postrm /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/DEBIAN/postrm

#WE NEED TO REMOVE -g SWITCH FROM THE BASH SCRIPT BELOW SINCE WE DON'T NEED DEBUG INFORMATION + FILE SIZE INCREASES DRAMATICALLY
sed -i 's/ccache g++ -O -g/ccache g++ -O/g' /$REX_DIR/naali/tools/build-ubuntu-deps.bash

sed -i 's/libboost-all-dev/libboost1.46-all-dev/g' /$REX_DIR/naali/tools/build-ubuntu-deps.bash
sed -i 's/libogre-dev/libogre1.7.3-dev/g' /$REX_DIR/naali/tools/build-ubuntu-deps.bash

sed -i 's/apt-get install/apt-get -y install/g' /$REX_DIR/naali/tools/build-ubuntu-deps.bash

#ENABLE OGRE BUILD
#sed -i "s#private_ogre=false#private_ogre=true#g" /$REX_DIR/naali/tools/build-ubuntu-deps.bash

/$REX_DIR/naali/tools/build-ubuntu-deps.bash
errorCheck "Check for error with build process"

cp -r /$REX_DIR/naali/bin/* /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH
cp /$REX_DIR/naali-deps/install/lib/*.so* /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH

#CREATE DEB PACKAGE USING DPKG, BY DEFAULT THE PACKAGE NAME WILL BE THE FOLDER NAME
cd /$REX_DIR

mv $PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH/scenes /$REX_DIR/$PACKAGE_NAME-$BRANCH-scenes-noarch/$FINAL_INSTALL_DIR-$BRANCH/
chmod -R a+rX $PACKAGE_NAME-$BRANCH-scenes-noarch
dpkg -b $PACKAGE_NAME-$BRANCH-scenes-noarch

cp /$REX_DIR/naali-deps/build/knet/lib/libkNet.so /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH

chmod -R a+rX $PACKAGE_NAME-$BRANCH-$VER-$ARCH
dpkg -b  $PACKAGE_NAME-$BRANCH-$VER-$ARCH
errorCheck "Check for error with dpkg"

#IF $TIMESTAMP IS SET ADD TIMESTAMP TO NAME
if [ x$USESTAMP == xset ]; then
	mv $PACKAGE_NAME-$BRANCH-$VER-$ARCH.deb $PACKAGE_NAME-$BRANCH-$VER-$DATE-$BUILDNUMBER-$ARCH.deb
fi

