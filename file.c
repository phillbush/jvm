#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "class.h"
#include "file.h"

#define MAGIC           0xCAFEBABE

#define TRY(expr) \
	do { \
		if ((expr) == -1) { \
			goto error; \
		} \
	} while(0)

/* error tags */
enum {
	ERR_NONE = 0,
	ERR_READ,
	ERR_ALLOC,
	ERR_CODE,
	ERR_CONSTANT,
	ERR_DESCRIPTOR,
	ERR_EOF,
	ERR_INDEX,
	ERR_KIND,
	ERR_MAGIC,
	ERR_TAG,
	ERR_METHOD,
};

/* error variables */
static int errtag = ERR_NONE;
static char *errstr[] = {
	[ERR_NONE] = NULL,
	[ERR_READ] = "could not read file",
	[ERR_ALLOC] = "could not allocate memory",
	[ERR_CODE] = "code does not follow jvm code constraints",
	[ERR_CONSTANT] = "reference to entry of wrong type on constant pool",
	[ERR_DESCRIPTOR] = "invalid descriptor string",
	[ERR_EOF] = "unexpected end of file",
	[ERR_INDEX] = "index to constant pool out of bounds",
	[ERR_KIND] = "invalid method handle reference kind",
	[ERR_MAGIC] = "invalid magic number",
	[ERR_METHOD] = "invalid method name",
	[ERR_TAG] = "unknown constant pool tag",
};

/* call malloc; return -1 on error */
static int
fmalloc(void **p, size_t size)
{
	if ((*p = malloc(size)) == NULL) {
		errtag = ERR_ALLOC;
		return -1;
	}
	return 0;
}

/* call calloc; return -1 on error */
static int
fcalloc(void **p, size_t nmemb, size_t size)
{
	if ((*p = calloc(nmemb, size)) == NULL) {
		errtag = ERR_ALLOC;
		return -1;
	}
	return 0;
}

/* get attribute tag from string */
static AttributeTag
getattributetag(char *tagstr)
{
	static struct {
		AttributeTag t;
		char *s;
	} tags[] = {
		{ConstantValue,      "ConstantValue"},
		{Code,               "Code"},
		{Deprecated,         "Deprecated"},
		{Exceptions,         "Exceptions"},
		{InnerClasses,       "InnerClasses"},
		{SourceFile,         "SourceFile"},
		{Synthetic,          "Synthetic"},
		{LineNumberTable,    "LineNumberTable"},
		{LocalVariableTable, "LocalVariableTable"},
		{UnknownAttribute,   NULL},
	};
	int i;

	for (i = 0; tags[i].s; i++)
		if (strcmp(tagstr, tags[i].s) == 0)
			break;
	return tags[i].t;
}

/* check if string is valid field type, return pointer to next character */
static int
isdescriptor(char *s)
{
	if (*s == '(') {
		s++;
		while (*s && *s != ')') {
			if (*s == 'L') {
				while (*s != '\0' && *s != ';')
					s++;
				if (*s == '\0')
					return 0;
			} else if (!strchr("BCDFIJSZ[", *s)) {
				return 0;
			}
			s++;
		}
		if (*s != ')')
			return 0;
		s++;
	}
	do {
		if (*s == 'L') {
			while (*s != '\0' && *s != ';')
				s++;
			if (*s == '\0')
				return 0;
		} else if (!strchr("BCDFIJSVZ[", *s)) {
			return 0;
		}
	} while (*s++ == '[');
	return 1;
}

/* check if kind of method handle is valid */
static int
checkkind(U1 kind)
{
	if (kind <= REF_none || kind >= REF_last) {
		errtag = ERR_KIND;
		return -1;
	}
	return 0;
}

/* check if index is valid and points to a given tag in the constant pool */
static int
checkindex(CP **cp, U2 count, ConstantTag tag, U2 index)
{
	if (index < 1 || index >= count) {
		errtag = ERR_INDEX;
		return -1;
	}
	switch (tag) {
	case CONSTANT_Untagged:
		break;
	case CONSTANT_Constant:
		if (cp[index]->tag != CONSTANT_Integer &&
		    cp[index]->tag != CONSTANT_Float &&
		    cp[index]->tag != CONSTANT_Long &&
		    cp[index]->tag != CONSTANT_Double &&
		    cp[index]->tag != CONSTANT_String)
			goto error;
		break;
	case CONSTANT_U1:
		if (cp[index]->tag != CONSTANT_Integer &&
		    cp[index]->tag != CONSTANT_Float &&
		    cp[index]->tag != CONSTANT_String)
			goto error;
		break;
	case CONSTANT_U2:
		if (cp[index]->tag != CONSTANT_Long &&
		    cp[index]->tag != CONSTANT_Double)
			goto error;
		break;
	default:
		if (cp[index]->tag != tag)
			goto error;
		break;
	}
	return 0;
error:
	errtag = ERR_CONSTANT;
	return -1;
}

/* check if index is points to a valid descriptor in the constant pool */
static int
checkdescriptor(CP **cp, U2 count, U2 index)
{
	if (index < 1 || index >= count) {
		errtag = ERR_INDEX;
		return -1;
	}
	if (cp[index]->tag != CONSTANT_Utf8) {
		errtag = ERR_CONSTANT;
		return -1;
	}
	if (!isdescriptor(cp[index]->info.utf8_info.bytes)) {
		errtag = ERR_DESCRIPTOR;
		return -1;
	}
	return 0;
}

/* check if method is not special (<init> or <clinit>) */
static int
checkmethod(ClassFile *class, U2 index)
{
	CONSTANT_Methodref_info *methodref;
	char *name, *type;

	methodref = &class->constant_pool[index]->info.methodref_info;
	class_getnameandtype(class, methodref->name_and_type_index, &name, &type);
	if (strcmp(name, "<init>") == 0 || strcmp(name, "<clinit>") == 0) {
		errtag = ERR_METHOD;
		return -1;
	}
	return 0;
}

/* read count bytes into buf */
static int
readb(FILE *fp, void *buf, U4 count)
{
	if (fread(buf, 1, count, fp) != count)
		return -1;
	return 0;
}

/* read unsigned integer of size count into *u */
static int
readu(FILE *fp, void *u, U2 count)
{
	U1 b[4];

	TRY(readb(fp, b, count));
	switch (count) {
	case 4:
		*(U4 *)u = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
		break;
	case 2:
		*(U2 *)u = (b[0] << 8) | b[1];
		break;
	default:
		*(U1 *)u = b[0];
		break;
	}
	return 0;
error:
	return -1;
}

/* read string of length count into buf; insert a nul at the end of it */
static int
reads(FILE *fp, char **s, U2 count)
{
	TRY(fmalloc((void **)s, count + 1));
	TRY(readb(fp, (*s), count));
	(*s)[count] = '\0';
	return 0;
error:
	return -1;
}

/* read index to constant pool and check whether it is a valid index to a given tag */
static int
readindex(FILE *fp, U2 *u, int canbezero, ClassFile *class, ConstantTag tag)
{
	U1 b[2];

	TRY(readb(fp, b, 2));
	*u = (b[0] << 8) | b[1];
	if (!canbezero || *u)
		TRY(checkindex(class->constant_pool, class->constant_pool_count, tag, *u));
	return 0;
error:
	return -1;
}

/* read descriptor index to constant pool and check whether it is a valid */
static int
readdescriptor(FILE *fp, U2 *u, ClassFile *class)
{
	U1 b[2];

	TRY(readb(fp, b, 2));
	*u = (b[0] << 8) | b[1];
	TRY(checkdescriptor(class->constant_pool, class->constant_pool_count, *u));
	return 0;
error:
	return -1;
}

/* read constant pool into *cp */
static int
readcp(FILE *fp, CP ***cp, U2 count)
{
	U2 i;

	if (count == 0) {
		*cp = NULL;
		return 0;
	}
	TRY(fcalloc((void **)cp, count, sizeof(**cp)));
	for (i = 1; i < count; i++) {
		TRY(fcalloc((void **)&(*cp)[i], 1, sizeof(*(*cp)[i])));
		TRY(readu(fp, &(*cp)[i]->tag, 1));
		switch ((*cp)[i]->tag) {
		case CONSTANT_Utf8:
			TRY(readu(fp, &(*cp)[i]->info.utf8_info.length, 2));
			TRY(reads(fp, &(*cp)[i]->info.utf8_info.bytes, (*cp)[i]->info.utf8_info.length));
			break;
		case CONSTANT_Integer:
			TRY(readu(fp, &(*cp)[i]->info.integer_info.bytes, 4));
			break;
		case CONSTANT_Float:
			TRY(readu(fp, &(*cp)[i]->info.float_info.bytes, 4));
			break;
		case CONSTANT_Long:
			TRY(readu(fp, &(*cp)[i]->info.long_info.high_bytes, 4));
			TRY(readu(fp, &(*cp)[i]->info.long_info.low_bytes, 4));
			i++;
			break;
		case CONSTANT_Double:
			TRY(readu(fp, &(*cp)[i]->info.double_info.high_bytes, 4));
			TRY(readu(fp, &(*cp)[i]->info.double_info.low_bytes, 4));
			i++;
			break;
		case CONSTANT_Class:
			TRY(readu(fp, &(*cp)[i]->info.class_info.name_index, 2));
			break;
		case CONSTANT_String:
			TRY(readu(fp, &(*cp)[i]->info.string_info.string_index, 2));
			break;
		case CONSTANT_Fieldref:
			TRY(readu(fp, &(*cp)[i]->info.fieldref_info.class_index, 2));
			TRY(readu(fp, &(*cp)[i]->info.fieldref_info.name_and_type_index, 2));
			break;
		case CONSTANT_Methodref:
			TRY(readu(fp, &(*cp)[i]->info.methodref_info.class_index, 2));
			TRY(readu(fp, &(*cp)[i]->info.methodref_info.name_and_type_index, 2));
			break;
		case CONSTANT_InterfaceMethodref:
			TRY(readu(fp, &(*cp)[i]->info.interfacemethodref_info.class_index, 2));
			TRY(readu(fp, &(*cp)[i]->info.interfacemethodref_info.name_and_type_index, 2));
			break;
		case CONSTANT_NameAndType:
			TRY(readu(fp, &(*cp)[i]->info.nameandtype_info.name_index, 2));
			TRY(readu(fp, &(*cp)[i]->info.nameandtype_info.descriptor_index, 2));
			break;
		case CONSTANT_MethodHandle:
			TRY(readu(fp, &(*cp)[i]->info.methodhandle_info.reference_kind, 1));
			TRY(readu(fp, &(*cp)[i]->info.methodhandle_info.reference_index, 2));
			break;
		case CONSTANT_MethodType:
			TRY(readu(fp, &(*cp)[i]->info.methodtype_info.descriptor_index, 2));
			break;
		case CONSTANT_InvokeDynamic:
			TRY(readu(fp, &(*cp)[i]->info.invokedynamic_info.bootstrap_method_attr_index, 2));
			TRY(readu(fp, &(*cp)[i]->info.invokedynamic_info.name_and_type_index, 2));
			break;
		default:
			errtag = ERR_TAG;
			goto error;
		}
	}
	for (i = 1; i < count; i++) {
		switch ((*cp)[i]->tag) {
		case CONSTANT_Utf8:
		case CONSTANT_Integer:
		case CONSTANT_Float:
			break;
		case CONSTANT_Long:
		case CONSTANT_Double:
			i++;
			break;
		case CONSTANT_Class:
			break;
		case CONSTANT_String:
			TRY(checkindex((*cp), count, CONSTANT_Utf8, (*cp)[i]->info.string_info.string_index));
			(*cp)[i]->info.string_info.string = (*cp)[(*cp)[i]->info.string_info.string_index]->info.utf8_info.bytes;
			break;
		case CONSTANT_Fieldref:
			TRY(checkindex((*cp), count, CONSTANT_Class, (*cp)[i]->info.fieldref_info.class_index));
			TRY(checkindex((*cp), count, CONSTANT_NameAndType, (*cp)[i]->info.fieldref_info.name_and_type_index));
			break;
		case CONSTANT_Methodref:
			TRY(checkindex((*cp), count, CONSTANT_Class, (*cp)[i]->info.methodref_info.class_index));
			TRY(checkindex((*cp), count, CONSTANT_NameAndType, (*cp)[i]->info.methodref_info.name_and_type_index));
			break;
		case CONSTANT_InterfaceMethodref:
			TRY(checkindex((*cp), count, CONSTANT_Class, (*cp)[i]->info.interfacemethodref_info.class_index));
			TRY(checkindex((*cp), count, CONSTANT_NameAndType, (*cp)[i]->info.interfacemethodref_info.name_and_type_index));
			break;
		case CONSTANT_NameAndType:
			TRY(checkindex((*cp), count, CONSTANT_Utf8, (*cp)[i]->info.nameandtype_info.name_index));
			TRY(checkdescriptor((*cp), count, (*cp)[i]->info.nameandtype_info.descriptor_index));
			break;
		case CONSTANT_MethodHandle:
			TRY(checkkind((*cp)[i]->info.methodhandle_info.reference_kind));
			switch ((*cp)[i]->info.methodhandle_info.reference_kind) {
			case REF_getField:
			case REF_getStatic:
			case REF_putField:
			case REF_putStatic:
				TRY(checkindex((*cp), count, CONSTANT_Fieldref, (*cp)[i]->info.methodhandle_info.reference_index));
				break;
			case REF_invokeVirtual:
			case REF_newInvokeSpecial:
				TRY(checkindex((*cp), count, CONSTANT_Methodref, (*cp)[i]->info.methodhandle_info.reference_index));
				break;
			case REF_invokeStatic:
			case REF_invokeSpecial:
				/* TODO check based on ClassFile version */
				break;
			case REF_invokeInterface:
				TRY(checkindex((*cp), count, CONSTANT_InterfaceMethodref, (*cp)[i]->info.methodhandle_info.reference_index));
				break;
			}
			break;
		case CONSTANT_MethodType:
			TRY(checkdescriptor((*cp), count, (*cp)[i]->info.methodtype_info.descriptor_index));
			break;
		case CONSTANT_InvokeDynamic:
			TRY(checkindex((*cp), count, CONSTANT_NameAndType, (*cp)[i]->info.invokedynamic_info.name_and_type_index));
			break;
		default:
			break;
		}
	}
	return 0;
error:
	return -1;
}

/* read interface indices into *p */
static int
readinterfaces(FILE *fp, U2 **p, U2 count)
{
	U2 i;

	if (count == 0) {
		*p = NULL;
		return 0;
	}
	TRY(fcalloc((void **)p, count, sizeof(**p)));
	for (i = 0; i < count; i++)
		TRY(readu(fp, &(*p)[i], 2));
	return 0;
error:
	return -1;
}

/* read code instructions into *code */
static int
readcode(FILE *fp, U1 **code, ClassFile *class, U4 count)
{
	int32_t j, npairs, off, high, low;
	I8 base, i;
	U2 u;

	if (count == 0) {
		*code = NULL;
		return 0;
	}
	TRY(fmalloc((void **)code, count));
	for (i = 0; i < count; i++) {
		TRY(readu(fp, &(*code)[i], 1));
		if ((*code)[i] >= CODE_LAST)
			goto error;
		switch ((*code)[i]) {
		case WIDE:
			TRY(readu(fp, &(*code)[++i], 1));
			switch ((*code)[i]) {
			case IINC:
				TRY(readu(fp, &(*code)[++i], 1));
				TRY(readu(fp, &(*code)[++i], 1));
				/* FALLTHROUGH */
			case ILOAD:
			case FLOAD:
			case ALOAD:
			case LLOAD:
			case DLOAD:
			case ISTORE:
			case FSTORE:
			case ASTORE:
			case LSTORE:
			case DSTORE:
			case RET:
				TRY(readu(fp, &(*code)[++i], 1));
				TRY(readu(fp, &(*code)[++i], 1));
				break;
			default:
				goto error;
				break;
			}
			break;
		case LOOKUPSWITCH:
			while ((3 - (i % 4)) > 0)
				TRY(readu(fp, &(*code)[++i], 1));
			for (j = 0; j < 8; j++)
				TRY(readu(fp, &(*code)[++i], 1));
			npairs = ((*code)[i-3] << 24) | ((*code)[i-2] << 16) | ((*code)[i-1] << 8) | (*code)[i];
			if (npairs < 0)
				goto error;
			for (j = 8 * npairs; j > 0; j--)
				TRY(readu(fp, &(*code)[++i], 1));
			break;
		case TABLESWITCH:
			base = i;
			while ((3 - (i % 4)) > 0)
				TRY(readu(fp, &(*code)[++i], 1));
			for (j = 0; j < 12; j++)
				TRY(readu(fp, &(*code)[++i], 1));
			off = ((*code)[i-11] << 24) | ((*code)[i-10] << 16) | ((*code)[i-9] << 8) | (*code)[i-8];
			low = ((*code)[i-7] << 24) | ((*code)[i-6] << 16) | ((*code)[i-5] << 8) | (*code)[i-4];
			high = ((*code)[i-3] << 24) | ((*code)[i-2] << 16) | ((*code)[i-1] << 8) | (*code)[i];
			if (base + off < 0 || base + off >= count)
				goto error;
			if (low > high)
				goto error;
			for (j = low; j <= high; j++) {
				TRY(readu(fp, &(*code)[++i], 1));
				TRY(readu(fp, &(*code)[++i], 1));
				TRY(readu(fp, &(*code)[++i], 1));
				TRY(readu(fp, &(*code)[++i], 1));
				off = ((*code)[i-3] << 24) | ((*code)[i-2] << 16) | ((*code)[i-1] << 8) | (*code)[i];
				if (base + off < 0 || base + off >= count) {
					goto error;
				}
			}
			break;
		case LDC:
			TRY(readu(fp, &(*code)[++i], 1));
			TRY(checkindex(class->constant_pool, class->constant_pool_count, CONSTANT_U1, (*code)[i]));
			break;
		case LDC_W:
			TRY(readu(fp, &(*code)[++i], 1));
			TRY(readu(fp, &(*code)[++i], 1));
			TRY(checkindex(class->constant_pool, class->constant_pool_count, CONSTANT_U1, (*code)[i - 1] << 8 | (*code)[i]));
			break;
		case LDC2_W:
			TRY(readu(fp, &(*code)[++i], 1));
			TRY(readu(fp, &(*code)[++i], 1));
			TRY(checkindex(class->constant_pool, class->constant_pool_count, CONSTANT_U2, (*code)[i - 1] << 8 | (*code)[i]));
			break;
		case GETSTATIC: case PUTSTATIC: case GETFIELD: case PUTFIELD:
			TRY(readu(fp, &(*code)[++i], 1));
			TRY(readu(fp, &(*code)[++i], 1));
			TRY(checkindex(class->constant_pool, class->constant_pool_count, CONSTANT_Fieldref, (*code)[i - 1] << 8 | (*code)[i]));
			break;
		case INVOKESTATIC:
			TRY(readu(fp, &(*code)[++i], 1));
			TRY(readu(fp, &(*code)[++i], 1));
			u = (*code)[i - 1] << 8 | (*code)[i];
			TRY(checkindex(class->constant_pool, class->constant_pool_count, CONSTANT_Methodref, u));
			TRY(checkmethod(class, u));
			break;
		case MULTIANEWARRAY:
			TRY(readu(fp, &(*code)[++i], 1));
			TRY(readu(fp, &(*code)[++i], 1));
			u = (*code)[i - 1] << 8 | (*code)[i];
			TRY(checkindex(class->constant_pool, class->constant_pool_count, CONSTANT_Class, u));
			TRY(readu(fp, &(*code)[++i], 1));
			if ((*code)[i] < 1)
				goto error;
			break;
		default:
			for (j = class_getnoperands((*code)[i]); j > 0; j--)
				TRY(readu(fp, &(*code)[++i], 1));
			break;
		}
	}
	if (i != count)
		goto error;
	return 0;
error:
	return -1;
}

/* read indices to constant pool into *p */
static int
readindices(FILE *fp, U2 **indices, U2 count)
{
	U2 i;

	if (count == 0) {
		*indices = NULL;
		return 0;
	}
	TRY(fcalloc((void **)indices, count, sizeof(**indices)));
	for (i = 0; i < count; i++)
		TRY(readu(fp, &(*indices)[i], 2));
	return 0;
error:
	return -1;
}

/* read exception table into *p */
static int
readexceptions(FILE *fp, Exception ***p, U2 count)
{
	U2 i;

	if (count == 0) {
		*p = NULL;
		return 0;
	}
	TRY(fcalloc((void **)p, count, sizeof(*(*p))));
	for (i = 0; i < count; i++) {
		TRY(fcalloc((void **)&(*p)[i], 1, sizeof(*(*p)[i])));
		TRY(readu(fp, &(*p)[i]->start_pc, 2));
		TRY(readu(fp, &(*p)[i]->end_pc, 2));
		TRY(readu(fp, &(*p)[i]->handler_pc, 2));
		TRY(readu(fp, &(*p)[i]->catch_type, 2));
	}
	return 0;
error:
	return -1;
}

/* read inner class table into *p */
static int
readclasses(FILE *fp, InnerClass ***p, ClassFile *class, U2 count)
{
	U2 i;

	if (count == 0) {
		*p = NULL;
		return 0;
	}
	TRY(fcalloc((void **)p, count, sizeof(*(*p))));
	for (i = 0; i < count; i++) {
		TRY(fcalloc((void **)&(*p)[i], 1, sizeof(*(*p)[i])));
		TRY(readindex(fp, &(*p)[i]->inner_class_info_index, 0, class, CONSTANT_Class));
		TRY(readindex(fp, &(*p)[i]->outer_class_info_index, 1, class, CONSTANT_Class));
		TRY(readindex(fp, &(*p)[i]->inner_name_index, 1, class, CONSTANT_Utf8));
		TRY(readu(fp, &(*p)[i]->inner_class_access_flags, 2));
	}
	return 0;
error:
	return -1;
}

/* read line number table into *p */
static int
readlinenumber(FILE *fp, LineNumber ***p, U2 count)
{
	U2 i;

	if (count == 0) {
		*p = NULL;
		return 0;
	}
	TRY(fcalloc((void **)p, count, sizeof(*(*p))));
	for (i = 0; i < count; i++) {
		TRY(fcalloc((void **)&(*p)[i], 1, sizeof(*(*p)[i])));
		TRY(readu(fp, &(*p)[i]->start_pc, 2));
		TRY(readu(fp, &(*p)[i]->line_number, 2));
	}
	return 0;
error:
	return -1;
}

/* read local variable table into *p */
static int
readlocalvariable(FILE *fp, LocalVariable ***p, ClassFile *class, U2 count)
{
	U2 i;

	if (count == 0) {
		*p = NULL;
		return 0;
	}
	TRY(fcalloc((void **)p, count, sizeof *(*p)));
	for (i = 0; i < count; i++) {
		TRY(fcalloc((void **)&(*p)[i], 1, sizeof *(*p)[i]));
		TRY(readu(fp, &(*p)[i]->start_pc, 2));
		TRY(readu(fp, &(*p)[i]->length, 2));
		TRY(readindex(fp, &(*p)[i]->name_index, 0, class, CONSTANT_Utf8));
		TRY(readdescriptor(fp, &(*p)[i]->descriptor_index, class));
		TRY(readu(fp, &(*p)[i]->index, 2));
	}
	return 0;
error:
	return -1;
}

/* read attribute list into *p */
static int
readattributes(FILE *fp, Attribute ***p, ClassFile *class, U2 count)
{
	U4 length;
	U2 index;
	U2 i;
	U1 b;

	if (count == 0) {
		*p = NULL;
		return 0;
	}
	TRY(fcalloc((void **)p, count, sizeof(**p)));
	for (i = 0; i < count; i++) {
		TRY(fcalloc((void **)&(*p)[i], 1, sizeof(*(*p)[i])));
		TRY(readindex(fp, &index, 0, class, CONSTANT_Utf8));
		TRY(readu(fp, &length, 4));
		(*p)[i]->tag = getattributetag(class->constant_pool[index]->info.utf8_info.bytes);
		switch ((*p)[i]->tag) {
		case ConstantValue:
			TRY(readindex(fp, &(*p)[i]->info.constantvalue.constantvalue_index, 0, class, CONSTANT_Constant));
			break;
		case Code:
			TRY(readu(fp, &(*p)[i]->info.code.max_stack, 2));
			TRY(readu(fp, &(*p)[i]->info.code.max_locals, 2));
			TRY(readu(fp, &(*p)[i]->info.code.code_length, 4));
			TRY(readcode(fp, &(*p)[i]->info.code.code, class, (*p)[i]->info.code.code_length));
			TRY(readu(fp, &(*p)[i]->info.code.exception_table_length, 2));
			TRY(readexceptions(fp, &(*p)[i]->info.code.exception_table, (*p)[i]->info.code.exception_table_length));
			TRY(readu(fp, &(*p)[i]->info.code.attributes_count, 2));
			TRY(readattributes(fp, &(*p)[i]->info.code.attributes, class, (*p)[i]->info.code.attributes_count));
			break;
		case Deprecated:
			break;
		case Exceptions:
			TRY(readu(fp, &(*p)[i]->info.exceptions.number_of_exceptions, 2));
			TRY(readindices(fp, &(*p)[i]->info.exceptions.exception_index_table, (*p)[i]->info.exceptions.number_of_exceptions));
			break;
		case InnerClasses:
			TRY(readu(fp, &(*p)[i]->info.innerclasses.number_of_classes, 2));
			TRY(readclasses(fp, &(*p)[i]->info.innerclasses.classes, class, (*p)[i]->info.innerclasses.number_of_classes));
			break;
		case SourceFile:
			TRY(readindex(fp, &(*p)[i]->info.sourcefile.sourcefile_index, 0, class, CONSTANT_Utf8));
			break;
		case Synthetic:
			break;
		case LineNumberTable:
			TRY(readu(fp, &(*p)[i]->info.linenumbertable.line_number_table_length, 2));
			TRY(readlinenumber(fp, &(*p)[i]->info.linenumbertable.line_number_table, (*p)[i]->info.linenumbertable.line_number_table_length));
			break;
		case LocalVariableTable:
			TRY(readu(fp, &(*p)[i]->info.localvariabletable.local_variable_table_length, 2));
			TRY(readlocalvariable(fp, &(*p)[i]->info.localvariabletable.local_variable_table, class, (*p)[i]->info.localvariabletable.local_variable_table_length));
			break;
		case UnknownAttribute:
			while (length-- > 0)
				TRY(readb(fp, &b, 1));
			break;
		}
	}
	return 0;
error:
	return -1;
}

/* read fields into *p */
static int
readfields(FILE *fp, Field ***p, ClassFile *class, U2 count)
{
	U2 i;

	if (count == 0) {
		*p = NULL;
		return 0;
	}
	TRY(fcalloc((void **)p, count, sizeof(**p)));
	for (i = 0; i < count; i++) {
		TRY(fcalloc((void **)&(*p)[i], 1, sizeof(*(*p)[i])));
		TRY(readu(fp, &(*p)[i]->access_flags, 2));
		TRY(readindex(fp, &(*p)[i]->name_index, 0, class, CONSTANT_Utf8));
		TRY(readdescriptor(fp, &(*p)[i]->descriptor_index, class));
		TRY(readu(fp, &(*p)[i]->attributes_count, 2));
		TRY(readattributes(fp, &(*p)[i]->attributes, class, (*p)[i]->attributes_count));
	}
	return 0;
error:
	return -1;
}

/* read methods into *p */
static int
readmethods(FILE *fp, Method ***p, ClassFile *class, U2 count)
{
	U2 i;

	if (count == 0) {
		*p = NULL;
		return 0;
	}
	TRY(fcalloc((void **)p, count, sizeof(**p)));
	for (i = 0; i < count; i++) {
		TRY(fcalloc((void **)&(*p)[i], 1, sizeof(*(*p)[i])));
		TRY(readu(fp, &(*p)[i]->access_flags, 2));
		TRY(readindex(fp, &(*p)[i]->name_index, 0, class, CONSTANT_Utf8));
		TRY(readdescriptor(fp, &(*p)[i]->descriptor_index, class));
		TRY(readu(fp, &(*p)[i]->attributes_count, 2));
		TRY(readattributes(fp, &(*p)[i]->attributes, class, (*p)[i]->attributes_count));
	}
	return 0;
error:
	return -1;
}

/* free attribute */
static void
attributefree(Attribute **attr, U2 count)
{
	U2 i, j;

	if (attr == NULL)
		return;
	for (i = 0; i < count; i++) {
		switch (attr[i]->tag) {
		case UnknownAttribute:
		case ConstantValue:
		case Deprecated:
		case SourceFile:
		case Synthetic:
			break;
		case Code:
			free(attr[i]->info.code.code);
			free(attr[i]->info.code.exception_table);
			attributefree(attr[i]->info.code.attributes, attr[i]->info.code.attributes_count);
			break;
		case Exceptions:
			free(attr[i]->info.exceptions.exception_index_table);
			break;
		case InnerClasses:
			if (attr[i]->info.innerclasses.classes != NULL)
				for (j = 0; j < attr[i]->info.innerclasses.number_of_classes; j++)
					free(attr[i]->info.innerclasses.classes[j]);
			free(attr[i]->info.innerclasses.classes);
			break;
		case LineNumberTable:
			if (attr[i]->info.linenumbertable.line_number_table != 0)
				for (j = 0; j < attr[i]->info.linenumbertable.line_number_table_length; j++)
					free(attr[i]->info.linenumbertable.line_number_table[j]);
			free(attr[i]->info.linenumbertable.line_number_table);
			break;
		case LocalVariableTable:
			if (attr[i]->info.localvariabletable.local_variable_table != 0)
				for (j = 0; j < attr[i]->info.localvariabletable.local_variable_table_length; j++)
					free(attr[i]->info.localvariabletable.local_variable_table[j]);
			free(attr[i]->info.localvariabletable.local_variable_table);
			break;
		}
	}
	free(attr);
}

/* free class structure */
void
file_free(ClassFile *class)
{
	U2 i;

	if (class == NULL)
		return;
	if (class->constant_pool) {
		for (i = 1; i < class->constant_pool_count; i++) {
			switch (class->constant_pool[i]->tag) {
			case CONSTANT_Utf8:
				free(class->constant_pool[i]->info.utf8_info.bytes);
				break;
			case CONSTANT_Double:
			case CONSTANT_Long:
				i++;
				break;
			default:
				break;
			}
		}
	}
	free(class->constant_pool);
	free(class->interfaces);
	if (class->fields)
		for (i = 0; i < class->fields_count; i++)
			attributefree(class->fields[i]->attributes, class->fields[i]->attributes_count);
	free(class->fields);
	if (class->methods)
		for (i = 0; i < class->methods_count; i++)
			attributefree(class->methods[i]->attributes, class->methods[i]->attributes_count);
	free(class->methods);
	attributefree(class->attributes, class->attributes_count);
}

/* read class file */
int
file_read(FILE *fp, ClassFile *class)
{
	U4 magic;

	TRY(readu(fp, &magic, 4));
	if (magic != MAGIC) {
		errtag = ERR_MAGIC;
		goto error;
	}
	class->init = 0;
	class->next = NULL;
	class->super = NULL;
	TRY(readu(fp, &class->minor_version, 2));
	TRY(readu(fp, &class->major_version, 2));
	TRY(readu(fp, &class->constant_pool_count, 2));
	TRY(readcp(fp, &class->constant_pool, class->constant_pool_count));
	TRY(readu(fp, &class->access_flags, 2));
	TRY(readu(fp, &class->this_class, 2));
	TRY(readu(fp, &class->super_class, 2));
	TRY(readu(fp, &class->interfaces_count, 2));
	TRY(readinterfaces(fp, &class->interfaces, class->interfaces_count));
	TRY(readu(fp, &class->fields_count, 2));
	TRY(readfields(fp, &class->fields, class, class->fields_count));
	TRY(readu(fp, &class->methods_count, 2));
	TRY(readmethods(fp, &class->methods, class, class->methods_count));
	TRY(readu(fp, &class->attributes_count, 2));
	TRY(readattributes(fp, &class->attributes, class, class->attributes_count));
	return ERR_NONE;
error:
	file_free(class);
	return errtag;
}

/* return string describing error tag */
char *
file_errstr(int i)
{
	return errstr[i];
}
