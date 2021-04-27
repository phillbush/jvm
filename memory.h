/* virtual machine frame structure */
typedef struct Frame {
	struct Frame           *next;
	struct ClassFile       *class;          /* constant pool */
	union  Value           *local;          /* local variable table */
	union  Value           *stack;          /* operand stack */
	size_t                  max_locals;     /* local variable table */
	size_t                  max_stack;      /* operand stack */
	size_t                  nstack;         /* number of values on operand stack */
	struct Code_attribute  *code;           /* array of instructions */
	U2                      pc;             /* program counter */
} Frame;

Frame *frame_push(Code_attribute *code, ClassFile *class, U2 max_locals, U2 max_stack);
int frame_pop(void);
void frame_del(void);
void frame_stackpush(Frame *frame, Value value);
Value frame_stackpop(Frame *frame);
void frame_localstore(Frame *frame, U2 i, Value v);
Value frame_localload(Frame *frame, U2 i);
Heap *heap_alloc(int32_t nmemb, size_t size);
void *heap_use(Heap *entry);
int heap_free(Heap *heap);
Heap *array_new(int32_t *nmemb, U1 dimension, size_t size);
