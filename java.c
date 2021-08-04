#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include "util.h"
#include "class.h"
#include "file.h"
#include "memory.h"
#include "native.h"

/* path separator */
#ifdef _WIN32
#define PATHSEP ';'
#define DIRSEP  '\\'
#else
#define PATHSEP ':'
#define DIRSEP  '/'
#endif

enum {
	NO_RETURN = 0,
	RETURN_VOID = 1,
	RETURN_OPERAND = 2,
	RETURN_ERROR = 3
};

int methodcall(ClassFile *class, Frame *frame, char *name, char *descr, U2 flags);

static char **classpath = NULL;         /* NULL-terminated array of path strings */
static ClassFile *classes = NULL;       /* list of loaded classes */

/* show usage */
static void
usage(void)
{
	(void)fprintf(stderr, "usage: java [-cp classpath] class\n");
	exit(EXIT_FAILURE);
}

/* check if a class with the given name is loaded */
static ClassFile *
getclass(char *classname)
{
	ClassFile *class;

	for (class = classes; class; class = class->next)
		if (strcmp(classname, class_getclassname(class, class->this_class)) == 0)
			return class;
	return NULL;
}

/* break cpath into paths and set classpath global variable */
static void
setclasspath(char *cpath)
{
	char *s;
	size_t i, n;

	for (n = 1, s = cpath; *s; s++) {
		if (*s == PATHSEP) {
			*s = '\0';
			n++;
		}
	}
	classpath = ecalloc(n + 1, sizeof *classpath);
	classpath[0] = cpath;
	classpath[n] = NULL;
	for (i = 1; i < n; i++) {
		while (*cpath++)
			;
		classpath[i] = cpath;
	}
}

/* free all the classes in the list of loaded classes */
static void
classfree(void)
{
	ClassFile *tmp;

	while (classes) {
		tmp = classes;
		classes = classes->next;
		file_free(tmp);
		free(tmp);
	}
}

/* recursivelly load class and its superclasses from file matching class name */
static ClassFile *
classload(char *classname)
{
	ClassFile *class, *tmp;
	FILE *fp = NULL;
	size_t cplen, len, i;
	char *basename, *filename;

	if ((class = getclass(classname)) != NULL)
		return class;
	len = strlen(classname);
	basename = emalloc(len + 7);           /* 7 == strlen(".class") + 1 */
	memcpy(basename, classname, len);
	memcpy(basename + len, ".class", 6);
	basename[len + 6] = '\0';
	filename = 0;
	for (i = 0; classpath[i] != NULL; i++) {
		cplen = strlen(classpath[i]);
		filename = emalloc(cplen + len + 8);
		strncpy(filename, classpath[i], cplen);
		filename[cplen] = DIRSEP;
		strncpy(filename + cplen + 1, basename, len + 6);
		filename[cplen + len + 7] = '\0';
		if ((fp = fopen(filename, "rb")) != NULL) {
			free(filename);
			break;
		}
		free(filename);
	}
	free(basename);
	if (fp == NULL)
		errx(EXIT_FAILURE, "could not find class %s", classname);
	class = emalloc(sizeof *class);
	if (file_read(fp, class) != 0) {
		fclose(fp);
		free(class);
		errx(EXIT_FAILURE, "could not load class %s", classname);
	}
	fclose(fp);
	if (strcmp(class_getclassname(class, class->this_class), classname) != 0) {
		free(class);
		errx(EXIT_FAILURE, "could not find class %s", classname);
	}
	class->next = classes;
	class->super = NULL;
	classes = class;
	if (!class_istopclass(class)) {
		class->super = classload(class_getclassname(class, class->super_class));
		for (tmp = class->super; tmp; tmp = tmp->super) {
			if (strcmp(class_getclassname(class, class->this_class),
			           class_getclassname(tmp, tmp->this_class)) == 0) {
				errx(EXIT_FAILURE, "class circularity error");
			}
		}
	}
	return class;
}

/* initialize class */
static void
classinit(ClassFile *class)
{
	Method *method;

	if (class->init)
		return;
	class->init = 1;
	if (class->super)
		classinit(class->super);
	if ((method = class_getmethod(class, "<clinit>", "()V")) != NULL)
	(void)methodcall(class, NULL, "<clinit>", "()V", (class->major_version >= 51 ? ACC_STATIC : ACC_NONE));
}

/* resolve constant reference */
static Value
resolveconstant(ClassFile *class, U2 index)
{
	Value v;
	char *s;

	v.i = 0;
	switch (class->constant_pool[index]->tag) {
	case CONSTANT_Integer:
		v.i = class_getinteger(class, index);
		break;
	case CONSTANT_Float:
		v.f = class_getfloat(class, index);
		break;
	case CONSTANT_Long:
		v.l = class_getlong(class, index);
		break;
	case CONSTANT_Double:
		v.d = class_getdouble(class, index);
		break;
	case CONSTANT_String:
		s = class_getstring(class, index);
		v.v = heap_alloc(1, sizeof (char *));
		v.v->obj = s;
		break;
	}
	return v;
}

/* resolve field reference */
static Value
resolvefield(ClassFile *class, CONSTANT_Fieldref_info *fieldref)
{
	Value v;
	enum JavaClass jclass;
	char *classname, *name, *type;

	v.v = NULL;
	classname = class_getclassname(class, fieldref->class_index);
	class_getnameandtype(class, fieldref->name_and_type_index, &name, &type);
	if ((jclass = native_javaclass(classname)) != 0) {
		v.v = heap_alloc(1, sizeof (void *));
		v.v->obj = native_javaobj(jclass, name, type);
	} else {
		// TODO
	}
	if (v.v == NULL)
		errx(EXIT_FAILURE, "could not resolve field");
	return v;
}

/* aaload: load reference from array */
static int
opaaload(Frame *frame)
{
	Value va, vi, v;

	vi = frame_stackpop(frame);
	va = frame_stackpop(frame);
	if (va.v->obj == NULL) {
		// TODO: throw NullPointerException
	}
	if (vi.i < 0 || vi.i >= va.v->nmemb) {
		// TODO: throw ArrayIndexOutOfBoundsException
	}
	v.v = ((void **)va.v->obj)[vi.i];
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* aastore: store into reference array */
static int
opaastore(Frame *frame)
{
	Value va, vi, vv;

	vv = frame_stackpop(frame);
	vi = frame_stackpop(frame);
	va = frame_stackpop(frame);
	if (va.v->obj == NULL) {
		// TODO: throw NullPointerException
	}
	if (vi.i < 0 || vi.i >= va.v->nmemb) {
		// TODO: throw ArrayIndexOutOfBoundsException
	}
	((void **)va.v->obj)[vi.i] = vv.v;
	return NO_RETURN;
}

/* arraylength: get length of array */
static int
oparraylength(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	v.i = v.v->nmemb;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* bipush: push byte */
static int
opbipush(Frame *frame)
{
	Value v;

	v.i = (int8_t)frame->code->code[frame->pc++];
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* dadd: add double */
static int
opdadd(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.d += v2.d;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* daload: load double from array */
static int
opdaload(Frame *frame)
{
	Value va, vi, v;

	vi = frame_stackpop(frame);
	va = frame_stackpop(frame);
	if (va.v->obj == NULL) {
		// TODO: throw NullPointerException
	}
	if (vi.i < 0 || vi.i >= va.v->nmemb) {
		// TODO: throw ArrayIndexOutOfBoundsException
	}
	v.d = ((double *)va.v->obj)[vi.i];
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* dastore: store into double array */
static int
opdastore(Frame *frame)
{
	Value va, vi, vv;

	vv = frame_stackpop(frame);
	vi = frame_stackpop(frame);
	va = frame_stackpop(frame);
	if (va.v->obj == NULL) {
		// TODO: throw NullPointerException
	}
	if (vi.i < 0 || vi.i >= va.v->nmemb) {
		// TODO: throw ArrayIndexOutOfBoundsException
	}
	((double *)va.v->obj)[vi.i] = vv.d;
	return NO_RETURN;
}

/* dconst_0: push double 0 into stack */
static int
opdconst_0(Frame *frame)
{
	Value v;

	v.d = 0.0;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* dconst_1: push double 1 into stack */
static int
opdconst_1(Frame *frame)
{
	Value v;

	v.d = 1.0;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* ddiv: divide double */
static int
opddiv(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.d /= v2.d;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* dmul: multiply double */
static int
opdmul(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.d *= v2.d;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* dneg: negate double */
static int
opdneg(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	v.d = -v.d;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* drem: remainder double */
static int
opdrem(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.d = fmod(v1.d, v2.d);
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* dsub: subtract double */
static int
opdsub(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.d -= v2.d;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* dup: duplicate the top operand stack value */
static int
opdup(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	frame_stackpush(frame, v);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* dup2: duplicate the top one or two operand stack values */
static int
opdup2(Frame *frame)
{
	Value v1, v2;

	v1 = frame_stackpop(frame);
	v2 = frame_stackpop(frame);
	frame_stackpush(frame, v2);
	frame_stackpush(frame, v1);
	frame_stackpush(frame, v2);
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* dup2_x1: duplicate the top one or two operand stack values and insert two or three values down */
static int
opdup2_x1(Frame *frame)
{
	Value v1, v2, v3;

	v1 = frame_stackpop(frame);
	v2 = frame_stackpop(frame);
	v3 = frame_stackpop(frame);
	frame_stackpush(frame, v2);
	frame_stackpush(frame, v1);
	frame_stackpush(frame, v3);
	frame_stackpush(frame, v2);
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* dup2_x2: duplicate the top one or two operand stack values and insert two, three or four values down */
static int
opdup2_x2(Frame *frame)
{
	Value v1, v2, v3, v4;

	v1 = frame_stackpop(frame);
	v2 = frame_stackpop(frame);
	v3 = frame_stackpop(frame);
	v4 = frame_stackpop(frame);
	frame_stackpush(frame, v2);
	frame_stackpush(frame, v1);
	frame_stackpush(frame, v4);
	frame_stackpush(frame, v3);
	frame_stackpush(frame, v2);
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* dup_x1: duplicate the top operand stack value and insert two values down */
static int
opdup_x1(Frame *frame)
{
	Value v1, v2;

	v1 = frame_stackpop(frame);
	v2 = frame_stackpop(frame);
	frame_stackpush(frame, v1);
	frame_stackpush(frame, v2);
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* dup_x2: duplicate the top operand stack value and insert two or three values down */
static int
opdup_x2(Frame *frame)
{
	Value v1, v2, v3;

	v1 = frame_stackpop(frame);
	v2 = frame_stackpop(frame);
	v3 = frame_stackpop(frame);
	frame_stackpush(frame, v1);
	frame_stackpush(frame, v3);
	frame_stackpush(frame, v2);
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* fadd: add float */
static int
opfadd(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.f += v2.f;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* faload: load float from array */
static int
opfaload(Frame *frame)
{
	Value va, vi, v;

	vi = frame_stackpop(frame);
	va = frame_stackpop(frame);
	if (va.v->obj == NULL) {
		// TODO: throw NullPointerException
	}
	if (vi.i < 0 || vi.i >= va.v->nmemb) {
		// TODO: throw ArrayIndexOutOfBoundsException
	}
	v.f = ((float *)va.v->obj)[vi.i];
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* fastore: store into float array */
static int
opfastore(Frame *frame)
{
	Value va, vi, vv;

	vv = frame_stackpop(frame);
	vi = frame_stackpop(frame);
	va = frame_stackpop(frame);
	if (va.v->obj == NULL) {
		// TODO: throw NullPointerException
	}
	if (vi.i < 0 || vi.i >= va.v->nmemb) {
		// TODO: throw ArrayIndexOutOfBoundsException
	}
	((float *)va.v->obj)[vi.i] = vv.f;
	return NO_RETURN;
}

/* fconst_0: push float 0 into stack */
static int
opfconst_0(Frame *frame)
{
	Value v;

	v.f = 0.0;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* fconst_1: push float 1 into stack */
static int
opfconst_1(Frame *frame)
{
	Value v;

	v.f = 1.0;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* fconst_2: push float 2 into stack */
static int
opfconst_2(Frame *frame)
{
	Value v;

	v.f = 2.0;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* fdiv: divide float */
static int
opfdiv(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.f /= v2.f;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* fmul: multiply float */
static int
opfmul(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.f *= v2.f;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* fneg: negate float */
static int
opfneg(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	v.f = -v.f;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* frem: remainder float */
static int
opfrem(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.f = fmodf(v1.f, v2.f);
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* fsub: subtract float */
static int
opfsub(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.f -= v2.f;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* getstatic: get static field from class */
static int
opgetstatic(Frame *frame)
{
	CONSTANT_Fieldref_info *fieldref;
	Value v;
	U2 i;

	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	fieldref = &frame->class->constant_pool[i]->info.fieldref_info;
	v = resolvefield(frame->class, fieldref);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* goto: branch always */
static int
opgoto(Frame *frame)
{
	int16_t off = 0;
	U2 base, i;

	base = frame->pc - 1;
	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	memcpy(&off, &i, sizeof(off));
	frame->pc = base + off;
	return NO_RETURN;
}

/* i2b: convert int to byte */
static int
opi2b(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	v.i = v.i & UINT8_MAX;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iadd: add int */
static int
opiadd(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.i += v2.i;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* iaload: load int from array */
static int
opiaload(Frame *frame)
{
	Value va, vi, v;

	vi = frame_stackpop(frame);
	va = frame_stackpop(frame);
	if (va.v->obj == NULL) {
		// TODO: throw NullPointerException
	}
	if (vi.i < 0 || vi.i >= va.v->nmemb) {
		// TODO: throw ArrayIndexOutOfBoundsException
	}
	v.i = ((int *)va.v->obj)[vi.i];
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iastore: store into int array */
static int
opiastore(Frame *frame)
{
	Value va, vi, vv;

	vv = frame_stackpop(frame);
	vi = frame_stackpop(frame);
	va = frame_stackpop(frame);
	if (va.v->obj == NULL) {
		// TODO: throw NullPointerException
	}
	if (vi.i < 0 || vi.i >= va.v->nmemb) {
		// TODO: throw ArrayIndexOutOfBoundsException
	}
	((int *)va.v->obj)[vi.i] = vv.i;
	return NO_RETURN;
}

/* iconst_0: push int 0 into stack */
static int
opiconst_0(Frame *frame)
{
	Value v;

	v.i = 0;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iconst_1: push int 1 into stack */
static int
opiconst_1(Frame *frame)
{
	Value v;

	v.i = 1;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iconst_2: push int 2 into stack */
static int
opiconst_2(Frame *frame)
{
	Value v;

	v.i = 2;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iconst_3: push int 3 into stack */
static int
opiconst_3(Frame *frame)
{
	Value v;

	v.i = 3;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iconst_4: push int 4 into stack */
static int
opiconst_4(Frame *frame)
{
	Value v;

	v.i = 4;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iconst_5: push int 5 into stack */
static int
opiconst_5(Frame *frame)
{
	Value v;

	v.i = 5;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iconst_m1: push int -1 into stack */
static int
opiconst_m1(Frame *frame)
{
	Value v;

	v.i = -1;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* idiv: divide int */
static int
opidiv(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.i /= v2.i;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* if_icmpeq: branch if int comparison succeeds */
static int
opif_icmpeq(Frame *frame)
{
	Value v1, v2;
	int16_t off = 0;
	U2 base, i;

	base = frame->pc - 1;
	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	if (v1.i == v2.i) {
		memcpy(&off, &i, sizeof off);
		frame->pc = base + off;
	}
	return NO_RETURN;
}

/* if_icmpge: branch if int comparison succeeds */
static int
opif_icmpge(Frame *frame)
{
	Value v1, v2;
	int16_t off = 0;
	U2 base, i;

	base = frame->pc - 1;
	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	if (v1.i >= v2.i) {
		memcpy(&off, &i, sizeof off);
		frame->pc = base + off;
	}
	return NO_RETURN;
}

/* if_icmpgt: branch if int comparison succeeds */
static int
opif_icmpgt(Frame *frame)
{
	Value v1, v2;
	int16_t off = 0;
	U2 base, i;

	base = frame->pc - 1;
	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	if (v1.i > v2.i) {
		memcpy(&off, &i, sizeof off);
		frame->pc = base + off;
	}
	return NO_RETURN;
}

/* if_icmple: branch if int comparison succeeds */
static int
opif_icmple(Frame *frame)
{
	Value v1, v2;
	int16_t off = 0;
	U2 base, i;

	base = frame->pc - 1;
	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	if (v1.i <= v2.i) {
		memcpy(&off, &i, sizeof off);
		frame->pc = base + off;
	}
	return NO_RETURN;
}

/* if_icmplt: branch if int comparison succeeds */
static int
opif_icmplt(Frame *frame)
{
	Value v1, v2;
	int16_t off = 0;
	U2 base, i;

	base = frame->pc - 1;
	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	if (v1.i < v2.i) {
		memcpy(&off, &i, sizeof off);
		frame->pc = base + off;
	}
	return NO_RETURN;
}

/* if_icmpne: branch if int comparison succeeds */
static int
opif_icmpne(Frame *frame)
{
	Value v1, v2;
	int16_t off = 0;
	U2 base, i;

	base = frame->pc - 1;
	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	if (v1.i != v2.i) {
		memcpy(&off, &i, sizeof off);
		frame->pc = base + off;
	}
	return NO_RETURN;
}

/* iinc: increment local variable by constant */
static int
opiinc(Frame *frame)
{
	Value v;
	U1 i;
	int8_t c;

	i = frame->code->code[frame->pc++];
	c = (int8_t)frame->code->code[frame->pc++];
	v = frame_localload(frame, i);
	v.i += c;
	frame_localstore(frame, i, v);
	return NO_RETURN;
}

/* iload: load int from local variable */
static int
opiload(Frame *frame)
{
	Value v;
	U2 i;

	i = frame->code->code[frame->pc++];
	v = frame_localload(frame, i);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iload_0: load int from local variable */
static int
opiload_0(Frame *frame)
{
	Value v;

	v = frame_localload(frame, 0);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iload_1: load int from local variable */
static int
opiload_1(Frame *frame)
{
	Value v;

	v = frame_localload(frame, 1);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iload_2: load int from local variable */
static int
opiload_2(Frame *frame)
{
	Value v;

	v = frame_localload(frame, 2);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* iload_3: load int from local variable */
static int
opiload_3(Frame *frame)
{
	Value v;

	v = frame_localload(frame, 3);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* imul: multiply int */
static int
opimul(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.i *= v2.i;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* ineg: negate int */
static int
opineg(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	v.i = -v.i;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* invokestatic: invoke a class (static) method */
static int
opinvokestatic(Frame *frame)
{
	CONSTANT_Methodref_info *methodref;
	ClassFile *class;
	enum JavaClass jclass;
	char *classname, *name, *type;
	U2 i;

	// TODO: method must not be an instance initialization method,
	//       or the class or interface initialization method.
	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	methodref = &frame->class->constant_pool[i]->info.methodref_info;
	classname = class_getclassname(frame->class, methodref->class_index);
	class_getnameandtype(frame->class, methodref->name_and_type_index, &name, &type);
	if ((jclass = native_javaclass(classname)) != 0) {
		native_javamethod(frame, jclass, name, type);
	} else if ((class = classload(classname)) != NULL) {
		classinit(class);
		if (methodcall(class, frame, name, type, ACC_STATIC) == -1) {
			errx(EXIT_FAILURE, "could not find method %s", name);
		}
	} else {
		errx(EXIT_FAILURE, "could not load class %s", classname);
	}
	return NO_RETURN;
}

/* invokevirtual: invoke instance method; dispatch based on class */
static int
opinvokevirtual(Frame *frame)
{
	CONSTANT_Methodref_info *methodref;
	ClassFile *class;
	enum JavaClass jclass;
	char *classname, *name, *type;
	U2 i;

	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	methodref = &frame->class->constant_pool[i]->info.methodref_info;
	classname = class_getclassname(frame->class, methodref->class_index);
	class_getnameandtype(frame->class, methodref->name_and_type_index, &name, &type);
	if ((jclass = native_javaclass(classname)) != 0) {
		native_javamethod(frame, jclass, name, type);
	} else if ((class = classload(classname)) != NULL) {
		classinit(class);
		if (methodcall(class, NULL, name, type, ACC_STATIC) == -1) {
			errx(EXIT_FAILURE, "could not find method %s", name);
		}
	} else {
		errx(EXIT_FAILURE, "could not load class %s", classname);
	}
	return NO_RETURN;
}

/* irem: remainder int */
static int
opirem(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.i = v1.i - (v1.i / v2.i) * v2.i;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* ireturn: return something from method */
static int
opireturn(Frame *frame)
{
	(void)frame;
	return RETURN_OPERAND;
}

/* store: store into local variable */
static int
opistore(Frame *frame)
{
	Value v;
	U2 i;

	i = frame->code->code[frame->pc++];
	v = frame_stackpop(frame);
	frame_localstore(frame, i, v);
	return NO_RETURN;
}

/* istore_0: store int into local variable */
static int
opistore_0(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	frame_localstore(frame, 0, v);
	return NO_RETURN;
}

/* istore_1: store int into local variable */
static int
opistore_1(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	frame_localstore(frame, 1, v);
	return NO_RETURN;
}

/* istore_2: store int into local variable */
static int
opistore_2(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	frame_localstore(frame, 2, v);
	return NO_RETURN;
}

/* istore_3: store int into local variable */
static int
opistore_3(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	frame_localstore(frame, 3, v);
	return NO_RETURN;
}

/* isub: subtract int */
static int
opisub(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.i -= v2.i;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* ladd: add long */
static int
opladd(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.l += v2.l;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* laload: load long from array */
static int
oplaload(Frame *frame)
{
	Value va, vi, v;

	vi = frame_stackpop(frame);
	va = frame_stackpop(frame);
	if (va.v->obj == NULL) {
		// TODO: throw NullPointerException
	}
	if (vi.i < 0 || vi.i >= va.v->nmemb) {
		// TODO: throw ArrayIndexOutOfBoundsException
	}
	v.l = ((long *)va.v->obj)[vi.i];
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* lastore: store into long array */
static int
oplastore(Frame *frame)
{
	Value va, vi, vv;

	vv = frame_stackpop(frame);
	vi = frame_stackpop(frame);
	va = frame_stackpop(frame);
	if (va.v->obj == NULL) {
		// TODO: throw NullPointerException
	}
	if (vi.i < 0 || vi.i >= va.v->nmemb) {
		// TODO: throw ArrayIndexOutOfBoundsException
	}
	((long *)va.v->obj)[vi.i] = vv.l;
	return NO_RETURN;
}

/* lconst_0: push long 0 into stack */
static int
oplconst_0(Frame *frame)
{
	Value v;

	v.l = 0;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* lconst_1: push long 1 into stack */
static int
oplconst_1(Frame *frame)
{
	Value v;

	v.l = 1;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* ldc: push item from run-time constant pool */
static int
opldc(Frame *frame)
{
	Value v;
	U2 i;

	i = frame->code->code[frame->pc++];
	v = resolveconstant(frame->class, i);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* ldc2_w: push long or double from run-time constant pool (wide index) */
static int
opldc2_w(Frame *frame)
{
	Value v;
	U2 i;

	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	v = resolveconstant(frame->class, i);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* ldc_w: push item from run-time constant pool (wide index) */
static int
opldc_w(Frame *frame)
{
	Value v;
	U2 i;

	i = frame->code->code[frame->pc++] << 8;
	i |= frame->code->code[frame->pc++];
	v = resolveconstant(frame->class, i);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* ldiv: divide long */
static int
opldiv(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.l /= v2.l;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* lload: load long from local variable */
static int
oplload(Frame *frame)
{
	Value v;
	U2 i;

	i = frame->code->code[frame->pc++];
	v = frame_localload(frame, i);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* lload_0: load long from local variable */
static int
oplload_0(Frame *frame)
{
	Value v;

	v = frame_localload(frame, 0);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* lload_1: load long from local variable */
static int
oplload_1(Frame *frame)
{
	Value v;

	v = frame_localload(frame, 1);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* lload_2: load long from local variable */
static int
oplload_2(Frame *frame)
{
	Value v;

	v = frame_localload(frame, 2);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* lload_3: load long from local variable */
static int
oplload_3(Frame *frame)
{
	Value v;

	v = frame_localload(frame, 3);
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* lmul: multiply long */
static int
oplmul(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.l *= v2.l;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* lneg: negate long */
static int
oplneg(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	v.l = -v.l;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* lrem: remainder long */
static int
oplrem(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.l = v1.l - (v1.l / v2.l) * v2.l;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* lstore: store long into local variable */
static int
oplstore(Frame *frame)
{
	Value v;
	U2 i;

	i = frame->code->code[frame->pc++];
	v = frame_stackpop(frame);
	frame_localstore(frame, i, v);
	frame_localstore(frame, i + 1, v);
	return NO_RETURN;
}

/* lstore_0: store long into local variable */
static int
oplstore_0(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	frame_localstore(frame, 0, v);
	frame_localstore(frame, 1, v);
	return NO_RETURN;
}

/* lstore_1: store long into local variable */
static int
oplstore_1(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	frame_localstore(frame, 1, v);
	frame_localstore(frame, 2, v);
	return NO_RETURN;
}

/* lstore_2: store long into local variable */
static int
oplstore_2(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	frame_localstore(frame, 2, v);
	frame_localstore(frame, 3, v);
	return NO_RETURN;
}

/* lstore_3: store long into local variable */
static int
oplstore_3(Frame *frame)
{
	Value v;

	v = frame_stackpop(frame);
	frame_localstore(frame, 3, v);
	frame_localstore(frame, 4, v);
	return NO_RETURN;
}

/* lsub: subtract long */
static int
oplsub(Frame *frame)
{
	Value v1, v2;

	v2 = frame_stackpop(frame);
	v1 = frame_stackpop(frame);
	v1.l -= v2.l;
	frame_stackpush(frame, v1);
	return NO_RETURN;
}

/* multianewarray: create new multidimensional array */
static int
opmultianewarray(Frame *frame)
{
	Value v;
	Heap *h;
	char *type;
	int32_t *sizes;
	U1 i, dimension;
	U2 index;
	size_t s;

	index = frame->code->code[frame->pc++] << 8;
	index = frame->code->code[frame->pc++];
	dimension = frame->code->code[frame->pc++];
	sizes = ecalloc(dimension, sizeof *sizes);
	type = class_getclassname(frame->class, index);
	switch (*type) {
	case 'L':
	case '[':
		s = sizeof (void *);
		break;
	case 'D':
	case 'J':
		s = sizeof (int64_t);
		break;
	default:
		s = sizeof (int32_t);
		break;
	}
	for (i = 0; i < dimension; i++) {
		v = frame_stackpop(frame);
		if (v.i < 0) {
			// TODO: throw NegativeArraySizeException
		}
		if (v.i == 0) {
			// TODO: handle zero size
		}
		sizes[dimension - i - 1] = v.i;
	}
	h = array_new(sizes, dimension, s);
	if (h == NULL) {
		// TODO: throw error
	}
	v.v = h;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* newarray: create new array */
static int
opnewarray(Frame *frame)
{
	Value v;
	U1 type;
	Heap *h;
	size_t s;

	type = frame->code->code[frame->pc++];
	v = frame_stackpop(frame);
	switch (type) {
	case T_LONG:
		s = sizeof (int64_t);
		break;
	case T_DOUBLE:
		s = sizeof (double);
		break;
	case T_FLOAT:
		s = sizeof (float);
		break;
	default:
		s = sizeof (int32_t);
		break;
	}
	if (v.i < 0) {
		// TODO: throw NegativeArraySizeException
	}
	if (v.i == 0) {
		// TODO: handle zero size
	}
	h = array_new(&v.i, 1, s);
	v.v = h;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* nop: do nothing */
static int
opnop(Frame *frame)
{
	(void)frame;
	errx(EXIT_FAILURE, "instruction %02x not implemented (yet)", frame->code->code[frame->pc - 1]);
	return NO_RETURN;
}

/* pop: pop the top operand stack value */
static int
oppop(Frame *frame)
{
	Value v;
	v = frame_stackpop(frame);
	return NO_RETURN;
}

/* pop: pop the top one or two operand stack values */
static int
oppop2(Frame *frame)
{
	frame_stackpop(frame);
	frame_stackpop(frame);
	return NO_RETURN;
}

/* return: return void from method */
static int
opreturn(Frame *frame)
{
	(void)frame;
	return RETURN_VOID;
}

/* sipush: push short */
static int
opsipush(Frame *frame)
{
	Value v;
	U2 u;

	u = frame->code->code[frame->pc++] << 8;
	u |= frame->code->code[frame->pc++];
	v.i = (int16_t)u;
	frame_stackpush(frame, v);
	return NO_RETURN;
}

/* access jump table by index and jump */
static int
optableswitch(Frame *frame)
{
	int32_t def, high, low, j, n;
	U4 a, b, c, d, baseaddr, targetaddr;
	Value v;

	baseaddr = frame->pc - 1;
	while (frame->pc % 4)
		frame->pc++;
	v = frame_stackpop(frame);
	a = frame->code->code[frame->pc++];
	b = frame->code->code[frame->pc++];
	c = frame->code->code[frame->pc++];
	d = frame->code->code[frame->pc++];
	def = (a << 24) | (b << 16) | (c << 8) | d;
	a = frame->code->code[frame->pc++];
	b = frame->code->code[frame->pc++];
	c = frame->code->code[frame->pc++];
	d = frame->code->code[frame->pc++];
	low = (a << 24) | (b << 16) | (c << 8) | d;
	a = frame->code->code[frame->pc++];
	b = frame->code->code[frame->pc++];
	c = frame->code->code[frame->pc++];
	d = frame->code->code[frame->pc++];
	high = (a << 24) | (b << 16) | (c << 8) | d;
	targetaddr = baseaddr + def;
	for (j = low; j <= high; j++) {
		a = frame->code->code[frame->pc++];
		b = frame->code->code[frame->pc++];
		c = frame->code->code[frame->pc++];
		d = frame->code->code[frame->pc++];
		n = (a << 24) | (b << 16) | (c << 8) | d;
		if (v.i == j) {
			targetaddr = baseaddr + n;
			break;
		}
	}
	frame->pc = targetaddr;
	return NO_RETURN;
}

/* call method */
int
methodcall(ClassFile *class, Frame *frame, char *name, char *descriptor, U2 flags)
{
	static int(*instrtab[])(Frame *) = {
		/*
		 * some functions are used in more than one instructions,
		 * for example, there is no opareturn, opdreturn or oplreturn,
		 * there is only opireturn, that implements all function that
		 * return something.
		 */
		[NOP]             = opnop,
		[ACONST_NULL]     = opnop,
		[ICONST_M1]       = opiconst_m1,
		[ICONST_0]        = opiconst_0,
		[ICONST_1]        = opiconst_1,
		[ICONST_2]        = opiconst_2,
		[ICONST_3]        = opiconst_3,
		[ICONST_4]        = opiconst_4,
		[ICONST_5]        = opiconst_5,
		[LCONST_0]        = oplconst_0,
		[LCONST_1]        = oplconst_1,
		[FCONST_0]        = opfconst_0,
		[FCONST_1]        = opfconst_1,
		[FCONST_2]        = opfconst_2,
		[DCONST_0]        = opdconst_0,
		[DCONST_1]        = opdconst_1,
		[BIPUSH]          = opbipush,
		[SIPUSH]          = opsipush,
		[LDC]             = opldc,
		[LDC_W]           = opldc_w,
		[LDC2_W]          = opldc2_w,
		[ILOAD]           = opiload,
		[LLOAD]           = oplload,
		[FLOAD]           = opiload,
		[DLOAD]           = oplload,
		[ALOAD]           = opiload,
		[ILOAD_0]         = opiload_0,
		[ILOAD_1]         = opiload_1,
		[ILOAD_2]         = opiload_2,
		[ILOAD_3]         = opiload_3,
		[LLOAD_0]         = oplload_0,
		[LLOAD_1]         = oplload_1,
		[LLOAD_2]         = oplload_2,
		[LLOAD_3]         = oplload_3,
		[FLOAD_0]         = opiload_0,
		[FLOAD_1]         = opiload_1,
		[FLOAD_2]         = opiload_2,
		[FLOAD_3]         = opiload_3,
		[DLOAD_0]         = oplload_0,
		[DLOAD_1]         = oplload_1,
		[DLOAD_2]         = oplload_2,
		[DLOAD_3]         = oplload_3,
		[ALOAD_0]         = opiload_0,
		[ALOAD_1]         = opiload_1,
		[ALOAD_2]         = opiload_2,
		[ALOAD_3]         = opiload_3,
		[IALOAD]          = opiaload,
		[LALOAD]          = oplaload,
		[FALOAD]          = opfaload,
		[DALOAD]          = opdaload,
		[AALOAD]          = opaaload,
		[BALOAD]          = opiaload,
		[CALOAD]          = opiaload,
		[SALOAD]          = opiaload,
		[ISTORE]          = opistore,
		[LSTORE]          = oplstore,
		[FSTORE]          = opistore,
		[DSTORE]          = oplstore,
		[ASTORE]          = opistore,
		[ISTORE_0]        = opistore_0,
		[ISTORE_1]        = opistore_1,
		[ISTORE_2]        = opistore_2,
		[ISTORE_3]        = opistore_3,
		[LSTORE_0]        = oplstore_0,
		[LSTORE_1]        = oplstore_1,
		[LSTORE_2]        = oplstore_2,
		[LSTORE_3]        = oplstore_3,
		[FSTORE_0]        = opistore_0,
		[FSTORE_1]        = opistore_1,
		[FSTORE_2]        = opistore_2,
		[FSTORE_3]        = opistore_3,
		[DSTORE_0]        = oplstore_0,
		[DSTORE_1]        = oplstore_1,
		[DSTORE_2]        = oplstore_2,
		[DSTORE_3]        = oplstore_3,
		[ASTORE_0]        = opistore_0,
		[ASTORE_1]        = opistore_1,
		[ASTORE_2]        = opistore_2,
		[ASTORE_3]        = opistore_3,
		[IASTORE]         = opiastore,
		[LASTORE]         = oplastore,
		[FASTORE]         = opfastore,
		[DASTORE]         = opdastore,
		[AASTORE]         = opaastore,
		[BASTORE]         = opiastore,
		[CASTORE]         = opiastore,
		[SASTORE]         = opiastore,
		[POP]             = oppop,
		[POP2]            = oppop2,
		[DUP]             = opdup,
		[DUP_X1]          = opdup_x1,
		[DUP_X2]          = opdup_x2,
		[DUP2]            = opdup2,
		[DUP2_X1]         = opdup2_x1,
		[DUP2_X2]         = opdup2_x2,
		[SWAP]            = opnop,
		[IADD]            = opiadd,
		[LADD]            = opladd,
		[FADD]            = opfadd,
		[DADD]            = opdadd,
		[ISUB]            = opisub,
		[LSUB]            = oplsub,
		[FSUB]            = opfsub,
		[DSUB]            = opdsub,
		[IMUL]            = opimul,
		[LMUL]            = oplmul,
		[FMUL]            = opfmul,
		[DMUL]            = opdmul,
		[IDIV]            = opidiv,
		[LDIV]            = opldiv,
		[FDIV]            = opfdiv,
		[DDIV]            = opddiv,
		[IREM]            = opirem,
		[LREM]            = oplrem,
		[FREM]            = opfrem,
		[DREM]            = opdrem,
		[INEG]            = opineg,
		[LNEG]            = oplneg,
		[FNEG]            = opfneg,
		[DNEG]            = opdneg,
		[ISHL]            = opnop,
		[LSHL]            = opnop,
		[ISHR]            = opnop,
		[LSHR]            = opnop,
		[IUSHR]           = opnop,
		[LUSHR]           = opnop,
		[IAND]            = opnop,
		[LAND]            = opnop,
		[IOR]             = opnop,
		[LOR]             = opnop,
		[IXOR]            = opnop,
		[LXOR]            = opnop,
		[IINC]            = opiinc,
		[I2L]             = opnop,
		[I2F]             = opnop,
		[I2D]             = opnop,
		[L2I]             = opnop,
		[L2F]             = opnop,
		[L2D]             = opnop,
		[F2I]             = opnop,
		[F2L]             = opnop,
		[F2D]             = opnop,
		[D2I]             = opnop,
		[D2L]             = opnop,
		[D2F]             = opnop,
		[I2B]             = opi2b,
		[I2C]             = opnop,
		[I2S]             = opnop,
		[LCMP]            = opnop,
		[FCMPL]           = opnop,
		[FCMPG]           = opnop,
		[DCMPL]           = opnop,
		[DCMPG]           = opnop,
		[IFEQ]            = opnop,
		[IFNE]            = opnop,
		[IFLT]            = opnop,
		[IFGE]            = opnop,
		[IFGT]            = opnop,
		[IFLE]            = opnop,
		[IF_ICMPEQ]       = opif_icmpeq,
		[IF_ICMPNE]       = opif_icmpne,
		[IF_ICMPLT]       = opif_icmplt,
		[IF_ICMPGE]       = opif_icmpge,
		[IF_ICMPGT]       = opif_icmpgt,
		[IF_ICMPLE]       = opif_icmple,
		[IF_ACMPEQ]       = opnop,
		[IF_ACMPNE]       = opnop,
		[GOTO]            = opgoto,
		[JSR]             = opnop,
		[RET]             = opnop,
		[TABLESWITCH]     = optableswitch,
		[LOOKUPSWITCH]    = opnop,
		[IRETURN]         = opireturn,
		[LRETURN]         = opireturn,
		[FRETURN]         = opireturn,
		[DRETURN]         = opireturn,
		[ARETURN]         = opireturn,
		[RETURN]          = opreturn,
		[GETSTATIC]       = opgetstatic,
		[PUTSTATIC]       = opnop,
		[GETFIELD]        = opnop,
		[PUTFIELD]        = opnop,
		[INVOKEVIRTUAL]   = opinvokevirtual,
		[INVOKESPECIAL]   = opnop,
		[INVOKESTATIC]    = opinvokestatic,
		[INVOKEINTERFACE] = opnop,
		[INVOKEDYNAMIC]   = opnop,
		[NEW]             = opnop,
		[NEWARRAY]        = opnewarray,
		[ANEWARRAY]       = opnop,
		[ARRAYLENGTH]     = oparraylength,
		[ATHROW]          = opnop,
		[CHECKCAST]       = opnop,
		[INSTANCEOF]      = opnop,
		[MONITORENTER]    = opnop,
		[MONITOREXIT]     = opnop,
		[WIDE]            = opnop,
		[MULTIANEWARRAY]  = opmultianewarray,
		[IFNULL]          = opnop,
		[IFNONNULL]       = opnop,
		[GOTO_W]          = opnop,
		[JSR_W]           = opnop,
	};
	Attribute *cattr;       /* Code_attribute */
	Code_attribute *code;
	Frame *newframe;
	Method *method;
	Value v;
	char *s;
	U2 i;
	int ret = NO_RETURN;

	if ((method = class_getmethod(class, name, descriptor)) == NULL)
		return -1;
	if ((flags != ACC_NONE) && !(method->access_flags & flags))
		return -1;
	if ((cattr = class_getattr(method->attributes, method->attributes_count, Code)) == NULL)
		err(EXIT_FAILURE, "could not find code for method %s", name);
	code = &cattr->info.code;
	if ((newframe = frame_push(code, class, code->max_locals, code->max_stack)) == NULL)
		err(EXIT_FAILURE, "out of memory");
	if (frame) {
		s = descriptor;
		i = 0;
		while (*s && *s != ')') {
			v = frame_stackpop(frame);
			frame_localstore(newframe, i++, v);
			switch (*(++s)) {
			case 'L':
				while (*s && *s != ';') {
					s++;
				}
				if (*s == ';') {
					s++;
				}
				break;
			case '[':
				while (*s == '[') {
					s++;
				}
				if (*s == 'L') {
					while (*s && *s != ';') {
						s++;
					}
					if (*s == ';') {
						s++;
					}
				} else {
					s++;
				}
				break;
			case 'D':
			case 'J':
				frame_localstore(newframe, i++, v);
				/* FALLTHROUGH */
			default:
				s++;
				break;
			}
		}
	}
	while (newframe->pc < code->code_length) {
		if ((ret = (*instrtab[code->code[newframe->pc++]])(newframe)) != NO_RETURN) {
			break;
		}
	}
	if (ret == RETURN_OPERAND) {
		v = frame_stackpop(newframe);
		frame_stackpush(frame, v);
	}
	frame_pop();
	return 0;
}

/* load and initialize main class, then call main method */
static void
java(int argc, char *argv[])
{
	ClassFile *class;
	Frame *frame;
	Heap *h;
	Value v;
	int i;

	class = classload(argv[0]);
	argc--;
	argv++;
	classinit(class);
	frame = frame_push(NULL, NULL, 0, 1);
	v.v = array_new(&argc, 1, sizeof (void *));
	for (i = 0; i < argc; i++) {
		h = array_new((int32_t[]){0}, 1, 0);
		h->obj = argv[i];
		((void **)v.v->obj)[i] = h;
	}
	frame_stackpush(frame, v);
	if (methodcall(class, frame, "main", "([Ljava/lang/String;)V", (ACC_PUBLIC | ACC_STATIC)) == -1)
		errx(EXIT_FAILURE, "could not find main method");
	// TODO: free heap
}

/* java: launches a java application */
int
main(int argc, char *argv[])
{
	char *cpath = NULL;
	int i;

	setprogname(argv[0]);
	cpath = getenv("CLASSPATH");
	for (i = 1; i < argc && argv[i][0] == '-'; i++) {
		if (strcmp(argv[i], "-cp") == 0) {
			if (++i >= argc)
				usage();
			cpath = argv[i];
		} else {
			usage();
		}
	}
	if (i >= argc)
		usage();
	argc -= i;
	argv += i;
	if (cpath == NULL)
		cpath = ".";
	setclasspath(cpath);
	atexit(classfree);
	java(argc, argv);
	return 0;
}
