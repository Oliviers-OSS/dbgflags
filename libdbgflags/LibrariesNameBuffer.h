#ifndef LIBRARY_NAME_BUFFER_H_
#define LIBRARY_NAME_BUFFER_H_

typedef struct LibrariesNameBuffer_ {
  unsigned int allocatedSize;
  unsigned int usedSize;
  char *buffer;
  char *writeCursor;
  char *readCursor;
} LibrariesNameBuffer;

static __inline char *LibrariesNameBufferGetBuffer(LibrariesNameBuffer *buffer) {
  return buffer->buffer;  
}

static __inline unsigned int LibrariesNameBufferGetUsedSize(LibrariesNameBuffer *buffer) {
  return buffer->usedSize;
}

static __inline int LibrariesNameBufferInit(LibrariesNameBuffer *buffer) {
  int error = EXIT_SUCCESS;
  buffer->allocatedSize = 4096;
  buffer->usedSize = 0;
  buffer->readCursor = buffer->writeCursor = buffer->buffer = (char*) malloc(buffer->allocatedSize);
  if (NULL == buffer->buffer) {
     error = ENOMEM;
     buffer->allocatedSize = 0;
  }
  return error;
}

static __inline int LibrariesNameBufferAdd(LibrariesNameBuffer *buffer, const char *name) {
  int error = EXIT_SUCCESS;
  const size_t n = strlen(name);
  const unsigned int newUsedSize = n + 1 + buffer->usedSize;

  if (newUsedSize > buffer->allocatedSize) {
    const unsigned int newAllocatedSize = buffer->allocatedSize + 1024;
    char *newBuffer = (char*) realloc(buffer->buffer,newAllocatedSize);
    if (newBuffer != NULL) {
       buffer->allocatedSize = newAllocatedSize;
       buffer->buffer = newBuffer;
    } else {
       error = ENOMEM;
       ERROR_MSG("failed to reallocate %d bytes",newAllocatedSize);
    }
  }

  if (EXIT_SUCCESS == error) {
     strcpy(buffer->writeCursor,name);
     buffer->usedSize = newUsedSize;
     buffer->writeCursor += n;
     *buffer->writeCursor = '\0';
     buffer->writeCursor++;
  }

  return error;
}

static __inline int LibrariesNameBufferAddChar(LibrariesNameBuffer *buffer, const char character) {
  int error = EXIT_SUCCESS;
  const unsigned int newUsedSize = 1 + buffer->usedSize;

  if (newUsedSize > buffer->allocatedSize) {
    const unsigned int newAllocatedSize = buffer->allocatedSize + 1; /* just need one more byte */
    char *newBuffer = (char*) realloc(buffer->buffer,newAllocatedSize);
    if (newBuffer != NULL) {
       buffer->allocatedSize = newAllocatedSize;
       buffer->buffer = newBuffer;
    } else {
       error = ENOMEM;
       ERROR_MSG("failed to reallocate %d bytes",newAllocatedSize);
    }
  }

  if (EXIT_SUCCESS == error) {
     *buffer->writeCursor = character;
     buffer->usedSize = newUsedSize;
     buffer->writeCursor ++;
  }

  return error;
}

static __inline void LibrariesNameBufferFree(LibrariesNameBuffer *buffer) {
  if (buffer->buffer != NULL) {
     free(buffer->buffer);
     buffer->readCursor = buffer->writeCursor = buffer->buffer = NULL;
  }
}

#endif /* LIBRARY_NAME_BUFFER_H_ */
