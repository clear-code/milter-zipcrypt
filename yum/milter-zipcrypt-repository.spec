Summary: milter-zipcrypt RPM repository configuration
Name: milter-zipcrypt-repository
Version: 1.0.0
Release: 0
License: GPLv3+
URL: http://milter-zipcrypt.sourceforge.net/
Source: milter-zipcrypt-repository.tar.gz
Group: System Environment/Base
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-%(%{__id_u} -n)
BuildArchitectures: noarch

%description
milter-zipcrypt RPM repository configuration.

%prep
%setup -c

%build

%install
%{__rm} -rf %{buildroot}

%{__install} -Dp -m0644 RPM-GPG-KEY-milter-zipcrypt %{buildroot}%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-milter-zipcrypt

%{__install} -Dp -m0644 milter-zipcrypt.repo %{buildroot}%{_sysconfdir}/yum.repos.d/milter-zipcrypt.repo

%clean
%{__rm} -rf %{buildroot}

%post
rpm -q gpg-pubkey-1c837f31-4a2b9c3f &>/dev/null || \
    rpm --import %{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-milter-zipcrypt

%files
%defattr(-, root, root, 0755)
%doc *
%pubkey RPM-GPG-KEY-milter-zipcrypt
%dir %{_sysconfdir}/yum.repos.d/
%config(noreplace) %{_sysconfdir}/yum.repos.d/milter-zipcrypt.repo
%dir %{_sysconfdir}/pki/rpm-gpg/
%{_sysconfdir}/pki/rpm-gpg/RPM-GPG-KEY-milter-zipcrypt

%changelog
* Sat Feb 06 2010 Clear Code Inc. <info@clear-code.com>
- (1.0.0-0)
- Initial package.
