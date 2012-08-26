/* 
 * File:   configurationFileData.h
 * Author: oc
 *
 * Created on August 3, 2011, 7:26 PM
 */

#ifndef _CONFIGURATIONFILEDATA_H_
#define	_CONFIGURATIONFILEDATA_H_

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <linux/limits.h>
#include <dbgflags/debug_macros.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef MAX_KEY
#define MAX_KEY   20
#endif 
#ifndef MAX_VALUE 
#define MAX_VALUE PATH_MAX
#endif
#ifndef MAX_SECTION
#define MAX_SECTION 20
#endif 

typedef struct linked_list_element_ {
    char key[MAX_KEY];
    char value[MAX_VALUE];
    struct linked_list_element_ *next;
} linked_list_element;

typedef linked_list_element* linked_list;

static inline linked_list_element* linked_list_element_find(linked_list elements,const char *key) {
    linked_list_element *e = elements;
    while (e != NULL) {
        if (strcasecmp(e->key,key) != 0) {
            e = e->next;
        } else {
            break;
        }
    }
    return e;
}

static inline int  linked_list_element_add(linked_list *elements,const char *key,const char *value) {
    int error = EXIT_SUCCESS;
    linked_list_element *element = linked_list_element_find(*elements,key);
    if (NULL == element) {
        linked_list new = (linked_list) malloc(sizeof(linked_list_element));
        if (new != NULL) {
           linked_list first = *elements;
           strncpy (new->key,key,MAX_KEY);
           new->key[MAX_KEY-1] = '\0';
           strncpy (new->value,value,MAX_VALUE);
           new->value[MAX_VALUE-1]= '\0';
           new->next = first;
           *elements = new;
        } else {
            error = ENOMEM;
            ERROR_MSG("failed to allocate %d bytes for a new linked_list_element",sizeof(linked_list_element));
        }
    } else { !/* (NULL == e) */
       strncpy (element->value,value,MAX_VALUE);
       element->value[MAX_VALUE-1]= '\0';
    }

    return error;
}

static void linked_list_element_free(linked_list elements) {
    while(elements != NULL) {
        linked_list next = elements->next;
        free(elements);
        elements = next;
    }
}


typedef struct configuration_file_data_ {
    char section[MAX_SECTION];
    linked_list elements;
    struct configuration_file_data_ *next;
} configuration_file_data;

static inline configuration_file_data* configuration_file_data_find(configuration_file_data *data,const char *section) {
    configuration_file_data *element = data;
    while(element != NULL) {
        if (strcasecmp(element->section,section) != 0) {
            element = element->next;
        } else {
            break;
        }
    }
    return element;
}

static inline int configuration_file_data_init(configuration_file_data *data) {
    data->section[0] = '\0';
    data->elements = NULL;
    data->next = NULL;
}

static inline int configuration_file_data_add(configuration_file_data *data,const char *section, const char *key, const char *value) {
    int error = EXIT_SUCCESS;
    if (NULL == section) {
        error = linked_list_element_add(&data->elements,key,value);
    } else {
        configuration_file_data *element = configuration_file_data_find(data->next,section);
        if (element != NULL) {
            linked_list_element_add(&element->elements,key,value);
        } else {
            configuration_file_data *new = (configuration_file_data *) malloc(sizeof(configuration_file_data));
            if (new != NULL) {
                strncpy(new->section,section,MAX_SECTION);
                new->elements = NULL;
                error = linked_list_element_add(&new->elements,key,value);
                if (EXIT_SUCCESS == error) {
                    data->next = new;
                }
            } else {
                error = ENOMEM;
                ERROR_MSG("failed to allocate %d bytes for a new configuration_file_data eleemnt",sizeof(configuration_file_data));
            }
        } /* !(element != NULL) */
    } /* !(NULL == section) */
    return error;
}

static inline void configuration_file_data_free(configuration_file_data *data) {
    if (data != NULL) {
        configuration_file_data *element = data->next;
        linked_list_element_free(data->elements);
        data->elements = NULL;
        while(element != NULL) {
            configuration_file_data *next = element->next;
            linked_list_element_free(element->elements);
            element->elements = NULL;
            free(element);
            element = next;
        } /* while(element != NULL) */
    } /* (data != NULL) */
}

#ifdef	__cplusplus
}
#endif

#endif	/* _CONFIGURATIONFILEDATA_H_ */

