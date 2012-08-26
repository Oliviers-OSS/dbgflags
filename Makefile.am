AUTOMAKE_OPTIONS = foreign
SUBDIRS = libdbgflags zo logproc
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

