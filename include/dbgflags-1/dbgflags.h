/***************************************************************************
                          dbgflags.h  -  description
                             -------------------
    begin                : Sun Jan 10 2010
    copyright            : (C) 2010 by OC

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This file is part of the dbflags Library.                             *
 *                                                                         *
 *   The dbflags Library is free software; you can redistribute it and/or  *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   The dbflags Library is distributed in the hope that it will be useful,*
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with the dbflags Library; if not, write to the Free     *
 *   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA     *
 *   02111-1307 USA.                                                       *
 *                                                                         *
 ***************************************************************************/


#ifndef _DEBUGFLAGS_H_
#define _DEBUGFLAGS_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_DEBUGFLAGS_NAME_SIZE 32
#define FLAGS_SIZE 32 /*(sizeof(unsigned int))*/

/* interface v2.0 */
#define MAX_CUSTOM_CMD_AND_PARAMS  127

/* interface v2.0 */
typedef void (*customHelpCommand)(FILE *stream);
typedef int (*customCommandsHandler)(int argc, char *argv[],FILE *stream);
typedef struct _DebugFlagsCustomCommands {
    customHelpCommand customHelpCmd;    /* MUST BE set to NULL when not set */
    customCommandsHandler customCmdHandler; /* MUST BE set to NULL when not set */
} DebugFlagsCustomCommands;

/* interface v1.0 */
typedef struct _DebugFlags {
	char moduleName[MAX_DEBUGFLAGS_NAME_SIZE];
	char flagsNames[FLAGS_SIZE][MAX_DEBUGFLAGS_NAME_SIZE];
	unsigned int mask;        
        DebugFlagsCustomCommands customCommands; /* interface v2.0 */
} DebugFlags;

int registerDebugFlags(const DebugFlags *dbgFlags);
int registerLibraryDebugFlags(const DebugFlags *dbgFlags);
int unregisterDebugFlags(const DebugFlags *dbgFlags);


#define 	FLAG_ID_0	0		
#define 	FLAG_ID_1	1		
#define 	FLAG_ID_2	2		
#define 	FLAG_ID_3	3		
#define 	FLAG_ID_4	4		
#define 	FLAG_ID_5	5		
#define 	FLAG_ID_6	6		
#define 	FLAG_ID_7	7		
#define 	FLAG_ID_8	8		
#define 	FLAG_ID_9	9		
#define 	FLAG_ID_10	10		
#define 	FLAG_ID_11	11		
#define 	FLAG_ID_12	12		
#define 	FLAG_ID_13	13		
#define 	FLAG_ID_14	14		
#define 	FLAG_ID_15	15		
#define 	FLAG_ID_16	16		
#define 	FLAG_ID_17	17		
#define 	FLAG_ID_18	18		
#define 	FLAG_ID_19	19		
#define 	FLAG_ID_20	20		
#define 	FLAG_ID_21	21		
#define 	FLAG_ID_22	22		
#define 	FLAG_ID_23	23		
#define 	FLAG_ID_24	24		
#define 	FLAG_ID_25	25		
#define 	FLAG_ID_26	26		
#define 	FLAG_ID_27	27		
#define 	FLAG_ID_28	28		
#define 	FLAG_ID_29	29		
#define 	FLAG_ID_30	30		
#define 	FLAG_ID_31	31		

#define DEBUGFLAG(n)  (1<<n)

#define 	FLAG_0	DEBUGFLAG(FLAG_ID_0)
#define 	FLAG_1	DEBUGFLAG(FLAG_ID_1)
#define 	FLAG_2	DEBUGFLAG(FLAG_ID_2)
#define 	FLAG_3	DEBUGFLAG(FLAG_ID_3)
#define 	FLAG_4	DEBUGFLAG(FLAG_ID_4)
#define 	FLAG_5	DEBUGFLAG(FLAG_ID_5)
#define 	FLAG_6	DEBUGFLAG(FLAG_ID_6)
#define 	FLAG_7	DEBUGFLAG(FLAG_ID_7)
#define 	FLAG_8	DEBUGFLAG(FLAG_ID_8)
#define 	FLAG_9	DEBUGFLAG(FLAG_ID_9)
#define 	FLAG_10	DEBUGFLAG(FLAG_ID_10)
#define 	FLAG_11	DEBUGFLAG(FLAG_ID_11)
#define 	FLAG_12	DEBUGFLAG(FLAG_ID_12)
#define 	FLAG_13	DEBUGFLAG(FLAG_ID_13)
#define 	FLAG_14	DEBUGFLAG(FLAG_ID_14)
#define 	FLAG_15	DEBUGFLAG(FLAG_ID_15)
#define 	FLAG_16	DEBUGFLAG(FLAG_ID_16)
#define 	FLAG_17	DEBUGFLAG(FLAG_ID_17)
#define 	FLAG_18	DEBUGFLAG(FLAG_ID_18)
#define 	FLAG_19	DEBUGFLAG(FLAG_ID_19)
#define 	FLAG_20	DEBUGFLAG(FLAG_ID_20)
#define 	FLAG_21	DEBUGFLAG(FLAG_ID_21)
#define 	FLAG_22	DEBUGFLAG(FLAG_ID_22)
#define 	FLAG_23	DEBUGFLAG(FLAG_ID_23)
#define 	FLAG_24	DEBUGFLAG(FLAG_ID_24)
#define 	FLAG_25	DEBUGFLAG(FLAG_ID_25)
#define 	FLAG_26	DEBUGFLAG(FLAG_ID_26)
#define 	FLAG_27	DEBUGFLAG(FLAG_ID_27)
#define 	FLAG_28	DEBUGFLAG(FLAG_ID_28)
#define 	FLAG_29	DEBUGFLAG(FLAG_ID_29)
#define 	FLAG_30	DEBUGFLAG(FLAG_ID_30)
#define 	FLAG_31	DEBUGFLAG(FLAG_ID_31)

#ifdef __cplusplus
}

class DebugFlagsMgr {
  DebugFlags *dbgflag;
public:
   DebugFlagsMgr(DebugFlags &debugFlag):dbgflag(&debugFlag) {
      registerDebugFlags(dbgflag);
   }
  ~DebugFlagsMgr() {
     unregisterDebugFlags(dbgflag);
  }
};
#if 0
class CDebugFlagsMgr : public _DebugFlags {
  
public:
   CDebugFlagsMgr( char moduleName[MAX_DEBUGFLAGS_NAME_SIZE],
                    char flagsNames[FLAGS_SIZE][MAX_DEBUGFLAGS_NAME_SIZE],
                    unsigned int mask,
                    DebugFlagsCustomCommands customCommands)
                    :_DebugFlags(moduleName,flagsNames,mask,customCommands) {
      registerDebugFlags(this);
   }
  ~CDebugFlagsMgr() {
     unregisterDebugFlags(this);
  }
};
#endif

#endif

#endif /*__DEBUGFLAGS_H_*/
