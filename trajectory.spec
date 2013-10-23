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
BuildRequires: libsmartmet-newbase >= 13.9.23
BuildRequires: libsmartmet-smarttools >= 13.9.23
BuildRequires: boost-devel >= 1.53
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
%makeinstall includedir=%{buildroot}%{_includedir}/smartmet

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0775)
%{_bindir}/qdtrajectory

%files -n libsmartmet-%{LIBNAME}
%{_includedir}/smartmet/%{LIBNAME}
%{_libdir}/libsmartmet_%{LIBNAME}.a

%changelog
* Wed Oct 23 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.10.23-1.fmi
- Added building of a separate tools library
* Wed Sep 25 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.9.25-1.fmi
- Extracted from smartmetbizcode into a separate library
