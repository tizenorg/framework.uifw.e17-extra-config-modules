Name:           e17-extra-config-modules
Summary:        The E17 Extra Config Modules
Version:        0.1.28
Release:        1
Group:          Graphical desktop/Enlightenment
License:        BSD
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(enlightenment)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(dlog)
Requires:       libX11


%description
The E17 Extra Config Modules  The E17 extra config modules consists of modules made by SAMSUNG.


%prep
%setup -q


%build
export CFLAGS+=" -Wall -g -fPIC -rdynamic"
export LDFLAGS+=" -Wl,--hash-style=both -Wl,--as-needed -Wl,--rpath=/usr/lib"

%ifarch %{arm}
export CFLAGS+=" -D_ENV_ARM"
%endif

for FILE in config-tizen
do
        (cd $FILE && ./autogen.sh && ./configure --prefix=/usr && make )
done


%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp %{_builddir}/%{buildsubdir}/LICENSE %{buildroot}/usr/share/license/%{name}

for FILE in config-tizen
do
        (cd $FILE && %make_install )
done


%files
%defattr(-,root,root,-)
%{_libdir}/enlightenment/modules/config-tizen/*
/usr/share/license/%{name}
%manifest %{name}.manifest
