#include "buzzheap.h"
#include "buzzvm.h"
#include <stdlib.h>

/****************************************/
/****************************************/

#define BUZZHEAP_GC_INIT_MAXOBJS 20

/****************************************/
/****************************************/

void buzzheap_destroy_obj(uint32_t pos, void* data, void* params) {
   buzzobj_t o = *(buzzobj_t*)data;
   free(o);
}

buzzheap_t buzzheap_new() {
   /* Create heap state */
   buzzheap_t h = (buzzheap_t)malloc(sizeof(struct buzzheap_s));
   /* Create object list */
   h->objs = buzzdarray_new(10, sizeof(buzzobj_t), buzzheap_destroy_obj);
   /* Initialize GC max object threshold */
   h->max_objs = BUZZHEAP_GC_INIT_MAXOBJS;
   /* All done */
   return h;
}

/****************************************/
/****************************************/

void buzzheap_destroy(buzzheap_t* h) {
   /* Get rid of object list */
   buzzdarray_destroy(&((*h)->objs));
   /* Get rid of heap state */
   free(*h);
   /* Set heap to NULL */
   *h = NULL;
}

/****************************************/
/****************************************/

buzzobj_t buzzheap_newobj(buzzheap_t h,
                          uint16_t type) {
   /* Create a new object. calloc() filles it with zeroes */
   buzzobj_t o = (buzzobj_t)calloc(1, sizeof(union buzzobj_u));
   /* Set the object type */
   o->o.type = type;
   /* Add object to list */
   buzzdarray_push(h->objs, &o);
   /* All done */
   return o;
}

/****************************************/
/****************************************/

void buzzheap_objmark(buzzobj_t o,
                      uint16_t m) {
   if(o->o.marker == m) return;
   else {
      o->o.marker = m;
      /* Take care of composite types */
      // TODO
   }
}

void buzzheap_stackobj_mark(uint32_t pos,
                            void* data,
                            void* params) {
   buzzobj_t o = *(buzzobj_t*)data;
   buzzheap_t h = (buzzheap_t)params;
   buzzheap_objmark(o, h->marker);
}

void buzzheap_vstigobj_mark(const void* key,
                            void* data,
                            void* params) {
   buzzobj_t o = ((buzzvstig_elem_t*)data)->data;
   buzzheap_t h = (buzzheap_t)params;
   buzzheap_objmark(o, h->marker);
}

void buzzheap_vstig_mark(uint32_t pos,
                          void* data,
                          void* params) {
   buzzvstig_foreach(*(buzzvstig_t*)data, buzzheap_vstigobj_mark, params);
}

void buzzheap_gc(struct buzzvm_s* vm,
                 buzzheap_t h) {
   /* Increase the marker */
   ++h->marker;
   /* Go through all the objects in the VM stack and mark them */
   buzzdarray_foreach(vm->stack, buzzheap_stackobj_mark, h);
   /* Go through all the objects in the virtual stigmergy and mark them */
   buzzdarray_foreach(vm->stack, buzzheap_vstig_mark, h);
   /* Go through all the objects in the object list and delete the unmarked ones */
   int64_t i = buzzdarray_size(h->objs) - 1;
   while(i >= 0) {
      /* Check whether the marker is set to the latest value */
      if(buzzdarray_get(h->objs, i, buzzobj_t)->o.marker != h->marker) {
         /* No, erase the element */
         buzzdarray_remove(h->objs, i);
      }
      /* Next element */
      --i;
   }
}

/****************************************/
/****************************************/
