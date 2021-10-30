typedef enum JavaClass {
	NONE_CLASS     = -1,
	LANG_SYSTEM    = 0,
	LANG_STRING    = 1,
	IO_PRINTSTREAM = 2,
} JavaClass;

JavaClass native_javaclass(char *classname);
void *native_javaobj(JavaClass jclass, char *objname, char *objtype);
int native_javamethod(Frame *frame, JavaClass jclass, char *name, char *type);
