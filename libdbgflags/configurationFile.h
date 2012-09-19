/* 
 * File:   configurationFile.h
 * Author: oc
 *
 * Created on August 3, 2011, 7:24 PM
 */

#ifndef _CONFIGURATIONFILE_H_
#define	_CONFIGURATIONFILE_H_

#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "configurationFileData.h"

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef MAX_LINE_SIZE
#define MAX_LINE_SIZE 128
#endif
#ifndef EOL
#define EOL "\n"
#endif

static inline const char*  get_key_value(const linked_list_element *element) {
    const char *value = NULL;
    if (element->value[0] != '$') {
        value = element->value;
    } else {
        value = getenv(element->value+1); /* +1 to skip the $ character */
        if (NULL == value) {
            WARNING_MSG("environment variable %s not found",element->value);
            value = element->value;
        }
    }
    return value;
}

static inline const char*  configuration_file_get_value(configuration_file_data *data,const char *section, const char *key, const char *default_value) {
    const char *value = default_value;
    if (NULL == section) {
        linked_list_element *element = linked_list_element_find(data->elements,key);
        if (element != NULL) {
            value = get_key_value(element);
            INFO_MSG("/%s = %s",key,value);
        } else {
            NOTICE_MSG("%s key not found in root section",key);
        }
    } else {
        configuration_file_data *section_data = configuration_file_data_find(data->next,section);
        if (section_data != NULL) {
            linked_list_element *element = linked_list_element_find(section_data->elements,key);
            if (element != NULL) {
                value = get_key_value(element);
                INFO_MSG("%s/%s = %s",section,key,value);
            } else {
                NOTICE_MSG("%s key not found in section %s",key,section);
            }
        } else {
            NOTICE_MSG("section %s not found",section);
        }
    }
    return value;
}

typedef int (*configuration_file_comparator)(const char *section, const char *key,const char *value);

typedef struct configuration_file_iterator_ {
  configuration_file_comparator comparator;
  linked_list_element *current_position;
} configuration_file_iterator;

#define ITERATOR_POSITION_NOT_SET  (void*)-1

static inline void configuration_file_set_comparator(configuration_file_comparator comparator,configuration_file_iterator *iter) {
	iter->comparator = comparator;
	iter->current_position = ITERATOR_POSITION_NOT_SET;
}

static inline int configuration_file_iterator_has_next(configuration_file_iterator *iter) {
	return (iter->current_position != NULL);
}

static inline int configuration_file_find_value(const configuration_file_data *data,configuration_file_iterator *iter,const char *section, const char**key) {
  int error = ENOENT;
  const char *value = NULL;
  *key = NULL;
  
  if (iter->current_position != NULL) {
	  if (NULL == section) {
		if (ITERATOR_POSITION_NOT_SET == iter->current_position) {
			iter->current_position = data->elements;	
		}
		while (iter->current_position != NULL) {
		  value = get_key_value(iter->current_position);
		  if (iter->comparator(section,iter->current_position->key,value) != 0) {
			iter->current_position = iter->current_position->next;
		  } else {
			*key = iter->current_position->key;
			iter->current_position = iter->current_position->next;
			error = EXIT_SUCCESS;
			break;
		  }	
		} /* while (iter->current_position != NULL) */
	  } else {
		configuration_file_data *section_data = configuration_file_data_find(data->next,section);
		if (section_data != NULL) {
			if (ITERATOR_POSITION_NOT_SET == iter->current_position) {
				iter->current_position = section_data->elements;
			}
			while (iter->current_position != NULL) {
				value = get_key_value(iter->current_position);
				if (iter->comparator(section,iter->current_position->key,value) == 0) {
					iter->current_position = iter->current_position->next;
				} else {
					*key = iter->current_position->key;
					iter->current_position = iter->current_position->next;
					error = EXIT_SUCCESS;
					break;
				}
			} /* while (iter->current_position != NULL) */
		} /* (section_data != NULL) */
	  } /* (NULL == section) */
  } /* (iter->current_position != NULL) */
  return error;
}

static inline void filterCharacters(char *line) {
  register char *readCursor = line;
  register char *writeCursor = line;
  while(*readCursor != '\0') {
    switch(*readCursor) {
      case '\r':
        /* remove \r characters */
        break;
      default:
	    if (writeCursor != readCursor) {
			*writeCursor = *readCursor;
		}
	writeCursor++;
    } /* switch(*readCursor) */
    readCursor++;
  } /* while(*readCursor != '\0') */
  *writeCursor = '\0';
}

#define INC_READ_START(read_start) read_start = eolPos + strlen(EOL); \
                                              if ('\0' == *read_start) { \
                                                    read_start = buffer; \
                                               }

static inline int readLine(int fd,char **line,size_t *used_size) {
    int error = EXIT_SUCCESS;
    static pthread_mutex_t read_mutex = PTHREAD_MUTEX_INITIALIZER;
    static char buffer[MAX_LINE_SIZE*2];
    const char *buffer_write_limit = buffer + MAX_LINE_SIZE;
    static unsigned int buffer_initialized = 0;
    static char *read_start = buffer;
    static char *write_start = buffer;
    //static unsigned int eof_reached = 0;

    error = pthread_mutex_lock(&read_mutex);
    if (0 == error) {
        if (unlikely(0 == buffer_initialized)) {
            memset(buffer,0,sizeof(buffer));
            buffer_initialized = 1;
        }

        /* check if the current buffer already contains a line */
        char *eolPos = strstr(read_start,EOL);
        if (eolPos != NULL) {
            *eolPos = '\0';
            *line = read_start;
            INC_READ_START(read_start);
        } else {
            /* if not, read the file to get one */
            if (write_start > buffer_write_limit) {
                const size_t remaining_bytes = strlen(read_start);
                memmove(buffer,read_start,remaining_bytes);
                read_start = buffer;
                write_start = buffer+remaining_bytes;
            }
            const ssize_t nb_read = read(fd,write_start,MAX_LINE_SIZE);
            if (nb_read > 0) {
                *line = read_start;
                write_start[nb_read] = '\0';
                char *eolPos = strstr(read_start,EOL);
                if (eolPos != NULL) {
                    const size_t n = eolPos - read_start;
                    *used_size = n;
                    *eolPos = '\0';
                    write_start += nb_read;
                    INC_READ_START(read_start);
                } else {
                    if (MAX_LINE_SIZE == nb_read) {
                        WARNING_MSG("line is longer than %d characters",MAX_LINE_SIZE);
                    }
                    *used_size = nb_read;
                } /* !(eolPos != NULL)*/
            } else if (0 == nb_read ) { /* EOF reached */
                /* look for remaining bytes (if the last line is not empty and contains the EOF and no the EOL */
                const size_t n = strlen(read_start);
                *line = read_start;
                *used_size = n;
                read_start += n;
                //*read_start = '\0';
            } else {
                error = errno;
                ERROR_MSG("read error %d (%m)",error);
                *used_size = 0;
            }
        } /* !(eolPos != NULL) */
		
	filterCharacters(*line);
        const int unlock_error = pthread_mutex_unlock(&read_mutex);
        if (unlock_error != 0) {
            ERROR_MSG("pthread_mutex_unlock error %d",unlock_error);
            if (EXIT_SUCCESS == error) {
                error = unlock_error;
            }
        }
    } else {
        ERROR_MSG("pthread_mutex_lock error %d",error);
    }
    return  error;
}

static inline void removeTrailingCharacters(char *stringsEnd) {
    if (stringsEnd != NULL) {
        while(isspace(*stringsEnd)) {
            *stringsEnd = '\0';
            stringsEnd--;
        }
    }
}

static inline unsigned char isValidCharacter(const char character,const unsigned int rvalue) {
    unsigned int allowed = 0;
    if (isalnum(character)) {
        allowed = 1;
    }  else {
        switch(character) {
            case '_':
                /* break is missing */
            case '/':
            case '-':
            case '&':
            case '@':
                allowed = 1;
                break;
            default:
                /* right value only */
                if (rvalue) {
                    allowed = (' ' == character) || ('\t' == character) || ('$' == character);
                }
        } /* switch(character) */
    } /* !(isalnum(character)) */
    return allowed;
}

static inline unsigned char nextCharacterIsNotValid(const char character,const unsigned int rvalue) {
    unsigned int invalid = 1;
    switch(character) {        
        case ' ':
            /* break is missing */
        case '\t':
            /* break is missing */
        case '#':
            /* break is missing */
        case ';':
            /* break is missing */
        case '\n':
            /* break is missing */
        case '\r':
	  /* break is missing */
	case '\0':	  
            invalid = 0;
            break;
            /* left only */
        case '=':
            /* break is missing */
        case ':':
            invalid = (rvalue);
    } /* switch(character)*/
    return invalid;
}

static inline int readConfigurationFile(const char *filename, configuration_file_data *data) {
    int error = EXIT_SUCCESS;
    int fd = open(filename,O_RDONLY);
    if (fd != -1) {

        char *section = NULL;
		char session_buffer[MAX_SECTION];
        char *line = NULL;
        size_t size = 0;
        unsigned int line_number = 0;
        
        while (( (error = readLine(fd,&line,&size)) == EXIT_SUCCESS) && (size >0)) {

            register char *cursor = line;
            char *key = NULL;
            char *value = NULL;
            unsigned int rvalue = 0;
            
            DEBUG_VAR(line,"%s");
            ++line_number;
            while(*cursor != 0) {
                switch(*cursor) {
                    /* comments */
                    case '#':
                      /* break is missing */
                    case ';':
                        *cursor = '\0'; /* to end the processing of this line */
                        break;
					case '\t':
						/* break is missing */
                    case ' ':
                        *cursor = '\0';
                        cursor++;
                        break;
                    case '[':
                        if (!rvalue) {
                        cursor++;
                        section = cursor;
                        while(isalnum(*cursor)) {
                            cursor++;
                        }
                        } else {
                            ERROR_MSG("error on line %d in configuration file %s, character [ is only allowed to start a section declaration",line,filename);
                            *cursor = '\0'; /* to move to the next line */
                            key = NULL; /* to cancel the current line's data if any */
                        }
                        break;
                    case ']':
                        *cursor = '\0';
                        if ('\0' == section[0]) {
                            section = NULL;
                        } else {
							strcpy(session_buffer,section);
							section = session_buffer;
						}
                        cursor++;
                        break;
                    case ':':
                        /* break is missing */
                    case '=':
                        rvalue = 1;
                        *cursor = '\0';
                        cursor++;
                        break;
					case '\r': /* must have been filtered by the readLine function if any, but just in case... */
						cursor++; /* ignore it */
						break;
                    default:
                        if (rvalue) {
                            value = cursor;
                        } else {
                            key = cursor;
                        }
                        while(isValidCharacter(*cursor,rvalue)) {
                            cursor++;
                        }
                        /* remove trailing spaces if any */
                        if (rvalue) {
                            char *valuesEnd = cursor-1;
                            removeTrailingCharacters(valuesEnd);
                        }
                        /* catch invalid characters */
                        if (nextCharacterIsNotValid(*cursor,rvalue)) {
                            ERROR_MSG("error on line %d in configuration file %s: character %c (%d) is not allowed in this context",line_number,filename,isprint(*cursor)?(*cursor):'?',*cursor);
                            *cursor = '\0'; /* to move to the next line */
                            key = NULL; /* to cancel the current line's data if any */
                        }                                               
                } /* switch(*cursor) */
            } /* while(*cursor != 0) */			
            DEBUG_VAR(section,"%s");
            DEBUG_VAR(key,"%s");
            DEBUG_VAR(value,"%s");		
            if ( (key != NULL) && (value != NULL)) {			
                configuration_file_data_add(data,section,key,value);
            }
        } /* while (( (error = readLine(fd,&line,&size)) == EXIT_SUCCESS) && (size >0)) */
        close(fd);
        fd = -1;
    } else {
        error = errno;
        ERROR_MSG("error open file %s error %d (%m)",filename,error);
    }

    return error;
}

#ifdef	__cplusplus
}
#endif

#endif	/* _CONFIGURATIONFILE_H_ */

