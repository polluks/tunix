#include <stdio.h>
#include "defs.h"
#include "data.h"
#include "ir-codes.h"
#include "ir.h"
#include "sym.h"
#include "expr.h"
#include "lex.h"
#include "io.h"
#include "struct.h"
#include "gen.h"
#include "primary.h"

char sname[NAMESIZE];

int
primary (LVALUE * lval)
{
    int num[1]; // XXX Oh, dude! Did the first version know pointers? (smk)
    int reg, symbol_table_idx;
    SYMBOL *symbol;
    lval->ptr_type = 0;
    lval->tagsym = 0;
    if (match ("(")) {
        reg = hier1 (lval);
        needbrack (")");
        return reg;
    }
    if (amatch ("sizeof", 6)) {
        dosizeof (lval);
        return 0;
    }
    if (symname (sname)) {
        if ((symbol_table_idx = find_local (sname)) > -1) {
            symbol = &symbol_table[symbol_table_idx];
            return primary_local (lval, symbol);
        }
        if ((symbol = find_global (sname))
            && symbol->identity != FUNCTION)
            return primary_global (lval, symbol);
        blanks ();
        if (ch () != '(') {
            perror ("undeclared variable");
            return 0;
        }
        return primary_function (lval, sname);

    }
    if (constant (num)) {
        lval->symbol = 0;
        lval->indirect = 0;
        return 0;
    }
    gen_ldaci (0);
    junk ();
    perror ("invalid expression");
    return 0;
}

void
dosizeof (LVALUE * lval)
{
    int otag, offset, symbol_table_idx;
    SYMBOL *symbol;
    needbrack ("(");
    outb (IR_LDACI);
    if (amatch ("int", 3)
        || amatch ("unsigned int", 12)
        || amatch ("unsigned", 8)) {
        blanks ();
        // Pointers and ints are both INTSIZE.
        match ("*");
        outn (INTSIZE);
    } else if (amatch ("char", 4)
               || amatch ("unsigned char", 13)) {
        // If sizeof a char pointer, output INTSIZE.
        outn (match ("*") ?
                INTSIZE :
                1);
    } else if (amatch ("struct", 6)) {
        if (!symname (sname))
            illname ();
        if ((otag = find_tag (sname)) == -1) {
            perror ("sizeof(): struct tag undefined");
            return;
        }
        // Write out struct size, or INTSIZE
        // if struct pointer .
        outn (match ("*") ?
                INTSIZE :
                tags[otag].size);
    } else if (symname (sname)) {
        if (((symbol_table_idx = find_local (sname)) > -1)
            || (symbol = find_global (sname))) {
            if (symbol_table_idx > -1)
                symbol = &symbol_table[symbol_table_idx];
            if (symbol->storage == LSTATIC) {
                perror ("sizeof(): local static");
                return;
            }
            offset = symbol->offset;
            if (symbol->type & CINT
                || symbol->identity == POINTER)
                offset *= INTSIZE;
            else if (symbol->type == STRUCT)
                offset *= tags[symbol->tag].size;
            outn (offset);
        } else {
            perror ("sizeof(): undeclared variable");
            return;
        }
    } else {
        perror ("sizeof(): only on type or variable");
        return;
    }
    needbrack (")");
    lval->symbol = 0;
    lval->indirect = 0;
}

int
primary_local (LVALUE *lval, SYMBOL *symbol)
{
    int reg;
    reg = gen_get_local (symbol);
    lval->symbol = symbol;
    lval->indirect = symbol->type;
    if (symbol->type == STRUCT)
        lval->tagsym = &tags[symbol->tag];
    if (symbol->identity == ARRAY ||
        (symbol->identity == VARIABLE
         && symbol->type == STRUCT)) {
        lval->ptr_type = symbol->type;
        return reg;
    }
    if (symbol->identity == POINTER) {
        lval->indirect = CINT;
        lval->ptr_type = symbol->type;
    }
    return FETCH | reg;
}

int
primary_global (LVALUE *lval, SYMBOL *symbol)
{
    lval->symbol = symbol;
    lval->indirect = 0;
    if (symbol->type == STRUCT)
        lval->tagsym = &tags[symbol->tag];
    if (symbol->identity != ARRAY
        && (symbol->identity != VARIABLE
            || symbol->type != STRUCT)) {
        if (symbol->identity == POINTER)
            lval->ptr_type = symbol->type;
        return FETCH | REGA;
    }
    gen_ldacig (symbol->name);
    lval->indirect = symbol->type;
    lval->ptr_type = symbol->type;
    return 0;
}

int
primary_function (LVALUE *lval, char *sname)
{
    lval->symbol = add_global (sname, FUNCTION, CINT, 0, PUBLIC);
    lval->indirect = 0;
    return 0;
}

// true if val1 -> int pointer or int
// array and val2 not pointer or array.
// TODO: better name (smk)
int
dbltest (LVALUE * val1, LVALUE * val2)
{
    if (val1 == NULL)
        return FALSE;
    if (val1->ptr_type) {
        if (val1->ptr_type & CCHAR)
            return FALSE;
        if (val2->ptr_type)
            return FALSE;
        return TRUE;
    }
    return FALSE;
}

// Determine type of binary operation.
void
result (LVALUE * lval, LVALUE * lval2)
{
    if (lval->ptr_type && lval2->ptr_type)
        lval->ptr_type = 0;
    else if (lval2->ptr_type) {
        lval->symbol = lval2->symbol;
        lval->indirect = lval2->indirect;
        lval->ptr_type = lval2->ptr_type;
    }
}

int
constant (int val[])
{
    if (number (val))
        gen_ldaci (val[0]);
    else if (quoted_char (val))
        gen_ldaci (val[0]);
    else if (quoted_string (val)) {
        gen_ldaci (val[0]);
        gen_local (litlab);
        outb ('+');
    } else
        return 0;
    return 1;
}

int
number (int val[])
{
    int k, minus, base;
    char c;

    k = minus = 1;
    while (k) {
        k = 0;
        if (match ("+"))
            k = 1;
        if (match ("-")) {
            minus = (-minus);
            k = 1;
        }
    }
    if (!numeric (c = ch ()))
        return 0;
    if (match ("0x") || match ("0X"))
        while (numeric (c = ch ())
               || (c >= 'a' && c <= 'f')
               || (c >= 'A' && c <= 'F')) {
            inbyte ();
            k = k * 16 + (numeric (c) ? (c - '0') : ((c & 07) + 9));
    } else {
        base = (c == '0') ? 8 : 10;
        while (numeric (ch ())) {
            c = inbyte ();
            k = k * base + (c - '0');
        }
    }
    if (minus < 0)
        k = -k;
    val[0] = k;
    if (k < 0)
        return UINT;
    return CINT;
}

// Test if we have one char enclosed in
// single quotes.
// @param value returns the char found
// @return 1 if we have, 0 otherwise
int
quoted_char (int *value)
{
    int k;
    char c;

    k = 0;
    if (!match ("'"))
        return 0;
    while ((c = gch ()) != '\'') {
        c = (c == '\\') ? spechar () : c;
        k = (k & 255) * 256 + (c & 255);
    }
    *value = k;
    return 1;
}

// Test if we have string enclosed in
// double quotes. e.g. "abc".
// Load the string into literal pool.
// @param position returns beginning of
// the string.
// @return 1 if such string found,
// 0 otherwise
int
quoted_string (int *position)
{
    char c;

    if (!match ("\""))
        return 0;
    *position = litptr;
    while (ch () != '"') {
        if (!ch ())
            break;
        if (litptr >= LITMAX) {
            perror ("string space exhausted");
            while (!match ("\""))
                if (!gch ())
                    break;
            return 1;
        }
        c = gch ();
        litq[litptr++] = (c == '\\') ? spechar () : c;
    }
    gch ();
    litq[litptr++] = 0;
    return 1;
}

// Decode special characters (preceeded
// by back slashes).
int
spechar ()
{
    char c;

    c = ch ();
    if (c == 'n')
        c = LF;
    else if (c == 't')
        c = TAB;
    else if (c == 'r')
        c = CR;
    else if (c == 'f')
        c = FFEED;
    else if (c == 'b')
        c = BKSP;
    else if (c == '0')
        c = EOS;
    else if (c == EOS)
        return 0;
    gch ();
    return c;
}

// Perform a function call to label or primary.
// Called from hier11(), this routine
// will either call the named function,
// or if the supplied ptr is zero, will
// call the contents of HL.
// @param ptr name of the function.
void
callfunction (char *fname)
{
    int nargs;

    nargs = 0;
    blanks ();
    if (!fname)
        gen_push (REGA);
    while (!streq (line + lptr, ")")) {
        if (endst ())
            break;
        expression (NO);
        if (!fname)
            gen_swap_stack ();
        gen_push (REGA);
        nargs = nargs + INTSIZE;
        if (!match (","))
            break;
    }
    needbrack (")");
    if (fname)
        gen_call (fname);
    else
        callstk ();
    stkp = gen_modify_stack (stkp + nargs);
}

void
needlval ()
{
    perror ("must be lvalue");
}
