#ifndef STRVECTOR_H
#define STRVECTOR_H

#include <stddef.h>

char **concat(size_t n, ...);

char **split(char *string, char delim);

char **slice(char **argv, size_t start, size_t end);

void freevec(char **vector);

#endif /* STRVECTOR_H */
