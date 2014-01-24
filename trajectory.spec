%define LIBNAME trajectory
Summary: Trajectory calculation
Name: smartmet-%{LIBNAME}
Version: 14.1.24
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: libsmartmet-newbase >= 14.1.24
BuildRequires: libsmartmet-smarttools >= 13.12.2
BuildRequires: libsmartmet-locus >= 14.1.9
BuildRequires: libsmartmet-macgyver >= 14.1.14
BuildRequires: boost-devel
BuildRequires: ctpp2 >= 2.8.2
Requires: mysql++
Requires: bzip2
Provides: qdtrajectory

%description
FMI Trajectory Calculation Tools

%package -n libsmartmet-%{LIBNAME}
Summary: Trajectory calculation library
Group: Development/Libraries
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
%defattr(-,root,root,0775)
%{_bindir}/qdtrajectory

%files -n libsmartmet-%{LIBNAME}
%{_includedir}/smartmet/%{LIBNAME}
%{_libdir}/libsmartmet_%{LIBNAME}.a

%files -n smartmet-%{LIBNAME}-formats
%{_datadir}/smartmet/%{LIBNAME}/gpx.c2t
%{_datadir}/smartmet/%{LIBNAME}/kml.c2t
%{_datadir}/smartmet/%{LIBNAME}/kmlx.c2t
%{_datadir}/smartmet/%{LIBNAME}/kmz.c2t
%{_datadir}/smartmet/%{LIBNAME}/kmzx.c2t
%{_datadir}/smartmet/%{LIBNAME}/xml.c2t

%changelog
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
