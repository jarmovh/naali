Summary: Tundra viewer scenes
Name: Tundra-Fedora-Scenes
Version: 0.0
Release: 1
License: GPL
Group: Applications/Internet
Source: naali-latest.tar.gz
Patch: Tundra-Fedora.patch
BuildRoot: /opt/realXtend/

%description
Tundra is software to view and host interconnected 3D worlds or "3D internet".

%prep

%build

%install
mkdir -p $HOME/rpmbuild/BUILDROOT/Tundra-Fedora-Scenes-0.0-1.x86_64/opt/realXtend/
cp -r $HOME/naali-build/naali/bin/scenes $HOME/rpmbuild/BUILDROOT/Tundra-Fedora-Scenes-0.0-1.x86_64/opt/realXtend/

chmod 755 -R $HOME/rpmbuild/BUILDROOT/Tundra-Fedora-Scenes-0.0-1.x86_64/*

%clean
#rm -rf $RPM_BUILD_ROOT

%files
/opt/realXtend/*
%defattr(-,root,root)
%dir /opt/realXtend/

%doc
/opt/realXtend/*

%changelog


