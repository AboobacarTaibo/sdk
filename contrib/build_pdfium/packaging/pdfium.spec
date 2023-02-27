Name:		pdfium-mega
Version:	5247.0
Release:	1%{?dist}
Summary:	TODO
License:	TODO
Group:		Applications/Others
Url:		https://mega.nz
Source0:	pdfium-mega_%{version}.tar.gz
#Source1:	pdfium-rpmlintrc
Vendor:		TODO
Packager:	TODO

BuildRequires: autoconf, automake, libtool, gcc-c++, unzip, rsync, git

%if 0%{?fedora} || 0%{?rhel_version} || 0%{?centos_version} || 0%{?scientificlinux_version}
BuildRequires: libatomic
%endif

%if 0%{?centos_version} == 800
BuildRequires: python36
%else
%if 0%{?suse_version} > 1500
BuildRequires: python
%else
BuildRequires: python3
%endif
%endif

%if 0%{?suse_version}
BuildRequires: libxml2-2
%endif

%description
TODO

%prep

%setup -q

%define debug_package %{nil}

%build
./depot_tools/ninja %{?_smp_mflags} -C out pdfium

#clean all unrequired stuff
rm -rf `ls | grep -v "out\|public"`

%install
pwd
%{__install} -D out/obj/libpdfium.a $RPM_BUILD_ROOT%{_libdir}/libpdfium.a
for i in `find public -type f -name "*.h"`; do %{__install} -D -m 444 $i $RPM_BUILD_ROOT%{_includedir}/${i/public\//}; done

(cd $RPM_BUILD_ROOT; for i in `find ./%{_libdir} -type f`; do echo $i | sed "s#^./#/#g"; done) >> %{_topdir}/ExtraFiles.list
(cd $RPM_BUILD_ROOT; for i in `find ./%{_includedir}`; do echo $i | sed "s#^./#/#g"; done) >> %{_topdir}/ExtraFiles.list

%post

%clean
%{?buildroot:%__rm -rf "%{buildroot}"}

%files -f %{_topdir}/ExtraFiles.list
%defattr(-,root,root)

%changelog
