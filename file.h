#ifndef FILE_H
#define FILE_H

#include "class.h"
#include <stdio.h>

void file_free(ClassFile *class);
int file_read(FILE *fp, ClassFile *class);
char *file_errstr(int i);

#endif /* ifndef FILE_H */
