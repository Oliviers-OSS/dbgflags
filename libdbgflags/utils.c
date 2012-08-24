/***************************************************************************
                          utils.c  -  description
                             -------------------
    begin                : Fri May 14 2010
    copyright            : (C) 2010 by OC

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <dbgflags/dbgflags.h>
#include "debug.h"

int getFullProcessName(const pid_t pid,char *processName) {
  int error = EXIT_SUCCESS;
  char procEntryName[PATH_MAX];
  int fd = -1;
  
  sprintf(procEntryName,"/proc/%u/cmdline",pid);
  fd = open(procEntryName,O_RDONLY);
  if (fd != -1) {
    char procName[PATH_MAX];
    const ssize_t n = read(fd,procName,sizeof(procName));
    if (n != -1) {
       procName[n] = '\0';
       strcpy(processName,procName);
    } else {
      error = errno;
      //fprintf(stderr,"%s: error reading file %s (%m)\n",__FUNCTION__,procEntryName,error);
      ERROR_MSG("error reading file %s (%m)",procEntryName,error);
    }
    close(fd);
    fd = -1;
  } else {
    error = errno;
    //fprintf(stderr,"%s: error opening file %s (%m)\n",__FUNCTION__,procEntryName,error);
    ERROR_MSG("error opening file %s (%m)",procEntryName,error);
  }
      
  return error;
}

int getProcessName(const pid_t pid,char *processName) {
  char fullProcessName[PATH_MAX];
  int error = getFullProcessName(pid,fullProcessName);
  if (EXIT_SUCCESS == error) {
    const char *start = strrchr(fullProcessName,'/');
    if (start != NULL) {
      ++start;
    } else {
      start = fullProcessName;
    }
    strcpy(processName,start);
  } /* error already printed */
  
  return error;
}

int getCurrentFullProcessName(char *processName) {
   const pid_t pid = getpid();
   return getFullProcessName(pid,processName);
}

int getCurrentProcessName(char *processName) {
  const pid_t pid = getpid();
  return getProcessName(pid,processName);
}
  
int getcmdLine(char *cmdLine) {
  int error = EXIT_SUCCESS;
  char procEntryName[PATH_MAX];
  const pid_t pid = getpid();
  int fd = -1;
  
  sprintf(procEntryName,"/proc/%u/cmdline",pid);
  fd = open(procEntryName,O_RDONLY);
  if (fd != -1) {
    const ssize_t n = read(fd,cmdLine,ARG_MAX);
    if (n != -1) {
       cmdLine[n] = '\0';       
    } else {
      error = errno;
      //fprintf(stderr,"%s: error reading file %s (%m)\n",__FUNCTION__,procEntryName,error);
      ERROR_MSG("error reading file %s (%m)\n",procEntryName,error);
    }
    close(fd);
    fd = -1;
  } else {
    error = errno;
    fprintf(stderr,"%s: error opening file %s (%m)\n",__FUNCTION__,procEntryName,error);
    ERROR_MSG("error opening file %s (%m)\n",procEntryName,error);
  }

  return error;
}
