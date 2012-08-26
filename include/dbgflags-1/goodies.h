/* 
 * File:   goodies.h
 * Author: oc
 *
 * Created on July 2, 2011, 7:33 PM
 */

#ifndef _GOODIES_H
#define	_GOODIES_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <dbgflags/version.h>

int stringToSyslogLevel(const char *syslogLevel);
int stringToFacility(const char *facilityString);
time_t parseDuration(const char *string);
size_t parseSize(const char *string);
#if DBGFLAGS_INTERFACE_VERSION >= 12
unsigned int parseFlagsOptions(const char *flagsOptions); //WARNING: argument is an array of strings ended by an empty one (terminated by two null characters)
#endif /* DBGFLAGS_INTERFACE_VERSION >= 12 */

#ifdef	__cplusplus
} /* extern "C" */
#endif

#ifdef	__cplusplus

#if DBGFLAGS_INTERFACE_VERSION >= 12
#include <vector>
#include <string>
#include <cstring>
#include <cassert>

static inline unsigned int parseFlagsOptions(const std::vector<std::string> &flagsOptions) {
  unsigned int flags(0);
  char options[256];
  char *iterator(options);
  std::vector<std::string>::const_iterator i(flagsOptions.begin());
  const std::vector<std::string>::const_iterator end(flagsOptions.end());
  
  // copy the content of the C++ vector to a basic C string array
  memset(options,0,sizeof(options)/sizeof(options[0]));
  while(i != end) {
	strcpy(iterator,(*i).c_str());
	iterator += (*i).length() + 1;
	assert(iterator < (options + sizeof(options)));
	++i;
  }
  flags = parseFlagsOptions(options);
  
  return flags;
}
#endif /* DBGFLAGS_INTERFACE_VERSION >= 12 */

} /* extern "C" */
#endif /* __cplusplus */

#endif	/* _GOODIES_H */

