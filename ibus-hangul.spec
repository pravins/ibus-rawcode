%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
%define mod_path ibus-1.1
Name:       ibus-hangul
Version:    1.1.0.20090219
Release:    1%{?dist}
Summary:    The Hangul engine for IBus input platform
License:    GPLv2+
Group:      System Environment/Libraries
URL:        http://code.google.com/p/ibus/
Source0:    http://ibus.googlecode.com/files/%{name}-%{version}.tar.gz

BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  gettext-devel
BuildRequires:  libtool
BuildRequires:  libhangul-devel
BuildRequires:  pkgconfig

Requires:   ibus

%description
The Hangul engine for IBus platform. It provides Korean input method from
libhangul.

%prep
%setup -q

%build
%configure --disable-static
# make -C po update-gmo
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=${RPM_BUILD_ROOT} install
rm -f $RPM_BUILD_ROOT%{python_sitearch}/_hangul.la

%find_lang %{name}

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %{name}.lang
%defattr(-,root,root,-)
%doc AUTHORS COPYING README
%{_libexecdir}/ibus-engine-hangul
%{_datadir}/ibus-hangul
%{_datadir}/ibus/component/*

%changelog
* Fri Aug 08 2008 Huang Peng <shawn.p.huang@gmail.com> - 1.1.0.20090219-1
- The first version.
