Summary: Tundra dependencies
Name: Tundra-Dependencies
Version: 0.0
Release: 1
Group: Applications/Internet
Source: tundra-fedora-dependencies.tar.gz
License: GPL 
Patch: realXtend-Tundra.patch
BuildRoot: /opt/tundra/

%description
Tundra is software to view and host interconnected 3D worlds or "3D internet".

%prep

%build

%install
cp -r $HOME/rpmbuild/BUILD/bin BUILDROOT/realXtend-Tundra-0.0-1.x86_64/

%clean

%files
/bin/ 

%changelog


