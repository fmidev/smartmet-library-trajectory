%define LIBNAME trajectory
Summary: Trajectory calculation
Name: smartmet-%{LIBNAME}
Version: 13.10.23
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Tools
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: libsmartmet-brainstorm-spine >= 13.10.9
BuildRequires: libsmartmet-newbase >= 13.10.17
BuildRequires: libsmartmet-smarttools >= 13.10.17
BuildRequires: libsmartmet-fminames >= 13.8.29
BuildRequires: libsmartmet-macgyver >= 13.10.8
BuildRequires: boost-devel
BuildRequires: ctpp2 >= 2.8.2
Requires: libsmartmet-brainstorm-spine >= 13.10.9
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
%{_datadir}/smartmet/%{LIBNAME}/gpx.c2t
%{_datadir}/smartmet/%{LIBNAME}/kml.c2t
%{_datadir}/smartmet/%{LIBNAME}/kmlx.c2t
%{_datadir}/smartmet/%{LIBNAME}/kmz.c2t
%{_datadir}/smartmet/%{LIBNAME}/kmzx.c2t
%{_datadir}/smartmet/%{LIBNAME}/xml.c2t

%files -n libsmartmet-%{LIBNAME}
%{_includedir}/smartmet/%{LIBNAME}
%{_libdir}/libsmartmet_%{LIBNAME}.a

%changelog
* Wed Oct 23 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.10.23-1.fmi
- Added building of a separate tools library
* Wed Sep 25 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.9.25-1.fmi
- Extracted from smartmetbizcode into a separate library
