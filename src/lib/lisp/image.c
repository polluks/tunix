#ifndef NO_IMAGE

#ifdef __CC65__
#include <ingle/cc65-charmap.h>
#include <cbm.h>
#pragma allow-eager-inline (off)
#pragma inline-stdfuncs (off)
#endif

#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#ifdef TARGET_UNIX
#include <signal.h>
#endif

#include <simpleio/libsimpleio.h>
#include <lisp/liblisp.h>
#include <git-version.h>

image_header header;
lispptr ** tmpo;

#ifdef __CC65__
#pragma code-name ("CODE_IMAGE")
#endif

bool FASTCALL
image_save (char * pathname)
{
    simpleio_chn_t old_out = fnout;
    simpleio_chn_t chout;
    size_t len;

    // Open image file for writing.
    chout = simpleio_open (pathname, 'w');

    // Break if file couldn't be opened.
    if (!chout)
        return false;

    // Switch to image file's channel for writing.
    setout (chout);

    // Make header.
    memcpy (&header.git_version, TUNIX_GIT_SHA, sizeof (header.git_version));

    // Write header.
    outm ((char *) &header, sizeof (header));

#ifdef FRAGMENTED_HEAP
    // Start with first heap.
    heap = heaps;
    do {
        switch_heap ();
#endif
        // Write heap start.
        outm ((char *) &heap_start, sizeof (lispptr));

        // Write heap size.
        len = heap_free - heap_start;
        outm ((char *) &len, sizeof (len));

        // Write heap data.
        outm (heap_start, len);
#ifdef FRAGMENTED_HEAP
    } while (heap->start);
#endif

    // Write global pointers.
    for (tmpo = global_pointers; *tmpo; tmpo++)
        outm ((char *) *tmpo, sizeof (lispptr));

    // Close image file.
    simpleio_close (chout);

    // Restore old output channel.
    setout (old_out);

#ifdef FRAGMENTED_HEAP
    // Switch to first heap.
    heap = heaps;
    switch_heap ();
#endif

    // Signal success.
    return true;
}

bool FASTCALL
image_load (char * pathname)
{
    simpleio_chn_t old_in = fnin;
    simpleio_chn_t chin;
    size_t len;
    lispptr pos;

    // Open image file.
    chin = simpleio_open (pathname, 'r');
    if (!chin)
        return false;

    // Set input channel to image.
    setin (chin);

    // Read header.
    inm ((char *) &header, sizeof (header));

    // Verify that the Git version matches.
    // NOTE: It must be exactly the same machine
    // used to save the image anyhow.
    if (strncmp ((char *) &header.git_version, TUNIX_GIT_SHA, sizeof (header.git_version))) {
        simpleio_close (chin);
        setin (old_in);
        return false;
    }

#ifdef FRAGMENTED_HEAP
    // Start with first heap.
    heap = heaps;
    do {
        switch_heap ();
#endif

        // Read heap address.
        inm ((char *) &pos, sizeof (lispptr));
#ifndef NAIVE
        if (pos != heap_start)
            internal_error_ptr (pos, "image position");
#endif

        // Read heap size.
        inm ((char *) &len, sizeof (len));

        // Read heap data.
        inm (heap_start, len);

        heap_free = heap_start + len;
        *heap_free = 0; // Mark end of heap.

#ifdef FRAGMENTED_HEAP
        heap->free = heap_free;
    } while (heap->start);
#endif

    // Read global pointers.
    for (tmpo = global_pointers; *tmpo; tmpo++)
        inm ((char *) *tmpo, sizeof (lispptr));

    // Close file.
    simpleio_close (chin);

    // Standard output to be safe.
    setin (STDIN);

    // Initialize stack pointers.
    stack    = stack_end;
    tagstack = tagstack_end;

    // GC to set up linked list of named symbols.
#ifdef FRAGMENTED_HEAP
    heap_free = heap_end;  // Ensure start with first heap.
#endif
    gc ();

    // Signal success.
    return true;
}

#endif // #ifndef NO_IMAGE
