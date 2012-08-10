Name:           e17-extra-private-modules
Summary:        The E17 Extra Private Modules The E17 extra private modules consists of modules made by SAMSUNG
Version:        0.1.19
Release:        1
Group:          TO_BE/FILLED_IN
License:        TO BE FILLED IN
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(enlightenment)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(dlog)
Requires: libx11


%description
The E17 Extra Private Modules  The E17 extra private modules consists of modules made by SAMSUNG.


%prep
%setup -q


%build
export CFLAGS+=" -Wall -g -fPIC -rdynamic"
export LDFLAGS+=" -Wl,--hash-style=both -Wl,--as-needed -Wl,--rpath=/usr/lib"

%ifarch %{arm}
export CFLAGS+=" -D_ENV_ARM"
%endif

for FILE in config-slp
do 
        (cd $FILE && ./autogen.sh && ./configure --prefix=/usr && make )
done


%install
rm -rf %{buildroot}

for FILE in config-slp
do 
        (cd $FILE && %make_install )
done


%files
%defattr(-,root,root,-)
%{_libdir}/enlightenment/modules/config-slp/*
