#!/bin/bash

export LANG=C

FINAL_INSTALL_DIR=/opt/realxtend
PACKAGE_NAME=realXtend
DATE=`date '+%y%m%d'`


#PARAMETERS PASSED 
BRANCH=$1
ARCH=$2
REX_DIR=$3
TAG=$4

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
deb http://fi.archive.ubuntu.com/ubuntu/ lucid main restricted
deb-src http://fi.archive.ubuntu.com/ubuntu/ lucid main restricted

deb http://fi.archive.ubuntu.com/ubuntu/ lucid-updates main restricted
deb-src http://fi.archive.ubuntu.com/ubuntu/ lucid-updates main restricted

deb http://fi.archive.ubuntu.com/ubuntu/ lucid universe
deb-src http://fi.archive.ubuntu.com/ubuntu/ lucid universe
deb http://fi.archive.ubuntu.com/ubuntu/ lucid-updates universe
deb-src http://fi.archive.ubuntu.com/ubuntu/ lucid-updates universe

deb http://fi.archive.ubuntu.com/ubuntu/ lucid multiverse
deb-src http://fi.archive.ubuntu.com/ubuntu/ lucid multiverse
deb http://fi.archive.ubuntu.com/ubuntu/ lucid-updates multiverse
deb-src http://fi.archive.ubuntu.com/ubuntu/ lucid-updates multiverse

deb http://security.ubuntu.com/ubuntu lucid-security main restricted
deb-src http://security.ubuntu.com/ubuntu lucid-security main restricted
deb http://security.ubuntu.com/ubuntu lucid-security universe
deb-src http://security.ubuntu.com/ubuntu lucid-security universe
deb http://security.ubuntu.com/ubuntu lucid-security multiverse
deb-src http://security.ubuntu.com/ubuntu lucid-security multiverse
EOF

cat > /etc/resolv.conf << EOF
# Generated by NetworkManager
domain oulu.fi
search oulu.fi
nameserver 130.231.240.1
nameserver 130.231.240.7
EOF

cat > /etc/hosts << EOF
127.0.0.1	localhost
127.0.1.1	joni-desktop

# The following lines are desirable for IPv6 capable hosts
::1     localhost ip6-localhost ip6-loopback
fe00::0 ip6-localnet
ff00::0 ip6-mcastprefix
ff02::1 ip6-allnodes
ff02::2 ip6-allrouters
ff02::3 ip6-allhosts
EOF

#UPDATE REPOSITORY
apt-get update
apt-get -y install git-core wget unzip

#DOWNLOAD REALXTEND NAALI/TUNDRA DEVELOP BRANCH
rm -fr /$REX_DIR/naali
git clone git://github.com/realXtend/naali.git /$REX_DIR/naali

cd /$REX_DIR/naali/

if [ $TAG != "none" ];
then
	git show-ref $TAG
	if [ $? -ne 0 ];
	then
		echo "Invalid tag" $TAG
		exit 1
	fi

	git checkout $TAG
	VER=$TAG
	
else
	if [ $BRANCH == "tundra" ];
	then
	git remote add upstream git://github.com/realXtend/naali.git
	git checkout $BRANCH
	git fetch upstream
	git merge $BRANCH remotes/upstream/$BRANCH

	VER=`grep "Tundra" /$REX_DIR/naali/Ui/NaaliMainWindow.cpp | cut -d 'v' -f2 -|cut -d '"' -f 1`
	else
	VER=`grep "Naali_v" /$REX_DIR/naali/Application/main.cpp | cut -d 'v' -f2 | tail -1 |cut -d '"' -f1`
	fi
fi

rm -fr /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH
rm -fr /$REX_DIR/$PACKAGE_NAME-$BRANCH-scenes-noarch



#UPDATE CORRECT VERSION NUMBERS + ARCHITECTURE
sed '/Architecture/c \Architecture: '$ARCH'' /$REX_DIR/config/control_$BRANCH > tmpfile ; mv tmpfile /$REX_DIR/config/control_$BRANCH
sed '/Version/c \Version: '$VER'' /$REX_DIR/config/control_$BRANCH > tmpfile ; mv tmpfile /$REX_DIR/config/control_$BRANCH


#CREATE STRUCTURE NEEDED FOR DEB PACKAGE && USE READY-MADE CONTROL FILE && DESKTOP ICON
mkdir -p /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/DEBIAN
mkdir -p /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH/

if [ $BRANCH == "tundra" ];
then
	#CREATE STRUCTURE FOR SCENES PACKAGE
	sed '/Version/c \Version: '$VER'' /$REX_DIR/config/control_scenes > tmpfile ; mv tmpfile /$REX_DIR/config/control_scenes
	mkdir -p /$REX_DIR/$PACKAGE_NAME-$BRANCH-scenes-noarch/DEBIAN
	mkdir -p /$REX_DIR/$PACKAGE_NAME-$BRANCH-scenes-noarch/$FINAL_INSTALL_DIR-$BRANCH/
	cp /$REX_DIR/config/control_scenes /$REX_DIR/$PACKAGE_NAME-$BRANCH-scenes-noarch/DEBIAN/control
fi

cp -r /$REX_DIR/config/usr_$BRANCH /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/usr
cp /$REX_DIR/config/run-linux_$BRANCH.sh /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH/run-linux.sh
cp /$REX_DIR/config/control_$BRANCH /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/DEBIAN/control

#WE NEED TO REMOVE -g SWITCH FROM THE BASH SCRIPT BELOW SINCE WE DON'T NEED DEBUG INFORMATION + FILE SIZE INCREASES DRAMATICALLY
sed -i 's/ccache g++ -O -g/ccache g++ -O/g' /$REX_DIR/naali/tools/build-ubuntu-deps.bash

/$REX_DIR/naali/tools/build-ubuntu-deps.bash
errorCheck "Check for error with build process"

cp -r /$REX_DIR/naali/bin/* /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH

#FOLLOWING LINES WILL BE REMOVED LATER ON
cp /$REX_DIR/naali-deps/build/PythonQt2.0.1/lib/libPythonQt_QtAll.so.1 /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH
cp /$REX_DIR/naali-deps/build/PythonQt2.0.1/lib/libPythonQt.so.1 /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH/
cp /$REX_DIR/naali-deps/build/qtpropertybrowser-2.5_1-opensource/lib/libQtSolutions_PropertyBrowser-2.5.so.1 /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH
#########################################

#CREATE DEB PACKAGE USING DPKG, BY DEFAULT THE PACKAGE NAME WILL BE THE FOLDER NAME
cd /$REX_DIR

if [ $BRANCH == "tundra" ];
then
	cp /$REX_DIR/naali-deps/build/knet/lib/libkNet.so /$REX_DIR/$PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH	
	mv $PACKAGE_NAME-$BRANCH-$VER-$ARCH/$FINAL_INSTALL_DIR-$BRANCH/scenes /$REX_DIR/$PACKAGE_NAME-$BRANCH-scenes-noarch/$FINAL_INSTALL_DIR-$BRANCH/
		
	dpkg -b $PACKAGE_NAME-$BRANCH-scenes-noarch
fi

dpkg -b $PACKAGE_NAME-$BRANCH-$VER-$ARCH
errorCheck "Check for error with dpkg"

if [ $TAG == "none" ];
then
	mv $PACKAGE_NAME-$BRANCH-$VER-$ARCH.deb $PACKAGE_NAME-$BRANCH-$VER-$DATE-$ARCH.deb
else
	mv $PACKAGE_NAME-$BRANCH-$VER-$ARCH.deb $PACKAGE_NAME-$BRANCH-$VER-$ARCH.deb
fi

chmod 600 /$REX_DIR/config/keys/id_*
sftp -b /dev/stdin -oIdentityFile=/$REX_DIR/config/keys/id_rsa -oUserKnownHostsFile=/$REX_DIR/config/keys/known_hosts tundrasftp@chiru.cie.fi  << end-upload
put *.deb files/
bye
end-upload

