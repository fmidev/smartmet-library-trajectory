%define DIRNAME trajectory
%define BINNAME smartmet-%{DIRNAME}
%define SPECNAME smartmet-library-%{DIRNAME}
%define DEVELNAME %{SPECNAME}-devel
Summary: Trajectory calculation
Name: %{BINNAME}
Version: 17.4.4
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Tools
URL: https://github.com/fmidev/smartmet-library-trajectory
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: smartmet-library-newbase-devel >= 17.3.9
BuildRequires: smartmet-library-smarttools-devel >= 17.3.9
BuildRequires: smartmet-library-locus-devel >= 17.3.14
BuildRequires: smartmet-library-macgyver-devel >= 17.3.14
BuildRequires: boost-devel
BuildRequires: ctpp2 >= 2.8.2
Requires: smartmet-library-macgyver >= 17.3.14
Requires: smartmet-library-locus >= 17.3.14
Requires: smartmet-library-newbase >= 17.3.9
Requires: smartmet-library-smarttools >= 17.3.9
Requires: smartmet-library-trajectory
Requires: smartmet-trajectory-formats
Requires: boost-date-time
Requires: boost-filesystem
Requires: boost-iostreams
Requires: boost-locale
Requires: boost-program-options
Requires: boost-regex
Requires: boost-thread
Requires: boost-system
Provides: qdtrajectory
Obsoletes: smartmet-trajectory < 17.1.4
Obsoletes: smartmet-trajectory-debuginfo < 17.1.4

%description
FMI Trajectory Calculation Tools

%package -n %{SPECNAME}
Summary: Trajectory calculation library
Group: Development/Libraries
Requires: smartmet-library-locus >= 17.3.14
Provides: %{SPECNAME}
Obsoletes: libsmartmet-trajectory < 17.1.4
%description -n %{SPECNAME}
FMI Trajectory Calculation Libraries

%package -n %{DEVELNAME}
Summary: Trajectory calculation library
Group: Development/Libraries
Requires: smartmet-library-locus >= 17.3.14
Provides: %{DEVELNAME}
Obsoletes: libsmartmet-trajectory-devel < 17.1.4
%description -n %{DEVELNAME}
FMI Trajectory Calculation Libraries

%package -n %{SPECNAME}-formats
Summary: Trajectory calculation library data formats
Group: Development/Libraries
Provides: %{SPECNAME}-formats
%description -n %{SPECNAME}-formats
FMI Trajectory Calculation Libraries

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{BINNAME}
 
%build
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,0775)
%{_bindir}/qdtrajectory

%files -n %{SPECNAME}-formats
%defattr(0664,root,root,0775)
%{_datadir}/smartmet/%{DIRNAME}/gpx.c2t
%{_datadir}/smartmet/%{DIRNAME}/kml.c2t
%{_datadir}/smartmet/%{DIRNAME}/kmlx.c2t
%{_datadir}/smartmet/%{DIRNAME}/kmz.c2t
%{_datadir}/smartmet/%{DIRNAME}/kmzx.c2t
%{_datadir}/smartmet/%{DIRNAME}/xml.c2t

%files -n %{SPECNAME}
%defattr(0775,root,root,0775)
%{_libdir}/lib%{BINNAME}.so
%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig


%files -n %{DEVELNAME}
%defattr(0664,root,root,0775)
%{_includedir}/smartmet/%{DIRNAME}

%changelog
* Tue Apr  4 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.4.4-1.fmi
- Recompiled due to smarttools API changes
- Fixed to specify database host and user name explicitly

* Tue Mar 14 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.3.14-1.fmi
- Switched to using macgyver StringConversion tools
 
* Sat Feb 11 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.11-1.fmi
- Repackaged due to newbase API change

* Fri Jan 27 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.27-1.fmi
- Recompiled due to NFmiQueryData object size change

* Wed Jan  4 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.4-1.fmi
- Switched to FMI open source naming conventions

* Tue Jun 28 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.28-1.fmi
- newbase API changed

* Sun Jan 17 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.17-1.fmi
- newbase API changed

* Wed Oct 28 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.10.28-1.fmi
- Removed deprecated number_cast calls

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
