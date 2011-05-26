#!/bin/bash
# Script to build naali and dependencies for Fedora 14

#IN CASE ERROR HAPPENS, $?-VARIABLE IS != 0
function errorCheck {
    if [ $? -ne 0 ];
    then
        echo $1
        exit $?
    fi
}

export PYTHON_PATH=/usr/bin/
export PYTHON_LIB=/usr/lib/python2.7/

if [ $(pwd) == / ]; then
    cd /builddir
fi

ARCH=$1
TIMESTAMP=$2
VER=$3

rpmbuild=/rpmbuild
naalidir=$(pwd)/naali
doneflags=$rpmbuild/flags
packets=/packages/usr
workdir=$(pwd)

pcount=`grep -c "^processor" /proc/cpuinfo`

yum install -y git-core

#if [ ! -d $naalidir ]; then
#	git clone git://github.com/jarmovh/naali.git ./naali
#else
	cd $naalidir
#	git stash
	git pull git://github.com/jarmovh/naali.git tundra
	cd ..
#fi

#sed -e "s#.x86_64#.$ARCH#g" /builddir/specs/tundra-fedora-0.0-1.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-0.0-1.spec

mv /builddir/specs/tundra-fedora-0.0-fc14.spec /builddir/specs/tundra-fedora-$VER-fc14.spec

sed -e "s#/naali-build#$workdir#g" /builddir/specs/tundra-fedora-$VER-fc14.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-$VER-fc14.spec
sed -e 's#$HOME##g' /builddir/specs/tundra-fedora-$VER-fc14.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-$VER-fc14.spec
sed -e "s#/naali-build#$workdir#g" /builddir/specs/tundra-fedora-scenes-$VER-fc14.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-scenes-$VER-fc14.spec
sed -e 's#$HOME##g' /builddir/specs/tundra-fedora-scenes-$VER-fc14.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-scenes-$VER-fc14.spec
sed -e "s#0.0#$VER#g" /builddir/specs/tundra-fedora-$VER-fc14.spec > tmpfile ; mv tmpfile /builddir/specs/tundra-fedora-$VER-fc14.spec

mkdir -p $packets/lib $packets/include $packets/../rpms
mkdir -p $rpmbuild/SPECS $rpmbuild/SOURCES $doneflags
cp /builddir/specs/* $rpmbuild/SPECS

cd $naalidir/tools

sed -i 's/ccache g++ -O -g /ccache g++ -O /' $naalidir/tools/build-fedora14-deps.bash

./build-fedora14-deps.bash
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

rpmbuild -bb  -vv --target x86_64 --define '_topdir /rpmbuild' --define '_signature gpg' --define "_gpg_name Jarmo Vähä" $rpmbuild/SPECS/tundra-fedora-scenes-$VER-1.spec
rpmbuild -bb -vv --target x86_64 --define '_topdir /rpmbuild' --define '_signature gpg' --define "_gpg_name Jarmo Vähä" $rpmbuild/SPECS/tundra-fedora-$VER-fc14.spec


