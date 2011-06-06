Summary: Tundra viewer
Name: Tundra-Fedora
Version: 0.0
Release: fc13
License: GPL
Group: Applications/Internet
BuildRoot: /opt/realXtend/
Requires: boost-date-time, boost-filesystem, boost-thread, boost-regex , boost-program-options, poco-foundation, poco-net, poco-util, openal-soft, ogre, openjpeg-libs, boost-test, gnutls, qtscriptgenerator, qtscriptbindings
%description
Tundra is software to view and host interconnected 3D worlds or "3D internet".

%prep

%build

%install
mkdir -p $HOME/rpmbuild/BUILDROOT/Tundra-Fedora-0.0-fc13.x86_64/opt/realXtend/lib
mkdir -p $HOME/rpmbuild/BUILDROOT/Tundra-Fedora-0.0-fc13.x86_64/usr/
cp -r $HOME/naali-build/naali/bin/* $HOME/rpmbuild/BUILDROOT/Tundra-Fedora-0.0-fc13.x86_64/opt/realXtend/
rm -fr $HOME/rpmbuild/BUILDROOT/Tundra-Fedora-0.0-fc13.x86_64/opt/realXtend/scenes
cp -fr $HOME/naali-build/naali-deps/install/lib/* $HOME/rpmbuild/BUILDROOT/Tundra-Fedora-0.0-fc13.x86_64/opt/realXtend/lib
cp -fr $HOME/naali-build/usr_tundra/* $HOME/rpmbuild/BUILDROOT/Tundra-Fedora-0.0-fc13.x86_64/usr/

%clean
rm -rf $RPM_BUILD_ROOT

%files
/opt/realXtend/*
/usr/*
%defattr(755,root,root)
%dir /opt/realXtend/

%doc
/opt/realXtend/*

%changelog


