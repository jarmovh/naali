#!/bin/bash

# Script to build naali and dependencies for Fedora 13
# Creates rpm packages from deps for easy installation.

#IN CASE ERROR HAPPENS, $?-VARIABLE IS != 0
function errorCheck {
    if [ $? -ne 0 ];
    then
        echo $1
        exit $?
    fi
}

export PYTHON_PATH=/usr/bin/
export PYTHON_LIB=/usr/lib/python2.6/

if [ $(pwd) == / ]; then
    cd /builddir
fi

ARCH=$1
TIMESTAMP=$2
VER=$3
TAG=$4
USETSTAMP=$5


rpmbuild=/rpmbuild
naalidir=$(pwd)/naali
doneflags=$rpmbuild/flags
packets=/packages/usr
workdir=$(pwd)

pcount=`grep -c "^processor" /proc/cpuinfo`

yum install -y git-core

cd $naalidir

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
	VER=`grep "Tundra" $naalidir/Viewer/main.cpp | cut -d 'v' -f2 -|cut -d '-' -f 1`
fi

git remote add upstream git://github.com/realXtend/naali.git
git fetch upstream
git branch master
git checkout master
git merge -s recursive remotes/upstream/master
cd ..



#sed -e "s#.x86_64#.$ARCH#g" /builddir/specs/tundra-fedora-0.0-1.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-0.0-1.spec

mv /builddir/specs/tundra-fedora-0.0-fc13.spec /builddir/specs/tundra-fedora-$VER-fc13.spec

sed -e "s#/naali-build#$workdir#g" /builddir/specs/tundra-fedora-$VER-fc13.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-$VER-fc13.spec
sed -e 's#$HOME##g' /builddir/specs/tundra-fedora-$VER-fc13.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-$VER-fc13.spec
sed -e "s#/naali-build#$workdir#g" /builddir/specs/tundra-fedora-scenes-$VER-fc13.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-scenes-$VER-fc13.spec
sed -e 's#$HOME##g' /builddir/specs/tundra-fedora-scenes-$VER-fc13.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-scenes-$VER-fc13.spec
sed -e "s#0.0#$VER#g" /builddir/specs/tundra-fedora-$VER-fc13.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-$VER-fc13.spec

mkdir -p $packets/lib $packets/include $packets/../rpms
mkdir -p $rpmbuild/SPECS $rpmbuild/SOURCES $doneflags
cp /builddir/specs/* $rpmbuild/SPECS

cd $naalidir/tools

sed -i 's/ccache g++ -O -g /ccache g++ -O /' $naalidir/tools/build-fedora13-deps.bash

./build-fedora13-deps.bash
errorCheck "Check for error with build process"

cat > $naalidir/bin/run-viewer.sh << EOF
#!/bin/bash
cd /opt/realXtend
export LD_LIBRARY_PATH=.:./modules/core:./lib
./viewer "$@"
EOF

cat > $naalidir/bin/run-server.sh << EOF
#!/bin/bash
cd /opt/realXtend
export LD_LIBRARY_PATH=.:./modules/core:./lib
./server "$@"
EOF

cd $naalidir/bin/
#strip * */**

chmod 755 run-server.sh
chmod 755 run-viewer.sh

rpmbuild -bb  -vv --target x86_64 --define '_topdir /rpmbuild' --define '_signature gpg' --define '_gpg_name Jarmo Vaehae' --sign $rpmbuild/SPECS/tundra-fedora-scenes-$VER-fc13.spec
rpmbuild -bb -vv --target x86_64 --define '_topdir /rpmbuild' $rpmbuild/SPECS/tundra-fedora-$VER-fc13.spec

if [ $USETSTAMP ]; then
	mv $rpmbuild/RPMS/x86_64/Tundra-Fedora-$VER-fc13.x86_64.rpm $rpmbuild/RPMS/x86_64/Tundra-Fedora-$VER-fc13.x86_64-$TIMESTAMP.rpm	
fi


