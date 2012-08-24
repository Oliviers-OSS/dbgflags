#ifndef _LIBRARY_DEBUG_FLAGS_ENTRY_H_
#define _LIBRARY_DEBUG_FLAGS_ENTRY_H_

struct LibraryDebugFlagsEntry_;

typedef struct LibraryDebugFlagsEntry_ {  
    DebugFlags *library;
    struct LibraryDebugFlagsEntry_ *previous;
    struct LibraryDebugFlagsEntry_ *next;    
} LibraryDebugFlagsEntry;

static __inline LibraryDebugFlagsEntry *LibraryDebugFlagsEntryFind(LibraryDebugFlagsEntry *entries,const char* name) {
    LibraryDebugFlagsEntry *cursor = entries;

    while(cursor != NULL) {
        const DebugFlags *dbgFlags = cursor->library;
        if (dbgFlags != NULL) {
            if (strcmp(dbgFlags->moduleName,name) == 0) {
                return cursor;
            } else {
                cursor = cursor->next;
            }
        } else {
            ERROR_MSG("list is corrupted !");
        }
    } /* while(cursor != NULL) */
    return NULL; /* not found */
}

static __inline int LibraryDebugFlagsEntryAdd(LibraryDebugFlagsEntry **entries,DebugFlags *library) {
    int error = EXIT_SUCCESS;
    LibraryDebugFlagsEntry *item = (LibraryDebugFlagsEntry *) malloc(sizeof(LibraryDebugFlagsEntry));
    if (item != NULL) {
        item->library = library;
        item->previous = NULL;
        item->next = *entries;
        if (*entries != NULL) {
            (*entries)->previous = item;
        } 
        *entries = item;
    } else {
        error = ENOMEM;
        ERROR_MSG("failed to allocated %d bytes for a new LibraryDebugFlagsEntry",sizeof(LibraryDebugFlagsEntry));
    }
    return error;
}

static __inline int LibraryDebugFlagsEntryRemoveByName(LibraryDebugFlagsEntry **entries,const char *name) {
    int error = ENOENT;
    LibraryDebugFlagsEntry *cursor = *entries;

    while(cursor != NULL) {
        const DebugFlags *dbgFlags = cursor->library;
        if (dbgFlags != NULL) {
            if (strcmp(dbgFlags->moduleName,name) == 0) {
                /* remove it */
                if (cursor->previous != NULL) {
                    cursor->previous->next = cursor->next;
                }
                if (cursor->next != NULL) {
                    cursor->next->previous = cursor->previous;
                }
                if (cursor == *entries) {
                    *entries = cursor->next;
                }
                free(cursor);
                cursor = NULL;
                error = EXIT_SUCCESS;
            } else {
                cursor = cursor->next;
            }
        } else {
            cursor = cursor->next;
        }
    } /* while(cursor != NULL) */

    return error;
}

static __inline int LibraryDebugFlagsEntryRemove(LibraryDebugFlagsEntry **entries,const DebugFlags *library) {
    int error = ENOENT;
    LibraryDebugFlagsEntry *cursor = *entries;

    while(cursor != NULL) {
        if (library != cursor->library) {
            cursor = cursor->next;
        } else {
            if (cursor->previous != NULL) {
                cursor->previous->next = cursor->next;
            }
            if (cursor->next != NULL) {
                cursor->next->previous = cursor->previous;
            }
            if (cursor == *entries) {
                *entries = cursor->next;
            }
            free(cursor);
            cursor = NULL;
            error = EXIT_SUCCESS;
        }
    }

    return error;
}

static __inline void LibraryDebugFlagsEntryClear(LibraryDebugFlagsEntry *entries) {
    LibraryDebugFlagsEntry *cursor = entries;
    while(cursor != NULL) {
        LibraryDebugFlagsEntry *item = cursor;
        cursor = item->next;
        free(item);
        item = NULL;
    }
}

typedef int (*ForEachCmd)(const DebugFlags *library,void *userParam);

static __inline int LibraryDebugFlagsEntryForEach(LibraryDebugFlagsEntry *entries,ForEachCmd cmd, void *cmdParam) {
  int error = EXIT_SUCCESS;
  LibraryDebugFlagsEntry *cursor = entries;

  while ((cursor != NULL) && (EXIT_SUCCESS == error)) {
     if (cursor->library != NULL) {
        error = (*cmd)(cursor->library,cmdParam);
     } else {
       WARNING_MSG("NULL library reference found in LibraryDebugFlagsEntries (item 0x%X)",cursor);
     }
     cursor = cursor->next;
  }

  return error;
}

#endif /* _LIBRARY_DEBUG_FLAGS_ENTRY_H_ */


