#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cassert>
#include <dbgflags/dbgflags.h>
#include <dbgflags/loggers.h>

#ifndef LOGGER
#define LOGGER consoleLogger
#endif /* LOGGER */
#define FILTER
//#define LOG_OPTS 0

#include <dbgflags/debug_macros.h>


void basic_tests(void) {
	char buffer[5];

	sprintf(buffer,"%b",0);
	DEBUG_MSG("%u %%b => %b (%s)",0,0,buffer);
	assert(strcmp(buffer,"b0") == 0);

	sprintf(buffer,"%b",5);
	DEBUG_MSG("%u %%b => %b (%s)",5,5,buffer);
	assert(strcmp(buffer,"b101") == 0);

	sprintf(buffer,"%10.5b",5);
	DEBUG_MSG("%u %%b => %b (%s)",5,5,buffer);
	assert(strcmp(buffer,"b0000000101") == 0);
}

void basic_tests_32(void) {
    unsigned int i;
    const char *results[] = {                 
		"b00000000000000000000000000000000"
		,"b00000000000000000000000000000001"
		,"b00000000000000000000000000000010"
		,"b00000000000000000000000000000011"
		,"b00000000000000000000000000000100"
		,"b00000000000000000000000000000101"
		,"b00000000000000000000000000000110"
		,"b00000000000000000000000000000111"
		,"b00000000000000000000000000001000"
		,"b00000000000000000000000000001001"
    };
    char buffer[8*sizeof(i)+2];
    for(i=0;i<10;i++) {
    	sprintf(buffer,"%32b",i);
    	DEBUG_MSG("%u %%32b => %32b (%s)",i,i,buffer);
    	assert(strcmp(buffer,results[i]) == 0);
    }
}

void basic_byte_tests(void) {
    unsigned int i;
    const char *results[] = {
		 "b00000000"
		,"b00000001"
		,"b00000010"
		,"b00000011"
		,"b00000100"
		,"b00000101"
		,"b00000110"
		,"b00000111"
		,"b00001000"
		,"b00001001"
    };
    char buffer[8*sizeof(i)+2];
    for(i=0;i<10;i++) {
    	sprintf(buffer,"%8hhb",i);
    	DEBUG_MSG("%u %%8hhb => %8hhb (%s)",i,i,buffer);
    	assert(strcmp(buffer,results[i]) == 0);
    }
}

void basic_format_tests(void) {
	unsigned int i(5);
	unsigned long l(5);
	long long ll(520);
	char buffer[8*sizeof(long long)+2];

	// byte
	sprintf(buffer,"%hhb",i);
	DEBUG_MSG("%u %%hhb => %hhb (%s)",i,i,buffer);
	assert(strcmp(buffer,"b101") == 0);
	i = 405;
	sprintf(buffer,"%hhb",i);
	DEBUG_MSG("%u %%hhb => %hhb (%s)",i,i,buffer);
	assert(strcmp(buffer,"b10010101") == 0);

	// short
	i = 5;
	sprintf(buffer,"%hb",i);
	DEBUG_MSG("%u %%hb => %hb (%s)",i,i,buffer);
	assert(strcmp(buffer,"b101") == 0);
	i = 405;
	sprintf(buffer,"%hb",i);
	DEBUG_MSG("%u %%hb => %hb (%s)",i,i,buffer);
	assert(strcmp(buffer,"b110010101") == 0);

	// long long
	sprintf(buffer,"%llb",ll);
	DEBUG_MSG("%llu %%llb => %llb (%s)",ll,ll,buffer);
	assert(strcmp(buffer,"b1000001000") == 0);

	// long
	i = 1052;
	sprintf(buffer,"%lb",i);
	DEBUG_MSG("%u %%lb => %lb (%s)",i,i,buffer);
	assert(strcmp(buffer,"b10000011100") == 0);
}

int main(int argc, char *argv[]) {
    int error = EXIT_SUCCESS;

    basic_tests();
    basic_tests_32();
    basic_byte_tests();
    basic_format_tests();

    return error;
}
