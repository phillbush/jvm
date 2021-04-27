#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "util.h"
#include "class.h"

static int noperands[] = {
	[NOP]             = 0,
	[ACONST_NULL]     = 0,
	[ICONST_M1]       = 0,
	[ICONST_0]        = 0,
	[ICONST_1]        = 0,
	[ICONST_2]        = 0,
	[ICONST_3]        = 0,
	[ICONST_4]        = 0,
	[ICONST_5]        = 0,
	[LCONST_0]        = 0,
	[LCONST_1]        = 0,
	[FCONST_0]        = 0,
	[FCONST_1]        = 0,
	[FCONST_2]        = 0,
	[DCONST_0]        = 0,
	[DCONST_1]        = 0,
	[BIPUSH]          = 1,
	[SIPUSH]          = 2,
	[LDC]             = 1,
	[LDC_W]           = 2,
	[LDC2_W]          = 2,
	[ILOAD]           = 1,
	[LLOAD]           = 1,
	[FLOAD]           = 1,
	[DLOAD]           = 1,
	[ALOAD]           = 1,
	[ILOAD_0]         = 0,
	[ILOAD_1]         = 0,
	[ILOAD_2]         = 0,
	[ILOAD_3]         = 0,
	[LLOAD_0]         = 0,
	[LLOAD_1]         = 0,
	[LLOAD_2]         = 0,
	[LLOAD_3]         = 0,
	[FLOAD_0]         = 0,
	[FLOAD_1]         = 0,
	[FLOAD_2]         = 0,
	[FLOAD_3]         = 0,
	[DLOAD_0]         = 0,
	[DLOAD_1]         = 0,
	[DLOAD_2]         = 0,
	[DLOAD_3]         = 0,
	[ALOAD_0]         = 0,
	[ALOAD_1]         = 0,
	[ALOAD_2]         = 0,
	[ALOAD_3]         = 0,
	[IALOAD]          = 0,
	[LALOAD]          = 0,
	[FALOAD]          = 0,
	[DALOAD]          = 0,
	[AALOAD]          = 0,
	[BALOAD]          = 0,
	[CALOAD]          = 0,
	[SALOAD]          = 0,
	[ISTORE]          = 1,
	[LSTORE]          = 1,
	[FSTORE]          = 1,
	[DSTORE]          = 1,
	[ASTORE]          = 1,
	[ISTORE_0]        = 0,
	[ISTORE_1]        = 0,
	[ISTORE_2]        = 0,
	[ISTORE_3]        = 0,
	[LSTORE_0]        = 0,
	[LSTORE_1]        = 0,
	[LSTORE_2]        = 0,
	[LSTORE_3]        = 0,
	[FSTORE_0]        = 0,
	[FSTORE_1]        = 0,
	[FSTORE_2]        = 0,
	[FSTORE_3]        = 0,
	[DSTORE_0]        = 0,
	[DSTORE_1]        = 0,
	[DSTORE_2]        = 0,
	[DSTORE_3]        = 0,
	[ASTORE_0]        = 0,
	[ASTORE_1]        = 0,
	[ASTORE_2]        = 0,
	[ASTORE_3]        = 0,
	[IASTORE]         = 0,
	[LASTORE]         = 0,
	[FASTORE]         = 0,
	[DASTORE]         = 0,
	[AASTORE]         = 0,
	[BASTORE]         = 0,
	[CASTORE]         = 0,
	[SASTORE]         = 0,
	[POP]             = 0,
	[POP2]            = 0,
	[DUP]             = 0,
	[DUP_X1]          = 0,
	[DUP_X2]          = 0,
	[DUP2]            = 0,
	[DUP2_X1]         = 0,
	[DUP2_X2]         = 0,
	[SWAP]            = 0,
	[IADD]            = 0,
	[LADD]            = 0,
	[FADD]            = 0,
	[DADD]            = 0,
	[ISUB]            = 0,
	[LSUB]            = 0,
	[FSUB]            = 0,
	[DSUB]            = 0,
	[IMUL]            = 0,
	[LMUL]            = 0,
	[FMUL]            = 0,
	[DMUL]            = 0,
	[IDIV]            = 0,
	[LDIV]            = 0,
	[FDIV]            = 0,
	[DDIV]            = 0,
	[IREM]            = 0,
	[LREM]            = 0,
	[FREM]            = 0,
	[DREM]            = 0,
	[INEG]            = 0,
	[LNEG]            = 0,
	[FNEG]            = 0,
	[DNEG]            = 0,
	[ISHL]            = 0,
	[LSHL]            = 0,
	[ISHR]            = 0,
	[LSHR]            = 0,
	[IUSHR]           = 0,
	[LUSHR]           = 0,
	[IAND]            = 0,
	[LAND]            = 0,
	[IOR]             = 0,
	[LOR]             = 0,
	[IXOR]            = 0,
	[LXOR]            = 0,
	[IINC]            = 2,
	[I2L]             = 0,
	[I2F]             = 0,
	[I2D]             = 0,
	[L2I]             = 0,
	[L2F]             = 0,
	[L2D]             = 0,
	[F2I]             = 0,
	[F2L]             = 0,
	[F2D]             = 0,
	[D2I]             = 0,
	[D2L]             = 0,
	[D2F]             = 0,
	[I2B]             = 0,
	[I2C]             = 0,
	[I2S]             = 0,
	[LCMP]            = 0,
	[FCMPL]           = 0,
	[FCMPG]           = 0,
	[DCMPL]           = 0,
	[DCMPG]           = 0,
	[IFEQ]            = 2,
	[IFNE]            = 2,
	[IFLT]            = 2,
	[IFGE]            = 2,
	[IFGT]            = 2,
	[IFLE]            = 2,
	[IF_ICMPEQ]       = 2,
	[IF_ICMPNE]       = 2,
	[IF_ICMPLT]       = 2,
	[IF_ICMPGE]       = 2,
	[IF_ICMPGT]       = 2,
	[IF_ICMPLE]       = 2,
	[IF_ACMPEQ]       = 2,
	[IF_ACMPNE]       = 2,
	[GOTO]            = 2,
	[JSR]             = 2,
	[RET]             = 1,
	[TABLESWITCH]     = OP_TABLESWITCH,
	[LOOKUPSWITCH]    = OP_LOOKUPSWITCH,
	[IRETURN]         = 0,
	[LRETURN]         = 0,
	[FRETURN]         = 0,
	[DRETURN]         = 0,
	[ARETURN]         = 0,
	[RETURN]          = 0,
	[GETSTATIC]       = 2,
	[PUTSTATIC]       = 2,
	[GETFIELD]        = 2,
	[PUTFIELD]        = 2,
	[INVOKEVIRTUAL]   = 2,
	[INVOKESPECIAL]   = 2,
	[INVOKESTATIC]    = 2,
	[INVOKEINTERFACE] = 4,
	[INVOKEDYNAMIC]   = 4,
	[NEW]             = 2,
	[NEWARRAY]        = 1,
	[ANEWARRAY]       = 2,
	[ARRAYLENGTH]     = 0,
	[ATHROW]          = 0,
	[CHECKCAST]       = 2,
	[INSTANCEOF]      = 2,
	[MONITORENTER]    = 0,
	[MONITOREXIT]     = 0,
	[WIDE]            = OP_WIDE,
	[MULTIANEWARRAY]  = 3,
	[IFNULL]          = 2,
	[IFNONNULL]       = 2,
	[GOTO_W]          = 4,
	[JSR_W]           = 4,
};

/* get number of operands of a given instruction */
int
class_getnoperands(U1 instruction)
{
	return noperands[instruction];
}

/* get attribute with given tag in list of attributes */
Attribute *
class_getattr(Attribute *attrs, U2 count, AttributeTag tag)
{
	U2 i;

	for (i = 0; i < count; i++)
		if (attrs[i].tag == tag)
			return &attrs[i];
	return NULL;
}

/* get string from constant pool */
char *
class_getutf8(ClassFile *class, U2 index)
{
	return class->constant_pool[index].info.utf8_info.bytes;
}

/* get string from constant pool */
char *
class_getclassname(ClassFile *class, U2 index)
{
	return class_getutf8(class, class->constant_pool[index].info.class_info.name_index);
}

/* get string from string reference */
char *
class_getstring(ClassFile *class, U2 index)
{
	return class_getutf8(class, class->constant_pool[index].info.string_info.string_index);
}

/* get int32_t from integer reference */
int32_t
class_getinteger(ClassFile *class, U2 index)
{
	return getint(class->constant_pool[index].info.integer_info.bytes);
}

/* get float from float reference */
float
class_getfloat(ClassFile *class, U2 index)
{
	return getfloat(class->constant_pool[index].info.integer_info.bytes);
}

/* get int64_t from long reference */
int64_t
class_getlong(ClassFile *class, U2 index)
{
	return getlong(class->constant_pool[index].info.long_info.high_bytes,
	               class->constant_pool[index].info.long_info.low_bytes);
}

/* get double from double reference */
double
class_getdouble(ClassFile *class, U2 index)
{
	return getdouble(class->constant_pool[index].info.long_info.high_bytes,
	                 class->constant_pool[index].info.long_info.low_bytes);
}

/* get name and type of field or method */
void
class_getnameandtype(ClassFile *class, U2 index, char **name, char **type)
{
	*name = class_getutf8(class, class->constant_pool[index].info.nameandtype_info.name_index);
	*type = class_getutf8(class, class->constant_pool[index].info.nameandtype_info.descriptor_index);
}

/* get method matching name and descriptor from class */
Method *
class_getmethod(ClassFile *class, char *name, char *descr)
{
	U2 i;

	for (i = 0; i < class->methods_count; i++)
		if (strcmp(name, class_getutf8(class, class->methods[i].name_index)) == 0 &&
		    strcmp(descr, class_getutf8(class, class->methods[i].descriptor_index)) == 0)
			return &class->methods[i];
	return NULL;
}

/* get field matching name and descriptor from class */
Field *
class_getfield(ClassFile *class, char *name, char *descr)
{
	U2 i;

	for (i = 0; i < class->fields_count; i++)
		if (strcmp(name, class_getutf8(class, class->fields[i].name_index)) == 0 &&
		    strcmp(descr, class_getutf8(class, class->fields[i].descriptor_index)) == 0)
			return &class->fields[i];
	return NULL;
}

/* check if super class is java.lang.Object */
int
class_istopclass(ClassFile *class)
{
	return strcmp(class_getclassname(class, class->super_class), "java/lang/Object") == 0;
}
