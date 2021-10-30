#include "memory.h"
#include "class.h"
#include "util.h"
#include <stdint.h>
#include <stdlib.h>

static Frame *framestack = NULL;
static Heap *heap = NULL;

/* allocate frame; push it onto framestack; and return it */
Frame *frame_push(Code_attribute *code, ClassFile *class, U2 max_locals,
                  U2 max_stack) {
  Frame *frame = NULL;
  Value *local = NULL;
  Value *stack = NULL;

  frame = malloc(sizeof *frame);
  local = calloc(max_locals, sizeof *local);
  stack = calloc(max_stack, sizeof *stack);
  if (frame == NULL || (max_locals && local == NULL) ||
      (max_stack && stack == NULL)) {
    free(frame);
    free(local);
    free(stack);
    return NULL;
  }
  frame->pc = 0;
  frame->code = code;
  frame->class = class;
  frame->local = local;
  frame->stack = stack;
  frame->max_locals = max_locals;
  frame->max_stack = max_stack;
  frame->nstack = 0;
  frame->next = framestack;
  framestack = frame;
  return frame;
}

/* pop and free frame from framestack; return -1 on error */
int frame_pop(void) {
  Frame *frame;

  if (framestack == NULL)
    return -1;
  frame = framestack;
  framestack = frame->next;
  free(frame->local);
  free(frame->stack);
  free(frame);
  return 0;
}

/* pop and free all frames from framestack */
void frame_del(void) {
  while (framestack) {
    frame_pop();
  }
}

/* push value onto frame's operand stack */
void frame_stackpush(Frame *frame, Value value) {
  // TODO: handle error when frame->nstack >= frame->max_stack
  frame->stack[frame->nstack++] = value;
}

/* push value onto frame's operand stack */
Value frame_stackpop(Frame *frame) {
  // TODO: handle error when --frame->nstack == 0
  if (frame->nstack == 0)
    errx(EXIT_FAILURE, "ASDA");
  return frame->stack[--frame->nstack];
}

/* store value into local variable array */
void frame_localstore(Frame *frame, U2 i, Value v) { frame->local[i] = v; }

/* get value from local variable array */
Value frame_localload(Frame *frame, U2 i) { return frame->local[i]; }

/* allocate entry in heap */
Heap *heap_alloc(int32_t nmemb, size_t size) {
  Heap *entry = NULL;
  void *obj = NULL;

  entry = malloc(sizeof *entry);
  if (nmemb) {
    obj = calloc(nmemb, size);
  }
  if (entry == NULL || (nmemb && obj == NULL)) {
    free(entry);
    free(obj);
    return NULL;
  }
  entry->nmemb = nmemb;
  entry->count = 0;
  entry->obj = obj;
  entry->prev = NULL;
  entry->next = heap;
  if (heap)
    heap->prev = entry;
  heap = entry;
  return heap;
}

/* use entry in heap */
void *heap_use(Heap *entry) {
  if (entry == NULL)
    return NULL;
  entry->count++;
  return entry->obj;
}

/* free entry in heap */
int heap_free(Heap *entry) {
  if (entry == NULL)
    return -1;
  if (entry->count == 1) {
    if (entry->nmemb) {
      free(entry->obj);
    }
    if (entry->next) {
      entry->next->prev = entry->prev;
    }
    if (entry->prev) {
      entry->prev->next = entry->next;
    } else {
      heap = entry->next;
    }
    free(entry);
  } else {
    entry->count--;
  }
  return 0;
}

/* recursivelly create multidimensional array */
Heap *array_new(int32_t *nmemb, U1 dimension, size_t size) {
  Heap *h;
  int32_t i;

  if (dimension == 1) {
    h = heap_alloc(*nmemb, size);
  } else {
    h = heap_alloc(*nmemb, sizeof(Heap *));
    for (i = 0; i < *nmemb; i++) {
      ((void **)h->obj)[i] = array_new(nmemb + 1, dimension - 1, size);
    }
  }
  return h;
}
