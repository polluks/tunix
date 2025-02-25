#ifdef __CC65__
#include <ingle/cc65-charmap.h>
#pragma allow-eager-inline(on)
#pragma inline-stdfuncs(on)
#endif

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <string.h>

#include <simpleio/libsimpleio.h>

#include "liblisp.h"

#ifdef TARGET_UNIX
#include <stdio.h>
#include <signal.h>
#endif

#ifdef COMPRESSED_CONS
bool do_compress_cons;
#endif

lispptr ** gp;

#ifdef VERBOSE_GC
simpleio_chn_t gc_oldout;
#endif

#ifdef __CC65__
#pragma code-name ("CODE_GC")
#endif

// Trace and mark reachable objects.
void FASTCALL
mark (lispptr x)
{
    CHKPTR(x);
    if (NOT_NIL(x) && !MARKED(x)) {
        MARK(x);
        for (; _CONSP(x); x = CDR(x)) {
            CHKPTR(x);
            MARK(x);
            mark (CAR(x));
        }
        CHKPTR(x);
        if (NOT_NIL(x)) {
            MARK(x);
            if (_SYMBOLP(x))
                mark (SYMBOL_VALUE(x));
        }
    }
}

xlat_item * xlat_end;
bool        xlat_full;

#ifdef USE_ZEROPAGE
#pragma bss-name (push, "ZEROPAGE")
#endif

lispobj_size_t n;

char * s;   // Source
char * d;   // Destination

xlat_item * xlat;
xlat_item * xlat_start;
char *      last_sweeped; // For merging consecutive gaps.
size_t      gapsize;

char *      p;
xlat_item * r;

#ifdef FRAGMENTED_HEAP
size_t total_removed;
#endif

#ifdef USE_ZEROPAGE
#pragma zpsym ("n")
#pragma zpsym ("s")
#pragma zpsym ("d")
#pragma zpsym ("xlat")
#pragma zpsym ("gapsize")
#pragma zpsym ("p")
#pragma zpsym ("r")
#pragma bss-name (pop)
#ifdef FRAGMENTED_HEAP
#pragma zpsym ("total_removed")
#endif // #ifdef FRAGMENTED_HEAP
#endif // #ifdef USE_ZEROPAGE

void FASTCALL
add_gap (lispobj_size_t n)
{
    // Log gap position and size.
    xlat--;
    xlat->pos  = s;
    xlat->size = n;

#ifdef FRAGMENTED_HEAP
    total_removed += n;
#endif
}

// Copy marked objects over deleted ones and make a
// relocation table containing the addresses and sizes of
// the deleted objects.
void
sweep ()
{
#ifdef VERBOSE_GC
    simpleio_chn_t gc_oldout = fnout;
#endif

#ifdef FRAGMENTED_HEAP
    // Start with first heap.
    struct heap_fragment * heap = heaps;
#endif

    // Invalidate pointer to last sweeped object.
    // Required to merge gaps.
    last_sweeped = nil;

    first_symbol = nil;
    last_symbol = nil;

    // Initialize relocation table.
    xlat = xlat_end;    // Point to its start.
    xlat_full = false;  // Mark it as not full.

#if defined(DUMP_MARKED) || defined(DUMP_SWEEPED)
    setout (STDOUT);
    outs ("Sweep objects:");
    terpri ();
    setout (gc_oldout);
#endif

    // Get heap pointers.
#ifdef FRAGMENTED_HEAP
    do {
        total_removed = 0;
        heap_start = heap->start;
        heap_free  = heap->free;
#endif // #ifdef FRAGMENTED_HEAP

#ifdef VERBOSE_GC
    setout (STDOUT);
    out ('S');
    setout (gc_oldout);
#endif

        // Sweep one heap.
        s = d = heap_start;
        while (*s) {
            // Get size of object.
            n = objsize (s);

            // Keep/copy marked object.
            if (MARKED(s) || xlat_full) {
#ifdef DUMP_MARKED
                dump_lispptr (s);
#endif
                // Link this and last named symbol.
                if (_NAMEDP(s) && SYMBOL_LENGTH(s)) {
                    if (NOT_NIL(last_symbol))
                        SET_SYMBOL_NEXT(last_symbol, d);
                    last_symbol = d;
                    if (NOT(first_symbol))
                        first_symbol = d;
                }
#ifdef COMPRESSED_CONS
                // Turn regular cons into compressed cons...
                else if (do_compress_cons && !xlat_full && _CONSP(s) && !_EXTENDEDP(s)) {
                    // ...if CDR is pointing to the following object.
                    if (CONS(s)->cdr == s + sizeof (cons)) {
#ifdef VERBOSE_COMPRESSED_CONS
                        setout (STDOUT);
                        out ('C');
                        setout (gc_oldout);
#endif

                        // Copy with mark bit cleard and type extended.
                        *d = (*s & ~TYPE_MARKED) | TYPE_EXTENDED;

                        // Copy CAR.
                        CONS(d)->car = CONS(s)->car;

                        // Advance.
                        d += sizeof (ccons);
                        s += sizeof (cons);

                        // Add gap to relocation table.
                        add_gap (sizeof (cons) - sizeof (ccons));

                        goto check_xlat;
                    }
                }
#endif // #ifdef COMPRESSED_CONS

#ifdef SKIPPING_SWEEP
                // Clear mark bit.
                if (s == d) {
                    *d &= ~TYPE_MARKED;
                    s += n;
                    d = s;
                } else {
#endif
                    // Copy object with mark bit cleared.
                    *d++ = *s++ & ~TYPE_MARKED;
                    while (--n)
                        *d++ = *s++;
#ifdef SKIPPING_SWEEP
                }
#endif
            } else {
#ifdef DUMP_SWEEPED
                dump_lispptr (s);
#endif
                // Remove object.
                if (last_sweeped == d) {
                    xlat->size += n;
#ifdef FRAGMENTED_HEAP
                    total_removed += n;
#endif
                } else {
                    last_sweeped = d;
                    add_gap (n);
                }

                // Step to next object.
                s += n;

#ifdef COMPRESSED_CONS
check_xlat:
#endif
#ifdef PARANOID
                if (xlat < xlat_start)
                    // Reloc table size must be a multiple of an entry's size!
                    internal_error ("xlat overflow");
#endif
                // Flag if relocation table is full, so
                // the rest of the objects won't be sweeped
                // and GC is started again.
                if (xlat == xlat_start)
                    xlat_full = true;
            }
        }

        // Mark end of heap.
        *d = 0;

#ifdef FRAGMENTED_HEAP
        // Save free pointer.
        heap->free = d;

        // Undo address shifting with negative gap size entry in
        // order to not affect the following heap's pointers.
        xlat--;
        xlat->pos  = s;
        xlat->size = -total_removed;
        if (xlat == xlat_start)
            xlat_full = true;
#ifdef VERBOSE_GC
        setout (STDOUT);
        outn (total_removed);
        outs ("B freed.");
        terpri ();
        setout (gc_oldout);
#endif // #ifdef VERBOSE_GC
    } while ((++heap)->start);
#else // #ifdef FRAGMENTED_HEAP
    // Save free pointer.
    heap_free = d;
#endif // #ifdef FRAGMENTED_HEAP

    // End symbol list.
    SET_SYMBOL_NEXT(last_symbol, nil);
}

// Relocate object pointer in "tmpstr".
lispptr
relocate_ptr (void)
{
    // Sum up gap sizes up to the pointer.
    gapsize = 0;
    for (r = xlat_end; r != xlat;) {
        r--;
        if (r->pos > (lispptr) tmpstr)
            break;
        gapsize += r->size;
    }

    // Subtract it from the pointer.
    return tmpstr - gapsize;
}

// Relocate object pointers on heap, stack, and in global vars.
void
relocate (void)
{
    // Relocate global pointers.
    for (gp = global_pointers; *gp; gp++) {
        tmpstr = **gp;
        **gp = relocate_ptr ();
    }

    // Relocate GC'ed stack.
    for (p = stack; p != stack_end; p += sizeof (lispptr)) {
        tmpstr = *(lispptr *) p;
        *(lispptr *)p = relocate_ptr ();
    }

#ifdef FRAGMENTED_HEAP
    heap = heaps;
    do {
        heap_start = heap->start;
#endif
#ifdef VERBOSE_GC
    setout (STDOUT);
    out ('R');
    setout (gc_oldout);
#endif

        // Relocate elements on heap.
        for (p = heap_start; *p; p += objsize (p)) {
            CHKPTR(p);
#ifdef PARANOID
            if (p >= heap_end)
                internal_error ("Reloc: heap overflow");
#endif
            if (_CONSP(p)) {
                tmpstr = CAR(p);
                SETCAR(p, relocate_ptr ());
#ifdef COMPRESSED_CONS
                if (!_EXTENDEDP(p)) {
#endif
                    tmpstr = CDR(p);
                    SETCDR(p, relocate_ptr ());
#ifdef COMPRESSED_CONS
                }
#endif
            } else if (_SYMBOLP(p)) {
                tmpstr = SYMBOL_VALUE(p);
                SET_SYMBOL_VALUE(p, relocate_ptr ());
            }
        }
#ifdef FRAGMENTED_HEAP
    } while ((++heap)->start);
#endif

#ifdef VERBOSE_GC
    setout (STDOUT);
    terpri ();
    setout (gc_oldout);
#endif
}

#ifdef __CC65__
#pragma allow-eager-inline (off)
#pragma inline-stdfuncs (off)
#endif

// Mark and sweep objects, and relocate object pointers.
void
gc (void)
{
#ifdef VERBOSE_GC
    gc_oldout = fnout;
#endif

#ifdef FRAGMENTED_HEAP
    // Switch to next heap if available.
    if (heap->start) {
#ifdef VERBOSE_GC
        setout (STDOUT);
        out ('N');
        setout (gc_oldout);
#endif
        goto next_heap;
    }
#endif

#ifdef RESTART_GC_ON_FULL_RELOC
restart:
#endif
#ifdef FRAGMENTED_HEAP
    // Switch to first heap.
    heap = heaps;
    switch_heap ();
#endif

#ifdef VERBOSE_GC
    setout (STDOUT);
    out ('M');
    setout (gc_oldout);
#endif

    // Mark global pointers.
#ifdef GC_DIAGNOSTICS
    setout (STDOUT);
    outs ("Mark globals: ");
    setout (gc_oldout);
#endif
    for (gp = global_pointers; *gp; gp++) {
#ifdef GC_DIAGNOSTICS
        setout (STDOUT);
        outhw ((int) *gp);
        out (' ');
        setout (gc_oldout);
#endif
        mark (**gp);
    }

    // Mark GC'ed stack.
#ifdef GC_DIAGNOSTICS
    setout (STDOUT);
    outs ("Mark stack: ");
    setout (gc_oldout);
#endif
    for (p = stack; p != stack_end; p += sizeof (lispptr)) {
#ifdef GC_DIAGNOSTICS
        setout (STDOUT);
        outhw ((int) *p);
        out (' ');
        setout (gc_oldout);
#endif
        mark (*(lispptr *) p);
    }
#ifdef GC_DIAGNOSTICS
    setout (STDOUT);
    outs ("Sweep: ");
    setout (gc_oldout);
#endif

    // Append used objects over unused ones, freeing space.
    // Log to gap table.
    sweep ();

    // Update pointers according to gap list.
    relocate ();

#ifdef RESTART_GC_ON_FULL_RELOC
    // Restart if sweep was interrupted due
    // to full relocation table.
    if (xlat_full) {
#ifdef VERBOSE_GC
        setout (STDOUT);
        outs ("!GC restart!");
        setout (gc_oldout);
#endif
        goto restart;
    }
#else // #ifdef RESTART_GC_ON_FULL_RELOC
#ifdef VERBOSE_GC
    if (xlat_full) {
        setout (STDOUT);
        outs ("!full reloc!");
        setout (gc_oldout);
    }
#endif
#endif // #ifdef RESTART_GC_ON_FULL_RELOC

#ifdef FRAGMENTED_HEAP
    // Switch to first heap to allocate from there.
    heap = heaps;
next_heap:
    switch_heap ();
#endif
}
