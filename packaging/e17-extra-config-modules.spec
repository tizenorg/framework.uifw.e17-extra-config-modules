Name:           e17-extra-config-modules
Summary:        The E17 Extra Config Modules The E17 extra config modules consists of modules made by SAMSUNG
Version:        0.1.54b01
Release:        1
VCS:            magnolia/framework/uifw/e17-extra-config-modules#0.1.53-3-gaeed05a89c1f362fea3ea1d0eded3f076b269525
Group:          TO_BE/FILLED_IN
License:        BSD
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  pkgconfig(enlightenment)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(dlog)
%if %{_repository} == "wearable"
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(ecore-file)
BuildRequires:  pkgconfig(capi-content-media-content)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  gettext
BuildRequires:  edje-tools
%endif
Requires: libx11


%description
The E17 Extra Config Modules  The E17 extra config modules consists of modules made by SAMSUNG.

%if %{_repository} == "wearable"
%define DEF_SUBDIRS config-tizen shot-tizen backkey-tizen
%elseif %{_repository} == "mobile"
%define DEF_SUBDIRS config-tizen
%endif


%prep
%setup -q

%build
export CFLAGS+=" -Wall -g -fPIC -rdynamic"
export LDFLAGS+=" -Wl,--hash-style=both -Wl,--as-needed -Wl,--rpath=/usr/lib"


%if %{_repository} == "wearable"
cd po
rm -rf CMakeFiles
rm -rf CMakeCache.txt
cmake .
make %{?jobs:-j%jobs}
make install
cd ..
%endif


%ifarch %{arm}
export CFLAGS+=" -D_ENV_ARM"
%endif

for FILE in %{DEF_SUBDIRS}
do
        (cd $FILE && ./autogen.sh && ./configure --prefix=/usr && make )
done

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
%if %{_repository} == "wearable"
mkdir -p %{buildroot}/etc/smack/accesses2.d
cp %{_builddir}/%{buildsubdir}/e17-extra-config-modules.rule %{buildroot}/etc/smack/accesses2.d
cp %{_builddir}/%{buildsubdir}/po/locale %{buildroot}/usr/share -a
%endif
cp %{_builddir}/%{buildsubdir}/LICENSE %{buildroot}/usr/share/license/%{name}


for FILE in %{DEF_SUBDIRS}
do
        (cd $FILE && %make_install )
done


%files
%defattr(-,root,root,-)
%{_libdir}/enlightenment/modules/config-tizen/*
%if %{_repository} == "wearable"
%{_bindir}/e_backkey_util
%{_libdir}/enlightenment/modules/shot-tizen/*
%{_libdir}/enlightenment/modules/backkey-tizen/*
%{_datadir}/enlightenment/data/*
/usr/share/locale/*
/etc/smack/accesses2.d/%{name}.rule
%attr(0700,root,root) /etc/opt/upgrade/%{name}.patch.sh
%endif
%manifest %{name}.manifest
/usr/share/license/%{name}
