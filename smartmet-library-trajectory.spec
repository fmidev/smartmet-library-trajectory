%define DIRNAME trajectory
%define BINNAME smartmet-%{DIRNAME}
%define SPECNAME smartmet-library-%{DIRNAME}
%define DEVELNAME %{SPECNAME}-devel
Summary: Trajectory calculation library
Name: %{SPECNAME}
Version: 25.2.18
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Libraries
URL: https://github.com/fmidev/smartmet-library-trajectory
Source0: %{SPECNAME}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)

%if 0%{?rhel} && 0%{rhel} < 9
%define smartmet_boost boost169
%else
%define smartmet_boost boost
%endif

BuildRequires: rpm-build
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: smartmet-library-newbase-devel >= 25.2.18
BuildRequires: smartmet-library-gis-devel >= 25.2.18
BuildRequires: smartmet-library-smarttools-devel >= 25.2.18
BuildRequires: smartmet-library-locus-devel >= 25.2.18
BuildRequires: smartmet-library-macgyver-devel >= 25.2.18
BuildRequires: %{smartmet_boost}-devel
BuildRequires: ctpp2 >= 2.8.8
Requires: gdal310
Requires: smartmet-library-macgyver >= 25.2.18
Requires: smartmet-library-gis >= 25.2.18
Requires: smartmet-library-locus >= 25.2.18
Requires: smartmet-library-newbase >= 25.2.18
Requires: smartmet-library-smarttools >= 25.2.18
Requires: %{smartmet_boost}-filesystem
Requires: %{smartmet_boost}-iostreams
Requires: %{smartmet_boost}-locale
Requires: %{smartmet_boost}-program-options
Requires: %{smartmet_boost}-regex
Requires: %{smartmet_boost}-thread
Requires: %{smartmet_boost}-system
BuildRequires: gdal310-devel
Provides: qdtrajectory
Obsoletes: libsmartmet-trajectory < 17.1.4

%if 0%{?rhel} && 0%{rhel} == 8
Requires: libpqxx >= 1:7.7.0, libpqxx < 1:7.8.0
BuildRequires: libpqxx-devel >= 1:7.7.0, libpqxx-devel < 1:7.8.0
#TestRequires: libpqxx-devel >= 1:7.7.0, libpqxx-devel < 1:7.8.0
%else
%if 0%{?rhel} && 0%{rhel} == 9
Requires: libpqxx >= 1:7.9.0, libpqxx < 1:7.10.0
BuildRequires: libpqxx-devel >= 1:7.9.0, libpqxx-devel < 1:7.10.0
#TestRequires: libpqxx-devel >= 1:7.9.0, libpqxx-devel < 1:7.10.0
%else
%if 0%{?rhel} && 0%{rhel} >= 10
Requires: libpqxx >= 1:7.10.0, libpqxx < 1:7.11.0
BuildRequires: libpqxx-devel >= 1:7.10.0, libpqxx-devel < 1:7.11.0
#TestRequires: libpqxx-devel >= 1:7.10.0, libpqxx-devel < 1:7.11.0
%else
Requires: libpqxx
BuildRequires: libpqxx-devel
#TestRequires: libpqxx-devel
%endif
%endif
%endif

%description
FMI Trajectory Calculation Tools

%package -n %{DEVELNAME}
Summary: Trajectory calculation library
Group: Development/Libraries
Requires: smartmet-library-locus >= 25.2.18
Requires: %{SPECNAME} = %{version}-%{release}
Provides: %{DEVELNAME}
Obsoletes: libsmartmet-trajectory-devel < 17.1.4
Obsoletes: smartmet-trajectory
Obsoletes: smartmet-trajectory-formats
%description -n %{DEVELNAME}
FMI Trajectory Calculation Libraries

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{SPECNAME}
 
%build
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,0775)
%{_libdir}/lib%{BINNAME}.so
%{_bindir}/qdtrajectory
%defattr(0664,root,root,0775)
%{_datadir}/smartmet/%{DIRNAME}/gpx.c2t
%{_datadir}/smartmet/%{DIRNAME}/kml.c2t
%{_datadir}/smartmet/%{DIRNAME}/kmlx.c2t
%{_datadir}/smartmet/%{DIRNAME}/kmz.c2t
%{_datadir}/smartmet/%{DIRNAME}/kmzx.c2t
%{_datadir}/smartmet/%{DIRNAME}/xml.c2t

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files -n %{DEVELNAME}
%defattr(0664,root,root,0775)
%{_includedir}/smartmet/%{DIRNAME}

%changelog
* Tue Feb 18 2025 Andris Pavēnis <andris.pavenis@fmi.fi> 25.2.18-1.fmi
- Update to gdal-3.10, geos-3.13 and proj-9.5

* Wed Aug  7 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.8.7-1.fmi
- Update to gdal-3.8, geos-3.12, proj-94 and fmt-11

* Mon Jul 22 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.7.22-1.fmi
- Replace BOOST_FOREACH and boost::array

* Fri Jul 12 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.7.12-1.fmi
- Replace many boost library types with C++ standard library ones

* Thu May 16 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.5.16-1.fmi
- Clean up boost date-time uses

* Fri Feb 23 2024 Mika Heiskanen <mika.heiskanen@fmi.fi> 24.2.23-1.fmi
- Full repackaging

* Tue Jan 30 2024 Mika Heiskanen <mika.heiskanen@fmi.fi> 24.1.30-1.fmi
- Repackaged due to newbase ABI changes

* Fri Jul 28 2023 Andris Pavēnis <andris.pavenis@fmi.fi> 23.7.28-1.fmi
- Repackage due to bulk ABI changes in macgyver/newbase/spine

* Mon Jul 10 2023 Andris Pavēnis <andris.pavenis@fmi.fi> 23.7.10-1.fmi
- Use postgresql 15, gdal 3.5, geos 3.11 and proj-9.0

* Fri Jun 17 2022 Andris Pavēnis <andris.pavenis@fmi.fi> 22.6.17-1.fmi
- Add support for RHEL9. Update libpqxx to 7.7.0 (rhel8+) and fmt to 8.1.1

* Tue May 24 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.5.24-1.fmi
- Repackaged due to NFmiArea ABI changes

* Fri May 20 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.5.20-1.fmi
- Repackaged due to ABI changes to newbase LatLon methods

* Wed May 18 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.5.18-1.fmi
- Removed obsolete WGS84 ifdefs

* Fri Jan 21 2022 Andris Pavēnis <andris.pavenis@fmi.fi> 22.1.21-1.fmi
- Repackage due to upgrade of packages from PGDG repo: gdal-3.4, geos-3.10, proj-8.2

* Tue Dec  7 2021 Andris Pavēnis <andris.pavenis@fmi.fi> 21.12.7-1.fmi
- Update to postgresql 13 and gdal 3.3

* Fri Nov 19 2021 Andris Pavēnis <andris.pavenis@fmi.fi> 21.11.19-1.fmi
- Merge contents of RPM packages smartmet-trajectory and smartmet-trajectory-formats into smartmet-library-trajectory

* Wed Oct 13 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.10.13-1.fmi
- Fixed requirements and provides fields

* Thu Jul  8 2021 Andris Pavēnis <andris.pavenis@fmi.fi> 21.7.8-1.fmi
- Use libpqxx7 for RHEL8

* Mon Jun 21 2021 Andris Pavēnis <andris.pavenis@fmi.fi> 21.6.21-1.fmi
- Repackaged due to smartmet-library-locus ABI changes

* Thu May  6 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.5.6-1.fmi
- Repackaged due to NFmiAzimuthalArea ABI changes

* Thu Feb 18 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.2.18-1.fmi
- Repackaged due to NFmiArea ABI changes

* Tue Feb 16 2021 Andris Pavēnis <andris.pavenis@fmi.fi> - 21.2.16-2.fmi
- Repackaged due to newbase ABI changes

* Tue Feb 16 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.2.16-1.fmi
- Repackaged due to NFmiArea ABI changes

* Wed Jan 20 2021 Andris Pavenis <andris.pavenis@fmi.fi> - 21.2.20-1.fmi
- Use makefile.inc

* Thu Jan 14 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.14-1.fmi
- Repackaged smartmet to resolve debuginfo issues

* Tue Jan  5 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.5-1.fmi
- gdal upgrade

* Tue Dec 15 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.12.15-1.fmi
- Upgrade to pgdg12

* Thu Aug 27 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.8.27-1.fmi
- NFmiGrid API changed

* Wed Aug 26 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.8.26-1.fmi
- Repackaged due to NFmiGrid API changes

* Fri Aug 21 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.8.21-1.fmi
- Upgrade to fmt 6.2

* Fri Jul 31 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.7.31-1.fmi
- Repackaged due to libpqxx upgrade

* Mon Jun  8 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.6.8-1.fmi
- Upgraded libpqxx dependencies

* Sat Apr 18 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.4.18-1.fmi
- Upgraded to Boost 1.69

* Thu Mar 26 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.3.26-1.fmi
- Repackaged due to NFmiArea API changes

* Fri Feb 14 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.2.14-1.fmi
- Upgrade to pgdg12

* Fri Feb  7 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.2.7-1.fmi
- Repackaged due to newbase ABI changes

* Fri Dec 13 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.12.13-1.fmi
- Removed obsolete GDAL dependency

* Thu Dec 12 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.12.12-1.fmi
- Upgrade to GDAL 3.0

* Wed Nov 20 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.11.20-1.fmi
- Repackaged due to newbase API changes

* Thu Oct 31 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.10.31-1.fmi
- Repackaged due to newbase API/ABI changes

* Thu Sep 26 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.26-1.fmi
- Repackaged due to newbase ABI changes

* Wed Aug 28 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.8.28-1.fmi
- Repackaged since Locus Location object ABI changed

* Fri Aug 31 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.31-1.fmi
- Read database settings from a configuration file

* Sat Apr  7 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.4.7-1.fmi
- Upgrade to boost 1.66

* Mon Jan 15 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.1.15-1.fmi
- Recompiled due to updated postgresql and pqxx dependencies

* Mon Aug 28 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.8.28-1.fmi
- Upgrade to boost 1.65

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
