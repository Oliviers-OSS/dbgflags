AUTOMAKE_OPTIONS = foreign
SUBDIRS = libdbgflags zo logproc man
#SUBDIRS = extras 
EXTRA_DIST = dbgflags.pc.in
ACLOCAL_AMFLAGS = -I m4

if TESTS_PROGRAMS_BUILD_ENABLED
    SUBDIRS += tests
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dbgflags.pc

# The rpmbuild command builds software in the $RPMBUILD_ROOT/BUILD directory.
# The rpmbuild command stores binary RPMs it creates in the $RPMBUILD_ROOT/RPMS directory
# $RPMBUILD_ROOT/SOURCES directory contains the sources for the application
# $RPMBUILD_ROOT/SPECS contains the spec file for each RPM you plan to make
# The rpmbuild command stores source RPMs it creates in the $RPMBUILD_ROOT/SRPMS directory.  
# TODO: use m4 macro to build rpm

RPMBUILD_ROOT = $(top_srcdir)/rpm
SPEC_FILE = ${RPMBUILD_ROOT}/SPECS/${PACKAGE_TARNAME}.spec
SPEC_BIN_FILE = ${RPMBUILD_ROOT}/SPECS/${PACKAGE_TARNAME}-bin.spec
FULL_RPMBUILD_ROOT = ${PWD}/rpm

dist-rpm: ${PACKAGE_TARNAME}-${PACKAGE_VERSION}.tar.gz
	/bin/rm -Rf ${RPMBUILD_ROOT} && \
	/bin/mkdir -p ${RPMBUILD_ROOT} ${RPMBUILD_ROOT}/BUILD ${RPMBUILD_ROOT}/RPMS ${RPMBUILD_ROOT}/SOURCES ${RPMBUILD_ROOT}/SPECS ${RPMBUILD_ROOT}/SRPMS 
	/bin/cp $(top_srcdir)/${PACKAGE_TARNAME}-${PACKAGE_VERSION}.tar.gz ${RPMBUILD_ROOT}/SOURCES
	echo /bin/rm -F ${RPMBUILD_ROOT}/SPECS/*
	/bin/echo "Summary: dbgflags library and tools" >${SPEC_FILE}
	/bin/echo "%define version ${PACKAGE_VERSION}" >>${SPEC_FILE}
	/bin/echo "License: LGPL (library) & GPL (tools)" >>${SPEC_FILE}
	/bin/echo "Group: Development" >>${SPEC_FILE}
	/bin/echo "Name: ${PACKAGE_NAME}" >>${SPEC_FILE}
	/bin/echo "Prefix: ${prefix}" >>${SPEC_FILE}
	/bin/echo "Release: ${LIBDBGFLAGS_VERSION}" >>${SPEC_FILE}
	/bin/echo "Source: ${PACKAGE_TARNAME}-${PACKAGE_VERSION}.tar.gz" >>${SPEC_FILE}
	/bin/echo "Version: %{version}" >>${SPEC_FILE}
	/bin/echo "Buildroot: /tmp/${PACKAGE_NAME}" >>${SPEC_FILE}
	/bin/echo "%description" >>${SPEC_FILE}
	/bin/echo "dynamic tracing and loggers library" >>${SPEC_FILE}
	/bin/echo "%prep" >>${SPEC_FILE}
	/bin/echo "%setup -q -n ${PACKAGE_NAME}-%{PACKAGE_VERSION}" >>${SPEC_FILE}
	/bin/echo "%build" >>${SPEC_FILE}
	/bin/echo "./configure --prefix=${prefix} && make" >>${SPEC_FILE}
	/bin/echo "%install" >>${SPEC_FILE}
	/bin/echo "/bin/rm -Rf ${RPM_BUILD_ROOT}" >>${SPEC_FILE}
	/bin/echo "make DESTDIR=%{?buildroot:%{buildroot}} install" >>${SPEC_FILE}
	/bin/echo "%clean" >>${SPEC_FILE}
	/bin/echo "/bin/rm -Rf ${RPM_BUILD_ROOT}" >>${SPEC_FILE}
	/bin/echo "%files" >>${SPEC_FILE}
	/bin/echo "%defattr(-,root,root)" >>${SPEC_FILE}
	/bin/echo "${prefix}/bin/logproc" >>${SPEC_FILE}
	/bin/echo "${prefix}/bin/zo" >>${SPEC_FILE}
	for f in `/bin/ls -1 ${PWD}/include/dbgflags/*.h ` ; do /bin/echo "${prefix}/include/dbgflags-${LIBDBGFLAGS_VERSION}/"`/usr/bin/basename $$f` >>${SPEC_FILE} ; done 
	/bin/echo "${prefix}/include/dbgflags-${LIBDBGFLAGS_VERSION}/loggers_streams" >>${SPEC_FILE}
	/bin/echo "${prefix}/include/dbgflags-${LIBDBGFLAGS_VERSION}/syslogex.h" >>${SPEC_FILE}
	/bin/echo "${prefix}/lib/libdbgflags.a" >>${SPEC_FILE}
	/bin/echo "${prefix}/lib/libdbgflags.la" >>${SPEC_FILE}
	for f in `/bin/ls -1 ${PWD}/libdbgflags/.libs/lib*.so* `; do /bin/echo "${prefix}/lib/"`/usr/bin/basename $$f` >>${SPEC_FILE} ; done 	
	/bin/echo "${prefix}/lib/pkgconfig/dbgflags.pc" >>${SPEC_FILE}
	/bin/echo "%_topdir ${FULL_RPMBUILD_ROOT}" >${FULL_RPMBUILD_ROOT}/rpmmacros
	/bin/echo "%_topdir ${FULL_RPMBUILD_ROOT}" >${HOME}/.rpmmacros
	/bin/cp $(top_srcdir)/${PACKAGE_TARNAME}-${PACKAGE_VERSION}.tar.gz ${RPMBUILD_ROOT}/SOURCES
	rpmbuild -ba --macros=${FULL_RPMBUILD_ROOT}/rpmmacros ${SPEC_FILE} 

dist-bin-rpm: 
	/bin/rm -Rf ${RPMBUILD_ROOT} && \
	/bin/mkdir -p ${RPMBUILD_ROOT} ${RPMBUILD_ROOT}/BUILD ${RPMBUILD_ROOT}/RPMS ${RPMBUILD_ROOT}/SOURCES ${RPMBUILD_ROOT}/SPECS ${RPMBUILD_ROOT}/SRPMS 
	/bin/cp $(top_srcdir)/${PACKAGE_TARNAME}-${PACKAGE_VERSION}.tar.gz ${RPMBUILD_ROOT}/SOURCES
	echo /bin/rm -F ${RPMBUILD_ROOT}/SPECS/*
	/bin/echo "Summary: dbgflags library and tools runtime" >${SPEC_BIN_FILE}
	/bin/echo "%define version ${PACKAGE_VERSION}" >>${SPEC_BIN_FILE}
	/bin/echo "License: LGPL (library) & GPL (tools)" >>${SPEC_BIN_FILE}
	/bin/echo "Group: Development" >>${SPEC_BIN_FILE}
	/bin/echo "Name: ${PACKAGE_NAME}" >>${SPEC_BIN_FILE}
	/bin/echo "Prefix: ${prefix}" >>${SPEC_BIN_FILE}
	/bin/echo "Release: bin" >>${SPEC_BIN_FILE}	
	/bin/echo "Version: %{version}" >>${SPEC_BIN_FILE}
	/bin/echo "Source: ${PACKAGE_TARNAME}-${PACKAGE_VERSION}.tar.gz" >>${SPEC_FILE}
	/bin/echo "Buildroot: ${FULL_RPMBUILD_ROOT}/BUILD" >>${SPEC_BIN_FILE}
	/bin/echo "%description" >>${SPEC_BIN_FILE}
	/bin/echo "dynamic tracing and loggers library runtime" >>${SPEC_BIN_FILE}
	/bin/echo "%prep" >>${SPEC_BIN_FILE}	
	/bin/echo "%build" >>${SPEC_BIN_FILE}
	/bin/echo "/bin/mkdir -p ${FULL_RPMBUILD_ROOT}/BUILD${prefix}/bin " >>${SPEC_BIN_FILE}
	/bin/echo "/bin/cp ${PWD}/zo/.libs/zo ${FULL_RPMBUILD_ROOT}/BUILD${prefix}/bin" >>${SPEC_BIN_FILE}
	/bin/echo "/bin/cp ${PWD}/logproc/.libs/logproc ${FULL_RPMBUILD_ROOT}/BUILD${prefix}/bin" >>${SPEC_BIN_FILE}
	/bin/echo "/bin/mkdir -p ${FULL_RPMBUILD_ROOT}/BUILD${prefix}/lib " >>${SPEC_BIN_FILE}
	/bin/echo "/bin/cp -d ${PWD}/libdbgflags/.libs/*.so* ${FULL_RPMBUILD_ROOT}/BUILD${prefix}/lib" >>${SPEC_BIN_FILE}
	/bin/echo "/bin/cp ${PWD}/libdbgflags/.libs/*a ${FULL_RPMBUILD_ROOT}/BUILD${prefix}/lib" >>${SPEC_BIN_FILE}
	/bin/echo "/bin/mkdir -p ${FULL_RPMBUILD_ROOT}/BUILD${prefix}/lib/pkgconfig" >>${SPEC_BIN_FILE}
	/bin/echo "/bin/cp ${PWD}/dbgflags.pc ${prefix}/lib/pkgconfig/dbgflags.pc" >>${SPEC_FILE}
	/bin/echo "%clean" >>${SPEC_BIN_FILE}
	/bin/echo "/bin/rm -Rf ${RPM_BUILD_ROOT}" >>${SPEC_BIN_FILE}
	/bin/echo "%files" >>${SPEC_BIN_FILE}
	/bin/echo "%defattr(-,root,root)" >>${SPEC_BIN_FILE}
	/bin/echo "${prefix}/bin/logproc" >>${SPEC_BIN_FILE}
	/bin/echo "${prefix}/bin/zo" >>${SPEC_BIN_FILE}	
	/bin/echo "${prefix}/lib/libdbgflags.a" >>${SPEC_BIN_FILE}
	/bin/echo "${prefix}/lib/libdbgflags.la" >>${SPEC_BIN_FILE}
	for f in `/bin/ls -1 ${PWD}/libdbgflags/.libs/lib*.so* `; do /bin/echo "${prefix}/lib/"`/usr/bin/basename $$f` >>${SPEC_BIN_FILE} ; done 
	/bin/echo "${prefix}/lib/pkgconfig/dbgflags.pc" >>${SPEC_FILE}
	/bin/echo "%_topdir ${FULL_RPMBUILD_ROOT}" >${FULL_RPMBUILD_ROOT}/rpmmacros
	/bin/echo "%_topdir ${FULL_RPMBUILD_ROOT}" >${HOME}/.rpmmacros
	rpmbuild -v -bb --macros=${FULL_RPMBUILD_ROOT}/rpmmacros ${SPEC_BIN_FILE} 

# Debian packages build

#DEB_BUILD_ROOT = $(top_srcdir)/${PACKAGE_TARNAME}-${PACKAGE_VERSION}
DEB_BUILD_ROOT = $(top_srcdir)/packages
#DEB_HOST_GNU_TYPE  = $(shell dpkg-architecture -qDEB_HOST_GNU_TYPE)
#DEB_BUILD_GNU_TYPE = $(shell dpkg-architecture -qDEB_BUILD_GNU_TYPE)
#DEBEMAIL = $(PACKAGE_BUGREPORT)
#DEBFULLNAME = `cat $(top_srcdir)/AUTHORS `
#ROOT_DIR=${PWD}
#FULL_DEBBUILD_ROOT = ${PWD}/${PACKAGE_TARNAME}-${PACKAGE_VERSION}
#DEB_CHANGELOG = $(FULL_DEBBUILD_ROOT)/debian/changelog
#DEB_PKG_DEV = $(FULL_DEBBUILD_ROOT)/debian/dbgflags-dev.install
#DEB_PKG_RT = $(FULL_DEBBUILD_ROOT)/debian/dbgflags1.install
#DEB_PKG_DOC = $(FULL_DEBBUILD_ROOT)/debian/dbgflags1.docs

# runtime Debian package 
dist-deb: 	
	autoreconf -fvi
	make clean && make # no configure to keep the last build options
	make dist	
	/bin/rm -Rf ${DEB_BUILD_ROOT}/libdbgflags 2>/dev/null
	/bin/mkdir -p ${DEB_BUILD_ROOT}/libdbgflags/DEBIAN
	/bin/echo "Package: " ${PACKAGE_TARNAME} >${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control
	/bin/echo "Version: " ${PACKAGE_VERSION} >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control
	if [ -n `echo ${host_cpu} | grep 64 ` ] ; then /bin/echo "Architecture: amd64" >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control ;  \
	elif [ -n `echo ${host_cpu} | grep 86 ` ] ; then /bin/echo "Architecture: i386" >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control; \
	else /bin/echo "Architecture: " ${host_cpu} >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control ; fi
	/bin/echo -e "Section: devel\nPriority: optional\nDepends: libc6-dev\nSuggests: libexpat1" >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control	
	/bin/echo -n "Maintainer: " >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control
	cat $(top_srcdir)/AUTHORS | tr -d '\r\n' >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control  
	/bin/echo " <" $(PACKAGE_BUGREPORT) ">" >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control				
	/bin/echo -n "Description: " >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control
	/bin/cat $(top_srcdir)/README >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control
	/bin/echo " This package contains the dynamic library and tools." >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control
	/bin/echo "Homepage: " ${PACKAGE_URL} >>${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/control
	/bin/echo -e "#!/bin/sh\n/sbin/ldconfig\n/usr/bin/mandb" >${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/postinst && chmod 755 ${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/postinst
	/bin/echo -e "#!/bin/sh\n/sbin/ldconfig\n/usr/bin/mandb" >${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/postrm && chmod 755 ${DEB_BUILD_ROOT}/libdbgflags/DEBIAN/postrm
	for subdir in bin lib man doc ; do /bin/mkdir -p ${DEB_BUILD_ROOT}/libdbgflags${prefix}/$$subdir ; done
	for program in zo logproc ; do /bin/cp $(top_builddir)/$$program/.libs/$$program  ${DEB_BUILD_ROOT}/libdbgflags${prefix}/bin ; done
	for lib in `/bin/ls -1 $(top_builddir)/libdbgflags/.libs/lib*.so* `; do /bin/cp -a $$lib ${DEB_BUILD_ROOT}/libdbgflags${prefix}/lib ; done
	for man in `/bin/ls -1 $(top_builddir)/man/*.1 `; do /bin/cp -a $$man ${DEB_BUILD_ROOT}/libdbgflags${prefix}/man ; done && gzip -9 ${DEB_BUILD_ROOT}/libdbgflags${prefix}/man/*	
	/bin/cp $(top_srcdir)/README ${DEB_BUILD_ROOT}/libdbgflags${prefix}/doc
	/bin/cp $(top_srcdir)/ChangeLog ${DEB_BUILD_ROOT}/libdbgflags${prefix}/doc/changeLog
	/bin/cp $(top_srcdir)/COPYING ${DEB_BUILD_ROOT}/libdbgflags${prefix}/doc/copyright
	cd ${DEB_BUILD_ROOT} && fakeroot dpkg-deb --build libdbgflags && /bin/mv -f libdbgflags.deb $(abs_srcdir)

# dev Debian package
dist-deb-dev: 
	autoreconf -fvi
	make clean && make # no configure to keep the last build options
	make dist 
	/bin/rm -Rf ${DEB_BUILD_ROOT}/libdbgflags-dev 2>/dev/null
	/bin/mkdir -p ${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN
	/bin/echo "Package: libdbgflags-dev" >${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control
	/bin/echo "Version: " ${PACKAGE_VERSION} >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control
	if [ -n `echo ${host_cpu} | grep 64 ` ] ; then /bin/echo "Architecture: amd64" >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control ;  \
	elif [ -n `echo ${host_cpu} | grep 86 ` ] ; then /bin/echo "Architecture: i386" >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control; \
	else /bin/echo "Architecture: " ${host_cpu} >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control ; fi
	/bin/echo -e "Section: libdevel\nPriority: optional\nDepends: libc6-dev\nSuggests: libexpat1" >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control
	/bin/echo -n "Maintainer: " >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control
	cat $(top_srcdir)/AUTHORS | tr -d '\r\n' >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control  
	/bin/echo " <" $(PACKAGE_BUGREPORT) ">" >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control				
	/bin/echo -n "Description: " >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control
	/bin/cat $(top_srcdir)/README >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control
	/bin/echo " This package contains the header files, static and dynamic libraries and tools." >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control
	/bin/echo "Homepage: " ${PACKAGE_URL} >>${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/control
	/bin/echo -e "#!/bin/sh\n/sbin/ldconfig\n/usr/bin/mandb" >${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/postinst && chmod 755 ${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/postinst
	/bin/echo -e "#!/bin/sh\n/sbin/ldconfig\n/usr/bin/mandb" >${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/postrm && chmod 755 ${DEB_BUILD_ROOT}/libdbgflags-dev/DEBIAN/postrm
	for subdir in bin lib man doc include ; do /bin/mkdir -p ${DEB_BUILD_ROOT}/libdbgflags-dev${prefix}/$$subdir ; done
	for program in zo logproc ; do /bin/cp $(top_builddir)/$$program/.libs/$$program  ${DEB_BUILD_ROOT}/libdbgflags-dev${prefix}/bin ; done
	for lib in `/bin/ls -1 $(top_builddir)/libdbgflags/.libs/lib*.so* $(top_builddir)/libdbgflags/.libs/lib*.la? `; do /bin/cp -a $$lib ${DEB_BUILD_ROOT}/libdbgflags-dev${prefix}/lib ; done
	for man in `/bin/ls -1 $(top_builddir)/man/*.1 $(top_builddir)/man/*.3 `; do /bin/cp -va $$man ${DEB_BUILD_ROOT}/libdbgflags-dev${prefix}/man ; done && gzip -9 ${DEB_BUILD_ROOT}/libdbgflags-dev${prefix}/man/*
	/bin/cp -aR $(top_srcdir)/include ${DEB_BUILD_ROOT}/libdbgflags-dev${prefix}
	/bin/cp $(top_srcdir)/README ${DEB_BUILD_ROOT}/libdbgflags-dev${prefix}/doc
	/bin/cp $(top_srcdir)/ChangeLog ${DEB_BUILD_ROOT}/libdbgflags-dev${prefix}/doc/changeLog
	/bin/cp $(top_srcdir)/COPYING ${DEB_BUILD_ROOT}/libdbgflags-dev${prefix}/doc/copyright
	cd ${DEB_BUILD_ROOT} && fakeroot dpkg-deb --build libdbgflags-dev && /bin/mv -f libdbgflags-dev.deb $(abs_srcdir)

# source Debian package	
dist-deb-src: 
	autoreconf -fvi		
	make dist 
	/bin/rm -Rf ${DEB_BUILD_ROOT}/libdbgflags-src 2>/dev/null
	/bin/mkdir -p ${DEB_BUILD_ROOT}/libdbgflags-src/DEBIAN
	/bin/echo "Package: libdbgflags-src" >${DEB_BUILD_ROOT}/libdbgflags-src/DEBIAN/control
	/bin/echo "Version: " ${PACKAGE_VERSION} >>${DEB_BUILD_ROOT}/libdbgflags-src/DEBIAN/control
	/bin/echo -e "Section: libdevel\nPriority: optional\nArchitecture: all\nDepends: libc6-dev, autotools-dev (>=20100122.1), libtool (>=2.2.6), libltdl-dev (>=2.2.6), automake (>=1.11), autoconf (>=2.67) \nSuggests: libexpat1 " >>${DEB_BUILD_ROOT}/libdbgflags-src/DEBIAN/control
	/bin/echo -n "Maintainer: " >>${DEB_BUILD_ROOT}/libdbgflags-src/DEBIAN/control
	cat $(top_srcdir)/AUTHORS | tr -d '\r\n' >>${DEB_BUILD_ROOT}/libdbgflags-src/DEBIAN/control  
	/bin/echo " <" $(PACKAGE_BUGREPORT) ">" >>${DEB_BUILD_ROOT}/libdbgflags-src/DEBIAN/control				
	/bin/echo -n "Description: " >>${DEB_BUILD_ROOT}/libdbgflags-src/DEBIAN/control
	/bin/cat $(top_srcdir)/README >>${DEB_BUILD_ROOT}/libdbgflags-src/DEBIAN/control
	/bin/echo " This package contains the source code of the dbgflags libraries and tools." >>${DEB_BUILD_ROOT}/libdbgflags-src/DEBIAN/control
	/bin/echo "Homepage: " ${PACKAGE_URL} >>${DEB_BUILD_ROOT}/libdbgflags-src/DEBIAN/control
	/bin/mkdir -p ${DEB_BUILD_ROOT}/libdbgflags-src${prefix}/src ${DEB_BUILD_ROOT}/libdbgflags-src${prefix}/doc
	/bin/cp $(top_srcdir)/${PACKAGE_TARNAME}-${PACKAGE_VERSION}.tar.bz2 ${DEB_BUILD_ROOT}/libdbgflags-src${prefix}/src	
	/bin/cp $(top_srcdir)/README ${DEB_BUILD_ROOT}/libdbgflags-src${prefix}/doc
	/bin/cp $(top_srcdir)/ChangeLog ${DEB_BUILD_ROOT}/libdbgflags-src${prefix}/doc/changeLog
	/bin/cp $(top_srcdir)/COPYING ${DEB_BUILD_ROOT}/libdbgflags-src${prefix}/doc/copyright
	cd ${DEB_BUILD_ROOT} && fakeroot dpkg-deb --build libdbgflags-src && /bin/mv -f libdbgflags-src.deb $(abs_srcdir)
	