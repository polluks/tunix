#include <stdlib.h>

#include "error.h"
#include "obj.h"
#include "libgfx.h"

void *
alloc_obj (short size, short x, short y, short w, short h)
{
    struct obj * obj = malloc (size);
    struct treenode * node;
    struct rect * rect;

    if (!obj)
        error_out_of_heap_memory ();

    node = &obj->node;
    node->prev = NULL;
    node->next = NULL;
    node->parent = NULL;
    node->children = NULL;

    rect = &obj->rect;
    rect->x = x;
    rect->y = y;
    rect->w = w;
    rect->h = h;

    return obj;
}

void __fastcall__
free_obj (struct obj * x)
{
    struct obj * c;

    while (x) {
        c = x->node.children;
        if (c)
            free_obj (c);
        free (x);
        x = x->node.next;
    }
}

void __fastcall__
draw_obj (struct obj * x)
{
    while (x) {
        x->draw (x);
        x = x->node.next;
    }
}
