#define OP_WIDE -1
#define OP_TABLESWITCH -2
#define OP_LOOKUPSWITCH -3

typedef uint8_t U1;
typedef uint16_t U2;
typedef uint32_t U4;
typedef uint64_t U8;

/* heap object structure */
typedef struct Heap {
  struct Heap *prev, *next;
  void *obj;
  int32_t nmemb;
  size_t count;
} Heap;

/* local variable or operand structure */
typedef union Value {
  int32_t i;
  int64_t l;
  float f;
  double d;
  Heap *v;
} Value;

typedef enum TypeCode {
  T_BOOLEAN = 4,
  T_CHAR = 5,
  T_FLOAT = 6,
  T_DOUBLE = 7,
  T_BYTE = 8,
  T_SHORT = 9,
  T_INT = 10,
  T_LONG = 11,
  T_LAST
} TypeCode;

typedef enum Instruction {
  /* constants */
  NOP = 0x00,
  ACONST_NULL = 0x01,
  ICONST_M1 = 0x02,
  ICONST_0 = 0x03,
  ICONST_1 = 0x04,
  ICONST_2 = 0x05,
  ICONST_3 = 0x06,
  ICONST_4 = 0x07,
  ICONST_5 = 0x08,
  LCONST_0 = 0x09,
  LCONST_1 = 0x0A,
  FCONST_0 = 0x0B,
  FCONST_1 = 0x0C,
  FCONST_2 = 0x0D,
  DCONST_0 = 0x0E,
  DCONST_1 = 0x0F,
  BIPUSH = 0x10,
  SIPUSH = 0x11,
  LDC = 0x12,
  LDC_W = 0x13,
  LDC2_W = 0x14,

  /* loads */
  ILOAD = 0x15,
  LLOAD = 0x16,
  FLOAD = 0x17,
  DLOAD = 0x18,
  ALOAD = 0x19,
  ILOAD_0 = 0x1A,
  ILOAD_1 = 0x1B,
  ILOAD_2 = 0x1C,
  ILOAD_3 = 0x1D,
  LLOAD_0 = 0x1E,
  LLOAD_1 = 0x1F,
  LLOAD_2 = 0x20,
  LLOAD_3 = 0x21,
  FLOAD_0 = 0x22,
  FLOAD_1 = 0x23,
  FLOAD_2 = 0x24,
  FLOAD_3 = 0x25,
  DLOAD_0 = 0x26,
  DLOAD_1 = 0x27,
  DLOAD_2 = 0x28,
  DLOAD_3 = 0x29,
  ALOAD_0 = 0x2A,
  ALOAD_1 = 0x2B,
  ALOAD_2 = 0x2C,
  ALOAD_3 = 0x2D,
  IALOAD = 0x2E,
  LALOAD = 0x2F,
  FALOAD = 0x30,
  DALOAD = 0x31,
  AALOAD = 0x32,
  BALOAD = 0x33,
  CALOAD = 0x34,
  SALOAD = 0x35,

  /* stores */
  ISTORE = 0x36,
  LSTORE = 0x37,
  FSTORE = 0x38,
  DSTORE = 0x39,
  ASTORE = 0x3A,
  ISTORE_0 = 0x3B,
  ISTORE_1 = 0x3C,
  ISTORE_2 = 0x3D,
  ISTORE_3 = 0x3E,
  LSTORE_0 = 0x3F,
  LSTORE_1 = 0x40,
  LSTORE_2 = 0x41,
  LSTORE_3 = 0x42,
  FSTORE_0 = 0x43,
  FSTORE_1 = 0x44,
  FSTORE_2 = 0x45,
  FSTORE_3 = 0x46,
  DSTORE_0 = 0x47,
  DSTORE_1 = 0x48,
  DSTORE_2 = 0x49,
  DSTORE_3 = 0x4A,
  ASTORE_0 = 0x4B,
  ASTORE_1 = 0x4C,
  ASTORE_2 = 0x4D,
  ASTORE_3 = 0x4E,
  IASTORE = 0x4F,
  LASTORE = 0x50,
  FASTORE = 0x51,
  DASTORE = 0x52,
  AASTORE = 0x53,
  BASTORE = 0x54,
  CASTORE = 0x55,
  SASTORE = 0x56,

  /* stack */
  POP = 0x57,
  POP2 = 0x58,
  DUP = 0x59,
  DUP_X1 = 0x5A,
  DUP_X2 = 0x5B,
  DUP2 = 0x5C,
  DUP2_X1 = 0x5D,
  DUP2_X2 = 0x5E,
  SWAP = 0x5F,

  /* math */
  IADD = 0x60,
  LADD = 0x61,
  FADD = 0x62,
  DADD = 0x63,
  ISUB = 0x64,
  LSUB = 0x65,
  FSUB = 0x66,
  DSUB = 0x67,
  IMUL = 0x68,
  LMUL = 0x69,
  FMUL = 0x6A,
  DMUL = 0x6B,
  IDIV = 0x6C,
  LDIV = 0x6D,
  FDIV = 0x6E,
  DDIV = 0x6F,
  IREM = 0x70,
  LREM = 0x71,
  FREM = 0x72,
  DREM = 0x73,
  INEG = 0x74,
  LNEG = 0x75,
  FNEG = 0x76,
  DNEG = 0x77,
  ISHL = 0x78,
  LSHL = 0x79,
  ISHR = 0x7A,
  LSHR = 0x7B,
  IUSHR = 0x7C,
  LUSHR = 0x7D,
  IAND = 0x7E,
  LAND = 0x7F,
  IOR = 0x80,
  LOR = 0x81,
  IXOR = 0x82,
  LXOR = 0x83,
  IINC = 0x84,

  /* conversions */
  I2L = 0x85,
  I2F = 0x86,
  I2D = 0x87,
  L2I = 0x88,
  L2F = 0x89,
  L2D = 0x8A,
  F2I = 0x8B,
  F2L = 0x8C,
  F2D = 0x8D,
  D2I = 0x8E,
  D2L = 0x8F,
  D2F = 0x90,
  I2B = 0x91,
  I2C = 0x92,
  I2S = 0x93,

  /* comparisons */
  LCMP = 0x94,
  FCMPL = 0x95,
  FCMPG = 0x96,
  DCMPL = 0x97,
  DCMPG = 0x98,
  IFEQ = 0x99,
  IFNE = 0x9A,
  IFLT = 0x9B,
  IFGE = 0x9C,
  IFGT = 0x9D,
  IFLE = 0x9E,
  IF_ICMPEQ = 0x9F,
  IF_ICMPNE = 0xA0,
  IF_ICMPLT = 0xA1,
  IF_ICMPGE = 0xA2,
  IF_ICMPGT = 0xA3,
  IF_ICMPLE = 0xA4,
  IF_ACMPEQ = 0xA5,
  IF_ACMPNE = 0xA6,

  /* control */
  GOTO = 0xA7,
  JSR = 0xA8,
  RET = 0xA9,
  TABLESWITCH = 0xAA,
  LOOKUPSWITCH = 0xAB,
  IRETURN = 0xAC,
  LRETURN = 0xAD,
  FRETURN = 0xAE,
  DRETURN = 0xAF,
  ARETURN = 0xB0,
  RETURN = 0xB1,

  /* references */
  GETSTATIC = 0xB2,
  PUTSTATIC = 0xB3,
  GETFIELD = 0xB4,
  PUTFIELD = 0xB5,
  INVOKEVIRTUAL = 0xB6,
  INVOKESPECIAL = 0xB7,
  INVOKESTATIC = 0xB8,
  INVOKEINTERFACE = 0xB9,
  INVOKEDYNAMIC = 0xBA,
  NEW = 0xBB,
  NEWARRAY = 0xBC,
  ANEWARRAY = 0xBD,
  ARRAYLENGTH = 0xBE,
  ATHROW = 0xBF,
  CHECKCAST = 0xC0,
  INSTANCEOF = 0xC1,
  MONITORENTER = 0xC2,
  MONITOREXIT = 0xC3,

  /* extended */
  WIDE = 0xC4,
  MULTIANEWARRAY = 0xC5,
  IFNULL = 0xC6,
  IFNONNULL = 0xC7,
  GOTO_W = 0xC8,
  JSR_W = 0xC9,

  /* reserved */
  CODE_LAST = 0xCA,
  BREAKPOINT = 0xCA,
  IMPDEP1 = 0xFE,
  IMPDEP2 = 0xFF
} Instruction;

typedef enum ConstantTag {
  CONSTANT_Untagged = 0,
  CONSTANT_Utf8 = 1,
  CONSTANT_Integer = 3,
  CONSTANT_Float = 4,
  CONSTANT_Long = 5,
  CONSTANT_Double = 6,
  CONSTANT_Class = 7,
  CONSTANT_String = 8,
  CONSTANT_Fieldref = 9,
  CONSTANT_Methodref = 10,
  CONSTANT_InterfaceMethodref = 11,
  CONSTANT_NameAndType = 12,
  CONSTANT_MethodHandle = 15,
  CONSTANT_MethodType = 16,
  CONSTANT_InvokeDynamic = 18,
  CONSTANT_Constant =
      20, /* used to tag integers, longs, doubles, floats and strings */
  CONSTANT_U1 = 21, /* used to tag integers, floats and strings */
  CONSTANT_U2 = 22  /* used to tag longs and doubles */
} ConstantTag;

typedef enum FlagType {
  TYPE_CLASS = 0x01,
  TYPE_FIELD = 0x02,
  TYPE_METHOD = 0x04,
  TYPE_INNER = 0x08
} FlagType;

typedef enum AccessFlags {
  ACC_NONE = 0x0000,
  ACC_PUBLIC = 0x0001,
  ACC_PRIVATE = 0x0002,
  ACC_PROTECTED = 0x0004,
  ACC_STATIC = 0x0008,
  ACC_FINAL = 0x0010,
  ACC_SUPER = 0x0020,
  ACC_SYNCHRONIZED = 0x0020,
  ACC_VOLATILE = 0x0040,
  ACC_BRIDGE = 0x0040,
  ACC_TRANSIENT = 0x0080,
  ACC_VARARGS = 0x0080,
  ACC_NATIVE = 0x0100,
  ACC_INTERFACE = 0x0200,
  ACC_ABSTRACT = 0x0400,
  ACC_STRICT = 0x0800,
  ACC_SYNTHETIC = 0x1000,
  ACC_ANNOTATION = 0x2000,
  ACC_ENUM = 0x4000
} AccessFlags;

typedef enum AttributeTag {
  UnknownAttribute,
  ConstantValue,
  Code,
  Deprecated,
  Exceptions,
  InnerClasses,
  SourceFile,
  Synthetic,
  LineNumberTable,
  LocalVariableTable
} AttributeTag;

typedef enum ReferenceKind {
  REF_none = 0,
  REF_getField = 1,
  REF_getStatic = 2,
  REF_putField = 3,
  REF_putStatic = 4,
  REF_invokeVirtual = 5,
  REF_invokeStatic = 6,
  REF_invokeSpecial = 7,
  REF_newInvokeSpecial = 8,
  REF_invokeInterface = 9,
  REF_last = 10,
} ReferenceKind;

typedef enum FieldType {
  TYPE_BYTE = 'B',
  TYPE_CHAR = 'C',
  TYPE_DOUBLE = 'D',
  TYPE_FLOAT = 'F',
  TYPE_INT = 'I',
  TYPE_LONG = 'J',
  TYPE_REFERENCE = 'L',
  TYPE_SHORT = 'S',
  TYPE_VOID = 'V',
  TYPE_BOOLEAN = 'Z',
  TYPE_ARRAY = '[',
  TYPE_TERMINAL = ';',
  TYPE_SEPARATOR = '/',
} FieldType;

typedef struct CONSTANT_Utf8_info {
  U2 length;
  char *bytes;
} CONSTANT_Utf8_info;

typedef struct CONSTANT_Integer_info {
  U4 bytes;
} CONSTANT_Integer_info;

typedef struct CONSTANT_Float_info {
  U4 bytes;
} CONSTANT_Float_info;

typedef struct CONSTANT_Long_info {
  U4 high_bytes;
  U4 low_bytes;
} CONSTANT_Long_info;

typedef struct CONSTANT_Double_info {
  U4 high_bytes;
  U4 low_bytes;
} CONSTANT_Double_info;

typedef struct CONSTANT_Class_info {
  U2 name_index;
} CONSTANT_Class_info;

typedef struct CONSTANT_String_info {
  U2 string_index;
  char *string;
} CONSTANT_String_info;

typedef struct CONSTANT_Fieldref_info {
  U2 class_index;
  U2 name_and_type_index;
} CONSTANT_Fieldref_info;

typedef struct CONSTANT_Methodref_info {
  U2 class_index;
  U2 name_and_type_index;
} CONSTANT_Methodref_info;

typedef struct CONSTANT_InterfaceMethodref_info {
  U2 class_index;
  U2 name_and_type_index;
} CONSTANT_InterfaceMethodref_info;

typedef struct CONSTANT_NameAndType_info {
  U2 name_index;
  U2 descriptor_index;
} CONSTANT_NameAndType_info;

typedef struct CONSTANT_MethodHandle_info {
  U1 reference_kind;
  U2 reference_index;
} CONSTANT_MethodHandle_info;

typedef struct CONSTANT_MethodType_info {
  U2 descriptor_index;
} CONSTANT_MethodType_info;

typedef struct CONSTANT_InvokeDynamic_info {
  U2 bootstrap_method_attr_index;
  U2 name_and_type_index;
} CONSTANT_InvokeDynamic_info;

typedef struct ConstantValue_attribute {
  U2 constantvalue_index;
} ConstantValue_attribute;

typedef struct Code_attribute {
  U2 max_stack;
  U2 max_locals;
  U4 code_length;
  U1 *code;
  U2 exception_table_length;
  struct Exception **exception_table;
  U2 attributes_count;
  struct Attribute **attributes;
} Code_attribute;

typedef struct Exceptions_attribute {
  U2 number_of_exceptions;
  U2 *exception_index_table;
} Exceptions_attribute;

typedef struct InnerClasses_attribute {
  U2 number_of_classes;
  struct InnerClass **classes;
} InnerClasses_attribute;

typedef struct SourceFile_attribute {
  U2 sourcefile_index;
} SourceFile_attribute;

typedef struct LineNumberTable_attribute {
  U2 line_number_table_length;
  struct LineNumber **line_number_table;
} LineNumberTable_attribute;

typedef struct LocalVariableTable_attribute {
  U2 local_variable_table_length;
  struct LocalVariable **local_variable_table;
} LocalVariableTable_attribute;

typedef struct CP {
  U1 tag;
  union {
    struct CONSTANT_Utf8_info utf8_info;
    struct CONSTANT_Integer_info integer_info;
    struct CONSTANT_Float_info float_info;
    struct CONSTANT_Long_info long_info;
    struct CONSTANT_Double_info double_info;
    struct CONSTANT_Class_info class_info;
    struct CONSTANT_String_info string_info;
    struct CONSTANT_Fieldref_info fieldref_info;
    struct CONSTANT_Methodref_info methodref_info;
    struct CONSTANT_InterfaceMethodref_info interfacemethodref_info;
    struct CONSTANT_NameAndType_info nameandtype_info;
    struct CONSTANT_MethodHandle_info methodhandle_info;
    struct CONSTANT_MethodType_info methodtype_info;
    struct CONSTANT_InvokeDynamic_info invokedynamic_info;
  } info;
} CP;

typedef struct Attribute {
  enum AttributeTag tag;
  union {
    struct ConstantValue_attribute constantvalue;
    struct Code_attribute code;
    struct Exceptions_attribute exceptions;
    struct InnerClasses_attribute innerclasses;
    struct SourceFile_attribute sourcefile;
    struct LineNumberTable_attribute linenumbertable;
    struct LocalVariableTable_attribute localvariabletable;
  } info;
} Attribute;

typedef struct Field {
  U2 access_flags;
  U2 name_index;
  U2 descriptor_index;
  U2 attributes_count;
  struct Attribute **attributes;
} Field;

typedef struct Method {
  U2 access_flags;
  U2 name_index;
  U2 descriptor_index;
  U2 attributes_count;
  struct Attribute **attributes;
} Method;

typedef struct Exception {
  U2 start_pc;
  U2 end_pc;
  U2 handler_pc;
  U2 catch_type;
} Exception;

typedef struct InnerClass {
  U2 inner_class_info_index;
  U2 outer_class_info_index;
  U2 inner_name_index;
  U2 inner_class_access_flags;
} InnerClass;

typedef struct LineNumber {
  U2 start_pc;
  U2 line_number;
} LineNumber;

typedef struct LocalVariable {
  U2 start_pc;
  U2 length;
  U2 name_index;
  U2 descriptor_index;
  U2 index;
} LocalVariable;

typedef struct ClassFile {
  int init;
  struct ClassFile *next, *super;
  U2 minor_version;
  U2 major_version;
  U2 constant_pool_count;
  struct CP **constant_pool;
  U2 access_flags;
  U2 this_class;
  U2 super_class;
  U2 interfaces_count;
  U2 *interfaces;
  U2 fields_count;
  struct Field **fields;
  U2 methods_count;
  struct Method **methods;
  U2 attributes_count;
  struct Attribute **attributes;
} ClassFile;

int class_getnoperands(U1 instruction);
Attribute *class_getattr(Attribute **attrs, U2 count, AttributeTag tag);
char *class_getutf8(ClassFile *class, U2 index);
char *class_getclassname(ClassFile *class, U2 index);
char *class_getstring(ClassFile *class, U2 index);
int32_t class_getinteger(ClassFile *class, U2 index);
float class_getfloat(ClassFile *class, U2 index);
int64_t class_getlong(ClassFile *class, U2 index);
double class_getdouble(ClassFile *class, U2 index);
void class_getnameandtype(ClassFile *class, U2 index, char **name, char **type);
Method *class_getmethod(ClassFile *class, char *name, char *descr);
Field *class_getfield(ClassFile *class, char *name, char *descr);
int class_istopclass(ClassFile *class);
