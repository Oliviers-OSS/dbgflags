/***************************************************************************
                          utils.h  -  description
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
#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>

int getFullProcessName(const pid_t pid,char *processName);
int getProcessName(const pid_t pid,char *processName);
int getCurrentFullProcessName(char *processName);
int getCurrentProcessName(char *processName);
int getcmdLine(char *cmdLine);

#ifdef __cplusplus
}
#endif

#endif /* _UTILS_H_ */

