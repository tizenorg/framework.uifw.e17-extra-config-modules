Name:           e17-extra-config-modules
Summary:        The E17 Extra Config Modules The E17 extra config modules consists of modules made by SAMSUNG
Version:        0.1.140
Release:        1
Group:          TO_BE/FILLED_IN
License:        Apache License, Version 2.0
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(enlightenment)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(ecore-file)
BuildRequires:  pkgconfig(capi-content-media-content)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(xi)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(vconf-internal-keys)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(storage)
BuildRequires:  cmake
BuildRequires:  gettext
BuildRequires:  edje-tools
BuildRequires:  efl-assist-devel
Requires: libx11


%description
The E17 Extra Config Modules  The E17 extra config modules consists of modules made by SAMSUNG.

%define DEF_SUBDIRS config-tizen

%prep
%setup -q

%build
export CFLAGS+=" -Wall -g -fPIC -rdynamic"
export LDFLAGS+=" -Wl,--hash-style=both -Wl,--as-needed -Wl,--rpath=/usr/lib"

%ifarch %{arm}
export CFLAGS+=" -D_ENV_ARM"
%endif

%if "%{?tizen_profile_name}" == "wearable"
	export CFLAGS+=" -DWEARABLE"
%endif

for FILE in %{DEF_SUBDIRS}
do
   (cd $FILE && ./autogen.sh && ./configure --prefix=/usr ${CONFIGURE_OPTIONS} && make )
done

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/usr/share/license/%{name}
mkdir -p %{buildroot}/etc/smack/accesses.d
cp %{_builddir}/%{buildsubdir}/e17-extra-config-modules.efl %{buildroot}/etc/smack/accesses.d

for FILE in %{DEF_SUBDIRS}
do
        (cd $FILE && %make_install )
done

%files
%defattr(-,root,root,-)
%{_libdir}/enlightenment/modules/config-tizen/*
%manifest %{name}.manifest
/usr/share/license/%{name}
/etc/smack/accesses.d/%{name}.efl
