#include "class.h"
#include "file.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CPINDEX 26 /* columns before index in the constant_pool section */
#define CPCOMMENT                                                              \
  43                   /* columns before comments in the constant_pool section \
                        */
#define CODEINDEX 25   /* columns before index in the code section */
#define CODECOMMENT 45 /* columns before comments in the code section */

/* names */
static char *cptags[] = {
    [CONSTANT_Untagged] = "",
    [CONSTANT_Utf8] = "Utf8",
    [CONSTANT_Integer] = "Integer",
    [CONSTANT_Float] = "Float",
    [CONSTANT_Long] = "Long",
    [CONSTANT_Double] = "Double",
    [CONSTANT_Class] = "Class",
    [CONSTANT_String] = "String",
    [CONSTANT_Fieldref] = "Fieldref",
    [CONSTANT_Methodref] = "Methodref",
    [CONSTANT_InterfaceMethodref] = "InterfaceMethodref",
    [CONSTANT_NameAndType] = "NameAndType",
    [CONSTANT_MethodHandle] = "MethodHandle",
    [CONSTANT_MethodType] = "MethodType",
    [CONSTANT_InvokeDynamic] = "InvokeDynamic",
};
static char *instrnames[CODE_LAST] = {
    [NOP] = "nop",
    [ACONST_NULL] = "aconst_null",
    [ICONST_M1] = "iconst_m1",
    [ICONST_0] = "iconst_0",
    [ICONST_1] = "iconst_1",
    [ICONST_2] = "iconst_2",
    [ICONST_3] = "iconst_3",
    [ICONST_4] = "iconst_4",
    [ICONST_5] = "iconst_5",
    [LCONST_0] = "lconst_0",
    [LCONST_1] = "lconst_1",
    [FCONST_0] = "fconst_0",
    [FCONST_1] = "fconst_1",
    [FCONST_2] = "fconst_2",
    [DCONST_0] = "dconst_0",
    [DCONST_1] = "dconst_1",
    [BIPUSH] = "bipush",
    [SIPUSH] = "sipush",
    [LDC] = "ldc",
    [LDC_W] = "ldc_w",
    [LDC2_W] = "ldc2_w",
    [ILOAD] = "iload",
    [LLOAD] = "lload",
    [FLOAD] = "fload",
    [DLOAD] = "dload",
    [ALOAD] = "aload",
    [ILOAD_0] = "iload_0",
    [ILOAD_1] = "iload_1",
    [ILOAD_2] = "iload_2",
    [ILOAD_3] = "iload_3",
    [LLOAD_0] = "lload_0",
    [LLOAD_1] = "lload_1",
    [LLOAD_2] = "lload_2",
    [LLOAD_3] = "lload_3",
    [FLOAD_0] = "fload_0",
    [FLOAD_1] = "fload_1",
    [FLOAD_2] = "fload_2",
    [FLOAD_3] = "fload_3",
    [DLOAD_0] = "dload_0",
    [DLOAD_1] = "dload_1",
    [DLOAD_2] = "dload_2",
    [DLOAD_3] = "dload_3",
    [ALOAD_0] = "aload_0",
    [ALOAD_1] = "aload_1",
    [ALOAD_2] = "aload_2",
    [ALOAD_3] = "aload_3",
    [IALOAD] = "iaload",
    [LALOAD] = "laload",
    [FALOAD] = "faload",
    [DALOAD] = "daload",
    [AALOAD] = "aaload",
    [BALOAD] = "baload",
    [CALOAD] = "caload",
    [SALOAD] = "saload",
    [ISTORE] = "istore",
    [LSTORE] = "lstore",
    [FSTORE] = "fstore",
    [DSTORE] = "dstore",
    [ASTORE] = "astore",
    [ISTORE_0] = "istore_0",
    [ISTORE_1] = "istore_1",
    [ISTORE_2] = "istore_2",
    [ISTORE_3] = "istore_3",
    [LSTORE_0] = "lstore_0",
    [LSTORE_1] = "lstore_1",
    [LSTORE_2] = "lstore_2",
    [LSTORE_3] = "lstore_3",
    [FSTORE_0] = "fstore_0",
    [FSTORE_1] = "fstore_1",
    [FSTORE_2] = "fstore_2",
    [FSTORE_3] = "fstore_3",
    [DSTORE_0] = "dstore_0",
    [DSTORE_1] = "dstore_1",
    [DSTORE_2] = "dstore_2",
    [DSTORE_3] = "dstore_3",
    [ASTORE_0] = "astore_0",
    [ASTORE_1] = "astore_1",
    [ASTORE_2] = "astore_2",
    [ASTORE_3] = "astore_3",
    [IASTORE] = "iastore",
    [LASTORE] = "lastore",
    [FASTORE] = "fastore",
    [DASTORE] = "dastore",
    [AASTORE] = "aastore",
    [BASTORE] = "bastore",
    [CASTORE] = "castore",
    [SASTORE] = "sastore",
    [POP] = "pop",
    [POP2] = "pop2",
    [DUP] = "dup",
    [DUP_X1] = "dup_x1",
    [DUP_X2] = "dup_x2",
    [DUP2] = "dup2",
    [DUP2_X1] = "dup2_x1",
    [DUP2_X2] = "dup2_x2",
    [SWAP] = "swap",
    [IADD] = "iadd",
    [LADD] = "ladd",
    [FADD] = "fadd",
    [DADD] = "dadd",
    [ISUB] = "isub",
    [LSUB] = "lsub",
    [FSUB] = "fsub",
    [DSUB] = "dsub",
    [IMUL] = "imul",
    [LMUL] = "lmul",
    [FMUL] = "fmul",
    [DMUL] = "dmul",
    [IDIV] = "idiv",
    [LDIV] = "ldiv",
    [FDIV] = "fdiv",
    [DDIV] = "ddiv",
    [IREM] = "irem",
    [LREM] = "lrem",
    [FREM] = "frem",
    [DREM] = "drem",
    [INEG] = "ineg",
    [LNEG] = "lneg",
    [FNEG] = "fneg",
    [DNEG] = "dneg",
    [ISHL] = "ishl",
    [LSHL] = "lshl",
    [ISHR] = "ishr",
    [LSHR] = "lshr",
    [IUSHR] = "iushr",
    [LUSHR] = "lushr",
    [IAND] = "iand",
    [LAND] = "land",
    [IOR] = "ior",
    [LOR] = "lor",
    [IXOR] = "ixor",
    [LXOR] = "lxor",
    [IINC] = "iinc",
    [I2L] = "i2l",
    [I2F] = "i2f",
    [I2D] = "i2d",
    [L2I] = "l2i",
    [L2F] = "l2f",
    [L2D] = "l2d",
    [F2I] = "f2i",
    [F2L] = "f2l",
    [F2D] = "f2d",
    [D2I] = "d2i",
    [D2L] = "d2l",
    [D2F] = "d2f",
    [I2B] = "i2b",
    [I2C] = "i2c",
    [I2S] = "i2s",
    [LCMP] = "lcmp",
    [FCMPL] = "fcmpl",
    [FCMPG] = "fcmpg",
    [DCMPL] = "dcmpl",
    [DCMPG] = "dcmpg",
    [IFEQ] = "ifeq",
    [IFNE] = "ifne",
    [IFLT] = "iflt",
    [IFGE] = "ifge",
    [IFGT] = "ifgt",
    [IFLE] = "ifle",
    [IF_ICMPEQ] = "if_icmpeq",
    [IF_ICMPNE] = "if_icmpne",
    [IF_ICMPLT] = "if_icmplt",
    [IF_ICMPGE] = "if_icmpge",
    [IF_ICMPGT] = "if_icmpgt",
    [IF_ICMPLE] = "if_icmple",
    [IF_ACMPEQ] = "if_acmpeq",
    [IF_ACMPNE] = "if_acmpne",
    [GOTO] = "goto",
    [JSR] = "jsr",
    [RET] = "ret",
    [TABLESWITCH] = "tableswitch",
    [LOOKUPSWITCH] = "lookupswitch",
    [IRETURN] = "ireturn",
    [LRETURN] = "lreturn",
    [FRETURN] = "freturn",
    [DRETURN] = "dreturn",
    [ARETURN] = "areturn",
    [RETURN] = "return",
    [GETSTATIC] = "getstatic",
    [PUTSTATIC] = "putstatic",
    [GETFIELD] = "getfield",
    [PUTFIELD] = "putfield",
    [INVOKEVIRTUAL] = "invokevirtual",
    [INVOKESPECIAL] = "invokespecial",
    [INVOKESTATIC] = "invokestatic",
    [INVOKEINTERFACE] = "invokeinterface",
    [INVOKEDYNAMIC] = "invokedynamic",
    [NEW] = "new",
    [NEWARRAY] = "newarray",
    [ANEWARRAY] = "anewarray",
    [ARRAYLENGTH] = "arraylength",
    [ATHROW] = "athrow",
    [CHECKCAST] = "checkcast",
    [INSTANCEOF] = "instanceof",
    [MONITORENTER] = "monitorenter",
    [MONITOREXIT] = "monitorexit",
    [WIDE] = "wide",
    [MULTIANEWARRAY] = "multianewarray",
    [IFNULL] = "ifnull",
    [IFNONNULL] = "ifnonnull",
    [GOTO_W] = "goto_w",
    [JSR_W] = "jsr_w",
};
static char *typenames[T_LAST] = {
    [T_BOOLEAN] = "boolean", [T_CHAR] = "char", [T_FLOAT] = "float",
    [T_DOUBLE] = "double",   [T_BYTE] = "byte", [T_SHORT] = "short",
    [T_INT] = "int",         [T_LONG] = "long",
};

/* flags */
static int cflag = 0;
static int lflag = 0;
static int pflag = 0;
static int sflag = 0;
static int verbose = 0;

/* show usage */
static void usage(void) {
  (void)fprintf(stderr, "usage: javap [-clpsv] classfile...\n");
  exit(EXIT_FAILURE);
}

/* get number of columns to align index or comment text */
static int getcol(int max, int n) {
  return (n > 0 && n < max) ? max - n - 1 : 1;
}

/* quote method name if it is <init> */
static char *quotename(char *s) {
  if (strcmp(s, "<init>") == 0)
    return "\"<init>\"";
  return s;
}

/* print access flags */
static void printflags(U2 flags, int type) {
  static struct {
    char *s;
    U2 flag;
    int type;
  } flagstab[] = {
      {"ACC_PUBLIC", 0x0001,
       TYPE_CLASS | TYPE_FIELD | TYPE_METHOD | TYPE_INNER},
      {"ACC_PRIVATE", 0x0002, TYPE_FIELD | TYPE_METHOD | TYPE_INNER},
      {"ACC_PROTECTED", 0x0004, TYPE_FIELD | TYPE_METHOD | TYPE_INNER},
      {"ACC_STATIC", 0x0008, TYPE_FIELD | TYPE_METHOD | TYPE_INNER},
      {"ACC_FINAL", 0x0010, TYPE_CLASS | TYPE_FIELD | TYPE_METHOD | TYPE_INNER},
      {"ACC_SUPER", 0x0020, TYPE_CLASS},
      {"ACC_SYNCHRONIZED", 0x0020, TYPE_METHOD},
      {"ACC_VOLATILE", 0x0040, TYPE_FIELD},
      {"ACC_BRIDGE", 0x0040, TYPE_METHOD},
      {"ACC_TRANSIENT", 0x0080, TYPE_FIELD},
      {"ACC_VARARGS", 0x0080, TYPE_METHOD},
      {"ACC_NATIVE", 0x0100, TYPE_METHOD},
      {"ACC_INTERFACE", 0x0200, TYPE_CLASS | TYPE_INNER},
      {"ACC_ABSTRACT", 0x0400, TYPE_CLASS | TYPE_METHOD | TYPE_INNER},
      {"ACC_STRICT", 0x0800, TYPE_METHOD},
      {"ACC_SYNTHETIC", 0x1000,
       TYPE_CLASS | TYPE_FIELD | TYPE_METHOD | TYPE_INNER},
      {"ACC_ANNOTATION", 0x2000, TYPE_CLASS | TYPE_INNER},
      {"ACC_ENUM", 0x4000, TYPE_CLASS | TYPE_FIELD | TYPE_INNER}};
  int p = 0; /* whether a flag was printed */
  size_t i;

  printf("flags: (0x%04X) ", flags);
  for (i = 0; i < LEN(flagstab); i++) {
    if (flags & flagstab[i].flag && flagstab[i].type & type) {
      if (p)
        printf(", ");
      printf("%s", flagstab[i].s);
      p = 1;
    }
  }
  putchar('\n');
}

/* print content of constant pool */
static void printcp(ClassFile *class) {
  CP **cp;
  U2 count, i;
  char *name, *type;
  int d, n;

  printf("Constant pool:\n");
  count = class->constant_pool_count;
  cp = class->constant_pool;
  for (i = 1; i < count; i++) {
    d = 0;
    n = i;
    do {
      d++;
    } while (n /= 10);
    d = (d < 4) ? 4 - d : 0;
    n = d;
    while (n--)
      putchar(' ');
    n = printf("#%d = %s", i, cptags[cp[i]->tag]);
    n = (n > 0) ? CPINDEX - (n + d) : 0;
    do {
      putchar(' ');
    } while (n-- > 0);
    switch (cp[i]->tag) {
    case CONSTANT_Utf8:
      printf("%s", cp[i]->info.utf8_info.bytes);
      break;
    case CONSTANT_Integer:
      printf("%ld", (long)getint(cp[i]->info.integer_info.bytes));
      break;
    case CONSTANT_Float:
      printf("%gd", getfloat(cp[i]->info.integer_info.bytes));
      break;
    case CONSTANT_Long:
      printf("%ld", getlong(cp[i]->info.long_info.high_bytes,
                            cp[i]->info.long_info.low_bytes));
      i++;
      break;
    case CONSTANT_Double:
      printf("%gd", getdouble(cp[i]->info.long_info.high_bytes,
                              cp[i]->info.long_info.low_bytes));
      i++;
      break;
    case CONSTANT_Class:
      n = printf("#%u", cp[i]->info.class_info.name_index);
      n = getcol(16, n);
      printf("%*c", n, ' ');
      printf("// %s", class_getutf8(class, cp[i]->info.class_info.name_index));
      break;
    case CONSTANT_String:
      n = printf("#%u", cp[i]->info.string_info.string_index);
      n = getcol(16, n);
      printf("%*c", n, ' ');
      printf("// %s",
             class_getutf8(class, cp[i]->info.string_info.string_index));
      break;
    case CONSTANT_Fieldref:
      n = printf("#%u.#%u", cp[i]->info.fieldref_info.class_index,
                 cp[i]->info.fieldref_info.name_and_type_index);
      n = getcol(16, n);
      printf("%*c", n, ' ');
      class_getnameandtype(class, cp[i]->info.fieldref_info.name_and_type_index,
                           &name, &type);
      printf("// %s.%s:%s",
             class_getclassname(class, cp[i]->info.fieldref_info.class_index),
             name, type);
      break;
    case CONSTANT_Methodref:
      n = printf("#%u.#%u", cp[i]->info.methodref_info.class_index,
                 cp[i]->info.methodref_info.name_and_type_index);
      n = getcol(16, n);
      printf("%*c", n, ' ');
      class_getnameandtype(
          class, cp[i]->info.methodref_info.name_and_type_index, &name, &type);
      name = quotename(name);
      printf("// %s.%s:%s",
             class_getclassname(class, cp[i]->info.methodref_info.class_index),
             name, type);
      break;
    case CONSTANT_InterfaceMethodref:
      printf("#%u", cp[i]->info.interfacemethodref_info.class_index);
      printf(".#%u", cp[i]->info.interfacemethodref_info.name_and_type_index);
      break;
    case CONSTANT_NameAndType:
      n = printf("#%u:#%u", cp[i]->info.nameandtype_info.name_index,
                 cp[i]->info.nameandtype_info.descriptor_index);
      n = getcol(16, n);
      printf("%*c", n, ' ');
      class_getnameandtype(class, i, &name, &type);
      name = quotename(name);
      printf("// %s:%s", name, type);
      break;
    case CONSTANT_MethodHandle:
      printf("%u", cp[i]->info.methodhandle_info.reference_kind);
      printf(":#%u", cp[i]->info.methodhandle_info.reference_index);
      break;
    case CONSTANT_MethodType:
      printf("#%u", cp[i]->info.methodtype_info.descriptor_index);
      break;
    case CONSTANT_InvokeDynamic:
      printf("#%u", cp[i]->info.invokedynamic_info.bootstrap_method_attr_index);
      printf(":#%u", cp[i]->info.invokedynamic_info.name_and_type_index);
      break;
    }
    putchar('\n');
  }
}

/* print class name */
static void printclass(ClassFile *class, U2 index) {
  char *s;

  s = class_getclassname(class, index);
  while (*s) {
    if (*s == TYPE_SEPARATOR)
      putchar('.');
    else
      putchar(*s);
    s++;
  }
}

/* print metadata about class */
static void printmeta(ClassFile *class) {
  int n;

  printf("  minor version: %u\n", class->minor_version);
  printf("  major version: %u\n", class->major_version);
  printf("  Java version: %u.0\n", class->major_version - 44);
  printf("  ");
  printflags(class->access_flags, TYPE_CLASS);

  n = printf("  this_class: #%u", class->this_class);
  n = getcol(CPCOMMENT, n);
  printf("%*c// ", n, ' ');
  printf("%s", class_getclassname(class, class->this_class));
  printf("\n");

  n = printf("  super_class: #%u", class->super_class);
  n = getcol(CPCOMMENT, n);
  printf("%*c// ", n, ' ');
  printf("%s", class_getclassname(class, class->super_class));
  printf("\n");

  printf("  interfaces: %u, fields: %u, methods: %u, attributes: %u\n",
         class->interfaces_count, class->fields_count, class->methods_count,
         class->attributes_count);
}

/* print source file name */
static void printsource(ClassFile *class) {
  Attribute *attr;

  if ((attr = class_getattr(class->attributes, class->attributes_count,
                            SourceFile)) == NULL)
    return;
  printf("Compiled from \"%s\"\n",
         class_getutf8(class, attr->info.sourcefile.sourcefile_index));
}

/* print class header */
static void printheader(ClassFile *class) {
  U2 i;

  if (class->access_flags & ACC_PUBLIC)
    printf("public ");
  if (class->access_flags & ACC_INTERFACE) {
    if (class->access_flags & ACC_STRICT)
      printf("strict ");
    printf("interface ");
  } else if (class->access_flags & ACC_ENUM) {
    if (class->access_flags & ACC_STRICT)
      printf("strict ");
    printf("enum ");
  } else {
    if (class->access_flags & ACC_ABSTRACT)
      printf("abstract ");
    else if (class->access_flags & ACC_FINAL)
      printf("final ");
    if (class->access_flags & ACC_STRICT)
      printf("strict ");
    printf("class ");
  }
  printclass(class, class->this_class);
  if (class->super_class && !class_istopclass(class)) {
    printf(" extends ");
    printclass(class, class->super_class);
    ;
  }
  if (class->interfaces_count > 0)
    printf(" implements ");
  for (i = 0; i < class->interfaces_count; i++) {
    if (i > 0)
      printf(", ");
    printclass(class, class->interfaces[i]);
  }
}

/* print type, return next type in descriptor */
static char *printtype(char *type) {
  char *s;

  s = type + 1;
  switch (*type) {
  case TYPE_BYTE:
    printf("byte");
    break;
  case TYPE_CHAR:
    printf("char");
    break;
  case TYPE_DOUBLE:
    printf("double");
    break;
  case TYPE_FLOAT:
    printf("float");
    break;
  case TYPE_INT:
    printf("int");
    break;
  case TYPE_LONG:
    printf("long");
    break;
  case TYPE_REFERENCE:
    while (*s && *s != TYPE_TERMINAL) {
      if (*s == TYPE_SEPARATOR)
        putchar('.');
      else
        putchar(*s);
      s++;
    }
    if (*s == TYPE_TERMINAL)
      s++;
    break;
  case TYPE_SHORT:
    printf("short");
    break;
  case TYPE_VOID:
    printf("void");
    break;
  case TYPE_BOOLEAN:
    printf("boolean");
    break;
  case TYPE_ARRAY:
    s = printtype(s);
    printf("[]");
    break;
  }
  return s;
}

/* print descriptor and name; return number of parameters */
static U2 printdeclaration(char *descriptor, char *name, int init) {
  U2 nargs = 0;
  char *s;

  s = strrchr(descriptor, ')');
  if (s == NULL) {
    printtype(descriptor);
    printf(" %s", name);
  } else {
    if (!init) {
      printtype(s + 1);
      putchar(' ');
    }
    printf("%s(", name);
    s = descriptor + 1;
    while (*s && *s != ')') {
      if (nargs)
        printf(", ");
      s = printtype(s);
      nargs++;
    }
    putchar(')');
  }
  return nargs;
}

/* print constant value of a field */
static void printconstant(ClassFile *class, Field *field) {
  U2 index, i;

  if (!(field->access_flags & ACC_STATIC))
    return;
  index = 0;
  for (i = 0; i < field->attributes_count; i++) {
    if (field->attributes[i]->tag == ConstantValue) {
      index = field->attributes[i]->info.constantvalue.constantvalue_index;
      break;
    }
  }
  if (index == 0)
    return;
  printf("    ConstantValue: ");
  switch (class->constant_pool[index]->tag) {
  case CONSTANT_Integer:
    printf("int %ld",
           (long)getint(class->constant_pool[index]->info.integer_info.bytes));
    break;
  case CONSTANT_Long:
    printf("long %ld",
           getlong(class->constant_pool[index]->info.long_info.high_bytes,
                   class->constant_pool[index]->info.long_info.low_bytes));
    break;
  case CONSTANT_Float:
    printf("float %gf",
           getfloat(class->constant_pool[index]->info.float_info.bytes));
    break;
  case CONSTANT_Double:
    printf("double %gd",
           getdouble(class->constant_pool[index]->info.double_info.high_bytes,
                     class->constant_pool[index]->info.double_info.low_bytes));
    break;
  case CONSTANT_String:
    printf(
        "String %s",
        class_getutf8(
            class, class->constant_pool[index]->info.string_info.string_index));
    break;
  }
  putchar('\n');
}

/* print field information */
static void printfield(ClassFile *class, U2 count) {
  Field *field;

  field = class->fields[count];
  if (!pflag && field->access_flags & ACC_PRIVATE)
    return;
  printf("  ");
  if (field->access_flags & ACC_PRIVATE)
    printf("private ");
  else if (field->access_flags & ACC_PROTECTED)
    printf("protected ");
  else if (field->access_flags & ACC_PUBLIC)
    printf("public ");
  if (field->access_flags & ACC_STATIC)
    printf("static ");
  if (field->access_flags & ACC_FINAL)
    printf("final ");
  if (field->access_flags & ACC_TRANSIENT)
    printf("transient ");
  if (field->access_flags & ACC_VOLATILE)
    printf("volatile ");
  printdeclaration(class_getutf8(class, field->descriptor_index),
                   class_getutf8(class, field->name_index), 0);
  printf(";\n");
  if (sflag)
    printf("    descriptor: %s\n",
           class_getutf8(class, field->descriptor_index));
  if (verbose) {
    printf("    ");
    printflags(field->access_flags, TYPE_FIELD);
    printconstant(class, field);
  }
  if (lflag || cflag) {
    putchar('\n');
  }
}

/* print line numbers */
static void printlinenumbers(LineNumberTable_attribute *lnattr) {
  LineNumber **ln;
  U2 count, i;

  printf("      LineNumberTable:\n");
  count = lnattr->line_number_table_length;
  ln = lnattr->line_number_table;
  for (i = 0; i < count; i++) {
    printf("        line %u: %u\n", ln[i]->line_number, ln[i]->start_pc);
  }
}

/* print local variables */
static void printlocalvars(ClassFile *class,
                           LocalVariableTable_attribute *lvattr) {
  LocalVariable **lv;
  U2 count, i;

  count = lvattr->local_variable_table_length;
  lv = lvattr->local_variable_table;
  if (count == 0)
    return;
  printf("      LocalVariableTable:\n");
  printf("        Start  Length  Slot  Name   Signature\n");
  for (i = 0; i < count; i++) {
    printf("      %7u %7u %5u %5s   %s\n", lv[i]->start_pc, lv[i]->length,
           lv[i]->index, class_getutf8(class, lv[i]->name_index),
           class_getutf8(class, lv[i]->descriptor_index));
  }
}

/* print code of method */
static void printcode(ClassFile *class, Code_attribute *codeattr, U2 nargs) {
  int32_t j, npairs, def, high, low;
  CP **cp;
  U1 *code;
  U1 opcode;
  U1 byte;
  U2 u;
  U4 count;
  U4 a, b, c, d;
  U4 i, base;
  int8_t ch;
  int16_t off;  /* branch offset */
  int32_t offw; /* wide branch offset */
  int n, m;     /* number of printed column, to format output */
  char *cname, *name, *type;

  cp = class->constant_pool;
  code = codeattr->code;
  count = codeattr->code_length;
  printf("    Code:\n");
  if (verbose) {
    printf("      stack=%u, locals=%u, args_size=%u", codeattr->max_stack,
           codeattr->max_locals, nargs);
    putchar('\n');
  }
  for (i = 0; i < count; i++) {
    opcode = code[i];
    if (verbose)
      printf("  ");
    n = printf("%8u: %s", i, instrnames[opcode]);
    m = getcol(CODEINDEX, n);
    switch (code[i]) {
    case WIDE:
      switch (code[++i]) {
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
        i += 2;
        break;
      case IINC:
        i += 4;
        break;
      }
      break;
    case BIPUSH:
      byte = code[++i];
      memcpy(&ch, &byte, sizeof(ch));
      printf("%*c%d", m, ' ', ch);
      break;
    case IINC:
      byte = code[++i];
      memcpy(&ch, &byte, sizeof(ch));
      printf("%*c%d, ", m, ' ', ch);
      byte = code[++i];
      memcpy(&ch, &byte, sizeof(ch));
      printf("%d", ch);
      break;
    case GOTO:
    case IF_ACMPEQ:
    case IF_ACMPNE:
    case IF_ICMPEQ:
    case IF_ICMPNE:
    case IF_ICMPLT:
    case IF_ICMPGT:
    case IF_ICMPLE:
    case IF_ICMPGE:
    case IFEQ:
    case IFNE:
    case IFLT:
    case IFGT:
    case IFLE:
    case IFGE:
    case JSR:
      base = i;
      u = 0;
      u = code[++i] << 8;
      u |= code[++i];
      memcpy(&off, &u, sizeof(off));
      off += base;
      printf("%*c%d", m, ' ', off);
      break;
    case GOTO_W:
    case JSR_W:
      base = i;
      a = 0;
      a = code[++i] << 24;
      a |= code[++i] << 16;
      a |= code[++i] << 8;
      a |= code[++i];
      memcpy(&offw, &a, sizeof(offw));
      offw += base;
      printf("%*c%d", m, ' ', offw);
      break;
    case LOOKUPSWITCH:
      i++;
      while (i % 4)
        i++;
      printf("->%u\n", i);
      i += 4;
      a = code[i++];
      b = code[i++];
      c = code[i++];
      d = code[i];
      npairs = (a << 24) | (b << 16) | (c << 8) | d;
      i += 8 * npairs;
      break;
    case TABLESWITCH:
      base = i++;
      while (i % 4)
        i++;
      a = code[i++];
      b = code[i++];
      c = code[i++];
      d = code[i++];
      def = (a << 24) | (b << 16) | (c << 8) | d;
      a = code[i++];
      b = code[i++];
      c = code[i++];
      d = code[i++];
      low = (a << 24) | (b << 16) | (c << 8) | d;
      a = code[i++];
      b = code[i++];
      c = code[i++];
      d = code[i++];
      high = (a << 24) | (b << 16) | (c << 8) | d;
      printf("   { // %d to %d\n", low, high);
      for (j = low; j <= high; j++) {
        a = code[i++];
        b = code[i++];
        c = code[i++];
        d = code[i++];
        printf("%24d: %d\n", j,
               ((int32_t)(a << 24) | (b << 16) | (c << 8) | d) + base);
      }
      i--;
      printf("                 default: %d\n", def + base);
      printf("            }");
      break;
    case GETSTATIC:
      u = code[++i] << 8;
      u |= code[++i];
      n += printf("%*c#%u", m, ' ', u);
      m = getcol(CODECOMMENT, n);
      class_getnameandtype(class, cp[u]->info.fieldref_info.name_and_type_index,
                           &name, &type);
      printf("%*c// Field %s.%s:%s", m, ' ',
             class_getclassname(class, cp[u]->info.fieldref_info.class_index),
             name, type);
      break;
    case INVOKEVIRTUAL:
    case INVOKESPECIAL:
    case INVOKESTATIC:
      u = code[++i] << 8;
      u |= code[++i];
      n += printf("%*c#%u", m, ' ', u);
      m = getcol(CODECOMMENT, n);
      class_getnameandtype(
          class, cp[u]->info.methodref_info.name_and_type_index, &name, &type);
      cname = class_getclassname(class, cp[u]->info.methodref_info.class_index);
      if (strcmp(cname, class_getclassname(class, class->this_class)) == 0)
        cname = "";
      name = quotename(name);
      printf("%*c// Method %s%s%s:%s", m, ' ', cname,
             (*cname == '\0' ? "" : "."), name, type);
      break;
    case LDC:
    case LDC_W:
    case LDC2_W:
      u = 0;
      if (code[i] == LDC_W || code[i] == LDC2_W)
        u = code[++i];
      u |= code[++i];
      n += printf("%*c#%u", m, ' ', u);
      m = getcol(CODECOMMENT, n);
      switch (cp[u]->tag) {
      case CONSTANT_String:
        printf("%*c// String %s", m, ' ', class_getstring(class, u));
        break;
      case CONSTANT_Long:
        printf("%*c// Integer %lld", m, ' ',
               (long long int)class_getlong(class, u));
        break;
      case CONSTANT_Double:
        printf("%*c// double %gd", m, ' ', (double)class_getdouble(class, u));
        break;
      case CONSTANT_Integer:
        printf("%*c// Integer %ld", m, ' ',
               (long int)class_getinteger(class, u));
        break;
      case CONSTANT_Float:
        printf("%*c// float %gf", m, ' ', (float)class_getfloat(class, u));
        break;
      }
      break;
    case MULTIANEWARRAY:
      u = code[++i] << 8;
      u |= code[++i];
      n += printf(" #%u,  %u", u, code[++i]);
      m = getcol(CODECOMMENT, n);
      printf("%*c// class \"%s\"", m, ' ',
             class_getutf8(class, cp[u]->info.class_info.name_index));
      break;
    case NEWARRAY:
      type = typenames[code[++i]];
      printf("%*c%s", m, ' ', type);
      break;
    default:
      for (j = 0; i < count && j < class_getnoperands(opcode); j++)
        i++;
      break;
    }
    printf("\n");
  }
}

/* print method information */
static void printmethod(ClassFile *class, U2 count) {
  char *name;
  int init = 0;
  U2 nargs;
  Attribute *cattr;  /* Code_attribute */
  Attribute *lnattr; /* LineNumberTable_attribute */
  Attribute *lvattr; /* LocalVariableTable_attribute */
  Method *method;

  method = class->methods[count];
  if (!pflag && method->access_flags & ACC_PRIVATE)
    return;
  if (count && (lflag || sflag || cflag))
    putchar('\n');
  name = class_getutf8(class, method->name_index);
  if (strcmp(name, "<init>") == 0) {
    name = class_getclassname(class, class->this_class);
    init = 1;
  }
  printf("  ");
  if (method->access_flags & ACC_PRIVATE)
    printf("private ");
  else if (method->access_flags & ACC_PROTECTED)
    printf("protected ");
  else if (method->access_flags & ACC_PUBLIC)
    printf("public ");
  if (method->access_flags & ACC_ABSTRACT)
    printf("abstract ");
  if (method->access_flags & ACC_STATIC)
    printf("static ");
  if (method->access_flags & ACC_FINAL)
    printf("final ");
  if (method->access_flags & ACC_SYNCHRONIZED)
    printf("synchronized ");
  if (method->access_flags & ACC_NATIVE)
    printf("native ");
  if (method->access_flags & ACC_STRICT)
    printf("strict ");
  nargs = printdeclaration(class_getutf8(class, method->descriptor_index), name,
                           init);
  if (!(method->access_flags & ACC_STATIC))
    nargs++;
  printf(";\n");
  if (sflag)
    printf("    descriptor: %s\n",
           class_getutf8(class, method->descriptor_index));
  if (verbose) {
    printf("    ");
    printflags(method->access_flags, TYPE_METHOD);
  }
  cattr = class_getattr(method->attributes, method->attributes_count, Code);
  if (cattr != NULL) {
    lnattr = class_getattr(cattr->info.code.attributes,
                           cattr->info.code.attributes_count, LineNumberTable);
    lvattr =
        class_getattr(cattr->info.code.attributes,
                      cattr->info.code.attributes_count, LocalVariableTable);
    if (cflag) {
      printcode(class, &cattr->info.code, nargs);
    }
    if (lflag && lnattr != NULL) {
      printlinenumbers(&lnattr->info.linenumbertable);
    }
    if (lflag && lvattr != NULL) {
      printlocalvars(class, &lvattr->info.localvariabletable);
    }
  }
}

/* print class contents */
static void javap(ClassFile *class) {
  U2 i;

  printsource(class);
  printheader(class);
  if (verbose) {
    printf("\n");
    printmeta(class);
    printcp(class);
    printf("{\n");
  } else {
    printf(" {\n");
  }
  for (i = 0; i < class->fields_count; i++)
    printfield(class, i);
  for (i = 0; i < class->methods_count; i++)
    printmethod(class, i);
  printf("}\n");
}

/* javap: disassemble jclass files */
int main(int argc, char *argv[]) {
  ClassFile *class;
  FILE *fp;
  int exitval = EXIT_SUCCESS;
  int status;
  int ch;

  setprogname(argv[0]);
  while ((ch = getopt(argc, argv, "clpsv")) != -1) {
    switch (ch) {
    case 'c':
      cflag = 1;
      break;
    case 'l':
      lflag = 1;
      break;
    case 'p':
      pflag = 1;
      break;
    case 's':
      sflag = 1;
      break;
    case 'v':
      verbose = 1;
      cflag = 1;
      lflag = 1;
      sflag = 1;
      break;
    default:
      usage();
      break;
    }
  }
  argc -= optind;
  argv += optind;
  if (argc == 0)
    usage();
  class = emalloc(sizeof *class);
  for (; argc--; argv++) {
    if ((fp = fopen(*argv, "rb")) == NULL) {
      warn("%s", *argv);
      exitval = EXIT_FAILURE;
      continue;
    }
    if ((status = file_read(fp, class)) != 0) {
      warnx("%s: %s", *argv, file_errstr(status));
      exitval = EXIT_FAILURE;
    } else {
      javap(class);
      file_free(class);
    }
    fclose(fp);
  }
  return exitval;
}
