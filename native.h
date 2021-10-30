typedef enum JavaClass {
  NONE_CLASS = 0,
  LANG_STRING,
  LANG_SYSTEM,
  IO_PRINTSTREAM,
} JavaClass;

JavaClass native_javaclass(char *classname);
void *native_javaobj(JavaClass jclass, char *objname, char *objtype);
int native_javamethod(Frame *frame, JavaClass jclass, char *name, char *type);
