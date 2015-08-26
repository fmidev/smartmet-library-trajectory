%define LIBNAME trajectory
Summary: Trajectory calculation
Name: smartmet-%{LIBNAME}
Version: 15.8.26
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: libsmartmet-newbase-devel >= 15.8.26
BuildRequires: libsmartmet-smarttools-devel >= 15.4.15
BuildRequires: libsmartmet-locus-devel >= 15.6.23
BuildRequires: libsmartmet-macgyver >= 15.8.24
BuildRequires: boost-devel
BuildRequires: ctpp2 >= 2.8.2
Requires: mysql++
Requires: bzip2
Requires: libsmartmet-macgyver >= 15.8.24
Requires: libsmartmet-locus >= 15.6.23
Requires: libsmartmet-newbase >= 15.8.26
Requires: libsmartmet-smarttools >= 15.4.15
Requires: boost-date-time
Requires: boost-filesystem
Requires: boost-iostreams
Requires: boost-locale
Requires: boost-program-options
Requires: boost-regex
Requires: boost-thread
Requires: boost-system
Provides: qdtrajectory

%description
FMI Trajectory Calculation Tools

%package -n libsmartmet-%{LIBNAME}
Summary: Trajectory calculation library
Group: Development/Libraries
Requires: libsmartmet-locus >= 15.6.23
Provides: libsmartmet-%{LIBNAME}
%description -n libsmartmet-%{LIBNAME}
FMI Trajectory Calculation Libraries

%package -n smartmet-%{LIBNAME}-formats
Summary: Trajectory calculation library data formats
Group: Development/Libraries
Provides: smartmet-%{LIBNAME}-formats
%description -n smartmet-%{LIBNAME}-formats
FMI Trajectory Calculation Libraries

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{LIBNAME}
 
%build
make %{_smp_mflags}

%install
%makeinstall includedir=%{buildroot}%{_includedir}/smartmet datadir=%{buildroot}%{_datadir}/smartmet

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,-)
%{_bindir}/qdtrajectory

%files -n libsmartmet-%{LIBNAME}
%{_libdir}/libsmartmet_%{LIBNAME}.so

%files -n smartmet-%{LIBNAME}-formats
%{_datadir}/smartmet/%{LIBNAME}/gpx.c2t
%{_datadir}/smartmet/%{LIBNAME}/kml.c2t
%{_datadir}/smartmet/%{LIBNAME}/kmlx.c2t
%{_datadir}/smartmet/%{LIBNAME}/kmz.c2t
%{_datadir}/smartmet/%{LIBNAME}/kmzx.c2t
%{_datadir}/smartmet/%{LIBNAME}/xml.c2t

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%package -n libsmartmet-%{LIBNAME}-devel
Summary: FMI trajectory development files
Provides: %{LIBNAME}-devel

%description -n libsmartmet-%{LIBNAME}-devel
FMI trajectory development files

%files -n libsmartmet-%{LIBNAME}-devel
%defattr(0664,root,root,-)
%{_includedir}/smartmet/%{LIBNAME}

%changelog
* Wed Aug 26 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.26-1.fmi
- Recompiled with latest newbase with faster parameter changing
* Wed Apr 15 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.15-1.fmi
- newbase API changed
* Thu Apr  9 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.9-1.fmi
- newbase API changed
* Mon Mar 30 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.3.30-1.fmi
- Switched to using dynamic linkage
* Tue Feb 24 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.2.24-1.fmi
- Recompiled with the latest newbase
* Thu Oct 30 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.10.30-1.fmi
- newbase has improved vertical interpolation algorithms
* Mon Sep  8 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.9.8-1.fmi
- Recompiled due to newbase API changes
* Tue Mar 18 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.3.18-1.fmi
- Use the maximum pressure available if the given height is too low
* Fri Feb 14 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.2.14-1.fmi
- Fixed checking of hybrid data
* Fri Jan 24 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.1.24-1.fmi
- Fixed PlaceMark case to Placemark
* Fri Jan 24 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.1.24-1.fmi
- Added missing PlaceMark end tag to KML template
* Mon Jan 20 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.1.20-1.fmi
- Split templates into a separate package
* Thu Dec 12 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.12.12-1.fmi
- Added options --height and --height-range
* Tue Nov 19 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.11.19-1.fmi
- Changed to use Locus library
* Tue Oct 29 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.10.29-1.fmi
- Fixed randomization of the trajectory
* Wed Oct 23 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.10.23-1.fmi
- Added building of a separate tools library
* Wed Sep 25 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.9.25-1.fmi
- Extracted from smartmetbizcode into a separate library
