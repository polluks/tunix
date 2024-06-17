#ifdef __CC65__
#ifndef __CBM__
#define __CBM__
#endif

#include <ingle/cc65-charmap.h>
#include <cbm.h>
#endif

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include <simpleio/libsimpleio.h>

#include "liblisp.h"

void print0 (lispptr);

void
space (void)
{
    if (last_out != '('
        && last_out != ')'
        && last_out != '\''
        && last_out > ' ')
        out (' ');
}

void FASTCALL
print_list (cons * c)
{
    bool first = true;

    if (CAR(c) == quote && CDR(c)) {
        out ('\'');
        print0 (CAR(CDR(c)));
        return;
    }

    out ('(');
    while (c) {
        if (!first)
            out (' ');
        else
            first = false;
        print0 (c->car);
        if (c->cdr && !CONSP(c->cdr)) {
            outs (" . ");
            print0 (c->cdr);
            break;
        }
        c = c->cdr;
    }
    out (')');
}

void FASTCALL
print_number (number * n)
{
    space ();
    outnu (n->value);
}

bool
needs_quotes (symbol * s)
{
    char * p = SYMBOL_NAME(s);
    char len = SYMBOL_LENGTH(s);
    char c;
    for (; len--; p++) {
        c = *p;
        if (c == '"' || c == ' ' || c == '(' || c == ')')
            return true;
    }
    return false;
}

void
print_quoted_string (symbol * s)
{
    char * p = SYMBOL_NAME(s);
    char len = SYMBOL_LENGTH(s);
    char c;
    out ('"');
    for (; len--; p++) {
        c = *p;
        if (c == '"')
            out ('\\');
        out (c);
    }
    out ('"');
}

void FASTCALL
print_named (symbol * s)
{
    space ();
    if (needs_quotes (s)) {
        print_quoted_string (s);
        return;
    }
    outsn (SYMBOL_NAME(s), SYMBOL_LENGTH(s));
}

void
print0 (lispptr x)
{
    uchar type;

    if (!x) {
        outs ("nil");
        return;
    }
    type = TYPE(x);
    if (type == TYPE_CONS)
        print_list ((cons *) x);
    else if (type == TYPE_NUMBER)
        print_number ((number *) x);
    else if (PTRTYPE(x) & TYPE_NAMED)
        print_named ((symbol *) x);
    else
        error ("Unknown object type.");
}

lispptr FASTCALL
print (lispptr x)
{
    print0 (x);
    return x;
}
