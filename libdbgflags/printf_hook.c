/*
 * printf_hook.c
 * printf extensions to display additional types and print format
 *  Created on: 7 août 2013
 *      Author: oc
 */


#include "config.h"

#ifdef HAVE_REGISTER_PRINTF_SPECIFIER

#include <stdio.h>
#include <printf.h>
#include <pthread.h>
#include <dbgflags/dbgflags.h>
#include "debug.h"

#define BINARY_FORMAT_KEY	'b'

void printf_binary_register() __attribute__((constructor));

static inline int binary_byte_string(const unsigned char value, char *buffer) {
	static const char *stringQuartet[] = {
			"0000"
			,"0001"
			,"0010"
			,"0011"
			,"0100"
			,"0101"
			,"0110"
			,"0111"
			,"1000"
			,"1001"
			,"1010"
			,"1011"
			,"1100"
			,"1101"
			,"1110"
			,"1111"
	};

	const unsigned char high = value >> 4;
	const unsigned char low = value & 0x0F;
	const int length = sprintf (buffer, "%s%s",stringQuartet[high],stringQuartet[low]);
	return length;
}

static void printf_binary_cancel_handler(void *ptr) {
	if (ptr) {
		char **bufferPtr = (char **)ptr;
		char *buffer = *bufferPtr;
		if (buffer != NULL) {
			free(buffer);
			*bufferPtr = NULL;
		}
	}
}


/* Type of a printf specifier-handler function.
   STREAM is the FILE on which to write output.
   INFO gives information about the format specification.
   ARGS is a vector of pointers to the argument data;
   the number of pointers will be the number returned
   by the associated arginfo function for the same INFO.

   The function should return the number of characters written,
   or -1 for errors.  */

static int printf_binary (FILE *stream,const struct printf_info *info,const void *const *args) {
	char *buffer = NULL;
	register int length = 0;
	const unsigned int n = 0;

	if (BINARY_FORMAT_KEY == info->spec) {
		/*
		 * WARNING: ALWAYS read at least sizeof(unsigned long) data
		 * whatever the argument data type !!!
		 * TODO: try to find a better and less dangerous way, may be using the field width or the precision information.
		 */
		const unsigned long *value = (const unsigned long *)args[n];

		pthread_cleanup_push(printf_binary_cancel_handler, &buffer);

		/*
		 * Format the output into a string according to format specifications
		 */

		if (info->is_char) {
			/* hh flag:
			 * A  following integer conversion corresponds to a signed char or unsigned char argument,
			 * or a following n conversion corresponds to a pointer to a signed char argument.
			 */
			const size_t size = sizeof(char) * (sizeof(char)  * 8) + 1; /* 1 byte => 8 characters */
			buffer = malloc(size);
			if (buffer != NULL) {
				const unsigned char n = (*value) & 0xFF;
				length = binary_byte_string(n,buffer);
			} else {
				ERROR_MSG("failed to allocate %u bytes to printf a binary string",size);
			}
		} else if (info->is_short) {
			/* h flag:
			 * A following integer conversion corresponds to a short int or unsigned short int argument,
			 * or a following n conversion corresponds to a pointer to a short int argument.
			 */
			const size_t size = sizeof(short) * (sizeof(char)  * 8) + 1; /* 1 byte => 8 characters */
			buffer = malloc(size);
			if (buffer != NULL) {
				const unsigned short n = (*value) & 0xFFFF;
				const unsigned char *values = (const unsigned char *)&n;
				char *byteBuffer = buffer;
				length = binary_byte_string(values[1],byteBuffer);
				if (length != -1) {
					byteBuffer = buffer + length;
					length += binary_byte_string(values[0],byteBuffer);
				}
			} else {
				ERROR_MSG("failed to allocate %u bytes to printf a binary string",size);
			}
		} else if ((info->is_long_double) && (info->is_long)) {
			/* ll flag:
			 * (ell-ell).  A following integer conversion corresponds to a long long int or unsigned long long  int
			 * argument, or a following n conversion corresponds to a pointer to a long long int argument.
			 */
			const size_t size = sizeof(long long) * (sizeof(char)  * 8) + 1; /* 1 byte => 8 characters */
			buffer = malloc(size);
			if (buffer != NULL) {
				int i;
				const unsigned char *values = (const unsigned char *)value;
				for(i= (sizeof(long long) -1);((i >= 0) && (length != -1));i--) {
					char *byteBuffer = buffer + length;
					length += binary_byte_string(values[i],byteBuffer);
				}
			} else {
				ERROR_MSG("failed to allocate %u bytes to printf a binary string",size);
			}
		} else if (info->is_long_double) {
			/* L flag:
			 * A following a, A, e, E, f, F, g, or G conversion corresponds to a long double argument.
			 */
			const size_t size = sizeof(long double) * (sizeof(char)  * 8) + 1; /* 1 byte => 8 characters */
			buffer = malloc(size);
			if (buffer != NULL) {
				int i;
				const unsigned char *values = (const unsigned char *)value;
				for(i= (sizeof(long double) -1);((i >= 0) && (length != -1));i--) {
					char *byteBuffer = buffer + length;
					length += binary_byte_string(values[i],byteBuffer);
				}
			} else {
				ERROR_MSG("failed to allocate %u bytes to printf a binary string",size);
			}
		} else if (info->is_long) {
			/* l flag:
			 * (ell)  A  following integer conversion corresponds to a long int or unsigned long int argument, or a
			 * following n conversion corresponds to a pointer to a long int argument, or a following c  conversion
			 * corresponds  to  a  wint_t argument, or a following s conversion corresponds to a pointer to wchar_t
			 * argument.
			 */
			const size_t size = sizeof(long) * (sizeof(char)  * 8) + 1; /* 1 byte => 8 characters */
			buffer = malloc(size);
			if (buffer != NULL) {
				const unsigned char *values = (const unsigned char *)value;
				int i;
				for(i= (sizeof(long) -1);((i >= 0) && (length != -1));i--) {
					char *byteBuffer = buffer + length;
					length += binary_byte_string(values[i],byteBuffer);
				}
			} else {
				ERROR_MSG("failed to allocate %u bytes to printf a binary string",size);
			}
		} else {
			/*
			 * no length modifier has been set
			 * Warning: long type may be 32 or 64 bits
			 */
			const size_t size = sizeof(long) * (sizeof(char)  * 8) + 1; /* 1 byte => 8 characters */
			buffer = malloc(size);
			if (buffer != NULL) {
				int i;
				const unsigned char *values = (const unsigned char *)value;
				for(i= (sizeof(long) -1);((i >= 0) && (length != -1));i--) {
					char *byteBuffer = buffer + length;
					length += binary_byte_string(values[i],byteBuffer);
				}
			} else {
				ERROR_MSG("failed to allocate %u bytes to printf a binary string",size);
			}
		}

		/* add the "binary" prefix, pad to the minimum field width and print to the stream. */
		if (length != -1) {
			register const char *bufferStart = buffer;
			char precPadding[128];
			register char *pos = precPadding;
			if ((0 == info->width) && (-1 == info->prec)) {
				/* remove unnecessary 0 on the left */
				*pos = '\0';
				while( '0' == *bufferStart ) {
					bufferStart++;
				}
				/* just in case: keep at least the last bit value whatever it's value ! */
				if ('\0' == *bufferStart) {
					bufferStart--;
				}
			} else {
				if (info->prec > length) {
					/* add 0 to the left to get the right precision (weird cast ?) */
					while(info->prec >= length) {
						*pos = '0';
						length++;
						pos++;
					}
				}
				*pos = '\0';
				if (info->width < length) {
					/* try to remove unnecessary 0 to get the right width (without losing data !) */
					while( ('0' == *bufferStart) && (info->width < length)) {
						bufferStart++;
						length--;
					}
				} else if (info->width != 0) {
					/* add space to get the desired width */
					while(length < info->width) {
						*pos = ' ';
						length++;
						pos++;
					}
					*pos = '\0';
				}
				//length = fprintf (stream, "b%*s",(info->left ? -info->width : info->width),bufferStart);
			} /* !((0 == info->width) && (0 == info->prec))  */
			length = fprintf (stream, "b%s%s",precPadding,bufferStart);
		}

		/* Clean up and return. */
		free (buffer);
		pthread_cleanup_pop(0);
	} /* (BINARY_FORMAT_KEY == info[n].spec) */
	return length;
}

/* Type of a printf specifier-arginfo function.
   INFO gives information about the format specification.
   N, ARGTYPES, *SIZE has to contain the size of the parameter for
   user-defined types, and return value are as for parse_printf_format
   except that -1 should be returned if the handler cannot handle
   this case.  This allows to partially overwrite the functionality
   of existing format specifiers.  */

static int printf_binary_arginfo_size (__const struct printf_info *info, size_t n, int *argtypes, int *size) {
	int retcode = -1;
	unsigned int item = 0;

	if (n > 0) {
		if (info[n-1].is_char) {
			/* hh flag:
			 * A  following integer conversion corresponds to a signed char or unsigned char argument,
			 * or a following n conversion corresponds to a pointer to a signed char argument.
			 */
			argtypes[item] = PA_CHAR; /* int, cast to char */
			size[item] = sizeof(char);
			retcode = 1;
		} else if (info[n-1].is_short) {
			/* h flag:
			 * A following integer conversion corresponds to a short int or unsigned short int argument,
			 * or a following n conversion corresponds to a pointer to a short int argument.
			 */
			argtypes[item] = PA_INT | PA_FLAG_SHORT;
			size[item] = sizeof(int);
			retcode = 1;
		} else if ((info[n-1].is_long_double) && (info[n-1].is_long)) {
			/* ll flag:
			 * (ell-ell).  A following integer conversion corresponds to a long long int or unsigned long long  int
             * argument, or a following n conversion corresponds to a pointer to a long long int argument.
			 */
			argtypes[item] = PA_INT | PA_FLAG_LONG_LONG;
			size[item] = sizeof(long long);
			retcode = 1;
		} else if (info[n-1].is_long_double) {
			/* L flag:
			 * A following a, A, e, E, f, F, g, or G conversion corresponds to a long double argument.
			 */
			argtypes[item] = PA_DOUBLE | PA_FLAG_LONG_DOUBLE;
			size[item] = sizeof(long double);
			retcode = 1;
		} else if (info[n-1].is_long) {
			/* l flag:
			 * (ell)  A  following integer conversion corresponds to a long int or unsigned long int argument, or a
			 * following n conversion corresponds to a pointer to a long int argument, or a following c  conversion
			 * corresponds  to  a  wint_t argument, or a following s conversion corresponds to a pointer to wchar_t
			 * argument.
			 */
			argtypes[item] = PA_INT | PA_FLAG_LONG;
			size[item] = sizeof(int);
			retcode = 1;
		} else {
			/*
			 * no length modifier has been set
			 */
			argtypes[item] = PA_INT;
			size[item] = sizeof(int);
			retcode = 1;
		}
	} /* (n > 0) */
	return retcode;
}

void printf_binary_register() {
	if (register_printf_specifier(BINARY_FORMAT_KEY,printf_binary,printf_binary_arginfo_size) != 0) {
		const int error = errno;
		if (error != 0) {
			ERROR_MSG("register_printf_specifier %c for binary print has failed, error %d (%m)",BINARY_FORMAT_KEY,error);
		} else {
			ERROR_MSG("register_printf_specifier %c for binary print has failed",BINARY_FORMAT_KEY);
		}
	}
}

#include <dbgflags/ModuleVersionInfo.h>
MODULE_NAME(printf_hook);
MODULE_AUTHOR(Olivier Charloton);
MODULE_VERSION(0.1);
MODULE_FILE_VERSION(0.2);
MODULE_DESCRIPTION(printf extensions to display POD in binary format);
MODULE_COPYRIGHT(LGPL);

#endif /* HAVE_REGISTER_PRINTF_SPECIFIER */
