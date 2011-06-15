Summary: Tundra viewer
Name: realXtend-Tundra
Version: 0.0
Release: fc14
License: GPL
Group: Applications/Internet
BuildRoot: /opt/realXtend/
Requires: boost-date-time, boost-filesystem, boost-thread, boost-regex , boost-program-options, poco-foundation, poco-net, poco-util, openal-soft, ogre, openjpeg-libs, boost-test, gnutls, qtscriptgenerator, qtscriptbindings, xmlrpc-epi, libogg
%description
Tundra is software to view and host interconnected 3D worlds or "3D internet".

%prep

%build

%install
mkdir -p $HOME/rpmbuild/BUILDROOT/realXtend-Tundra-0.0-fc14.x86_64/opt/realXtend/lib
mkdir -p $HOME/rpmbuild/BUILDROOT/realXtend-Tundra-0.0-fc14.x86_64/usr/
cp -r $HOME/naali-build/naali/bin/* $HOME/rpmbuild/BUILDROOT/realXtend-Tundra-0.0-fc14.x86_64/opt/realXtend/
rm -fr $HOME/rpmbuild/BUILDROOT/realXtend-Tundra-0.0-fc14.x86_64/opt/realXtend/scenes
cp -fr $HOME/naali-build/naali-deps/install/lib/* $HOME/rpmbuild/BUILDROOT/realXtend-Tundra-0.0-fc14.x86_64/opt/realXtend/lib
cp -fr $HOME/naali-build/usr_tundra/* $HOME/rpmbuild/BUILDROOT/realXtend-Tundra-0.0-fc14.x86_64/usr/

chmod 755 -R $HOME/rpmbuild/BUILDROOT/realXtend-Tundra-0.0-fc14.x86_64/*

%clean
#rm -rf $RPM_BUILD_ROOT

%files
/opt/realXtend/*
/usr/*
%defattr(755,root,root)
%dir /opt/realXtend/

%doc
/opt/realXtend/*

%changelog

%post
gconftool-2 -s /desktop/gnome/url-handlers/tundra/command '/opt/realXtend/run-viewer.sh' --type String
gconftool-2 -s /desktop/gnome/url-handlers/tundra/enabled --type Boolean true

%postun
gconftool-2 -u /desktop/gnome/url-handlers/tundra/command
gconftool-2 -u /desktop/gnome/url-handlers/tundra/enabled


