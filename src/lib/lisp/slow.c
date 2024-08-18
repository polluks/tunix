#ifdef __CC65__
#include <ingle/cc65-charmap.h>
#include <cbm.h>
#endif

#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>

#include <simpleio/libsimpleio.h>
#include <lisp/liblisp.h>

#ifdef __CC65__
#pragma bss-name (push, "ZEROPAGE")
#endif
lispptr tmp_pop;
#ifdef __CC65__
#pragma zpsym ("tmp_pop")
#pragma bss-name (pop)
#endif

#ifdef __CC65__
#pragma code-name ("CODE_SLOW")
#endif

lispptr FASTCALL
lisp_car (lispptr x)
{
    CHKPTR(x);
    return CONS(x)->car;
}

lispptr FASTCALL
lisp_cdr (lispptr x)
{
    CHKPTR(x);
#ifdef COMPRESSED_CONS
    if (_EXTENDEDP(x))
        return CCONS_CDR(x);
#endif
    return CONS(x)->cdr;
}

void FASTCALL
lisp_setcar (lispptr x, lispptr v)
{
    CHKPTR(x);
    _SETCAR(x, v);
}

void FASTCALL
lisp_setcdr (lispptr x, lispptr v)
{
    CHKPTR(x);
    _SETCDR(x, v);
}

bool FASTCALL
lisp_consp (lispptr x)
{
    CHKPTR(x);
    return _CONSP(x);
}

bool FASTCALL
lisp_atom (lispptr x)
{
    CHKPTR(x);
    return _ATOM(x);
}

bool FASTCALL
lisp_listp (lispptr x)
{
    CHKPTR(x);
    return _LISTP(x);
}

bool FASTCALL
lisp_numberp (lispptr x)
{
    CHKPTR(x);
    return _NUMBERP(x);
}

bool FASTCALL
lisp_symbolp (lispptr x)
{
    CHKPTR(x);
    return _SYMBOLP(x);
}

bool FASTCALL
lisp_builtinp (lispptr x)
{
    CHKPTR(x);
    return _BUILTINP(x);
}

bool FASTCALL
lisp_specialp (lispptr x)
{
    CHKPTR(x);
    return _SPECIALP(x);
}

void FASTCALL
pushgc (lispptr x)
{
    CHKPTR(x);
    GCSTACK_CHECK_OVERFLOW();
    stack -= sizeof (lispptr);
    *(lispptr *) stack = x;
}

lispptr
popgc ()
{
    GCSTACK_CHECK_UNDERFLOW();
    tmp_pop = *(lispptr *) stack;
    CHKPTR(tmp_pop);
    stack += sizeof (lispptr);
    return tmp_pop;
}

void FASTCALL
pushtag (char x)
{
    TAGSTACK_CHECK_OVERFLOW();
    *--tagstack = x;
}

char
poptag ()
{
    TAGSTACK_CHECK_UNDERFLOW();
    return *tagstack++;
}

void FASTCALL
pushtagw (lispptr x) // TODO: Never pass a lispptr!
{
    TAGSTACK_CHECK_OVERFLOW();
    tagstack -= sizeof (lispptr);
    *(lispptr *) tagstack = x;
}

lispptr
poptagw ()
{
    TAGSTACK_CHECK_UNDERFLOW();
    tmp_pop = *(lispptr *) tagstack;
    tagstack += sizeof (lispptr);
    return tmp_pop;
}
