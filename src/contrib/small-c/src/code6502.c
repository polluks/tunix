#include <stdio.h>
#include "defs.h"
#include "data.h"

/*
  Some predefinitions:

  Define ASNM and LDNM to the names of
  the assembler and linker respectively

  INTSIZE is the size of an integer in
      the target machine:
  BYTEOFF is the offset of an byte
      within an integer on the target
      machine. (ie: 8080,pdp11 = 0,
                    6809 = 1,
                    360 = 3)

  This compiler assumes that an integer
  is the SAME length as a pointer - in
  fact, the compiler uses INTSIZE for
  both.
*/

void
initmac ()
{
    defmac ("CPU6502 1");
    defmac ("SMALLC 1");
}

////////////
/// FILE ///
////////////

// Print all assembler info before any
// code is generated.
header ()
{
    output_string (
        "; Generated by Small C\n"
        "; MOS 6502 CPU backend\n"
    );
    frontend_version ();
    newline ();
}

trailer ()
{
}

////////////////
/// Assembly ///
////////////////

newline ()
{
#if __CYGWIN__ == 1
    output_byte (CR);
#endif
    output_byte (LF);
}

// Label prefix
output_label_prefix ()
{
    output_byte ('_');
}

// Label definition terminator
output_label_terminator ()
{
    output_byte (':');
}

// Comment line start.
gen_comment ()
{
    output_byte (';');
}

// Code (text) segment.
code_segment_gtext ()
{
    output_line (".code");
}

// Data segment
data_segment_gdata ()
{
    output_line (".data");
}

// Import/export variable symbol at
// scptr.
ppubext (SYMBOL * scptr)
{
    if (symbol_table[current_symbol_table_idx].storage ==
        STATIC)
        return;
    output_with_tab (scptr->storage ==
                     EXTERN ? ".import " : ".export ");
    output_string (scptr->name);
    newline ();
}

// Import/export function symbol at
// scptr.
fpubext (SYMBOL * scptr)
{
    if (scptr->storage == STATIC)
        return;
    output_with_tab (scptr->offset ==
                     FUNCTION ? ".export " : ".import ");
    output_string (scptr->name);
    newline ();
}

// Decimal number.
output_number (num)
int num;
{
    output_decimal (num);
}

// Load memory into primary register.
gen_get_memory (SYMBOL * sym)
{
    if ((sym->identity != POINTER) && (sym->type == CCHAR)) {
        output_with_tab ("lda ");
        output_string (sym->name);
        newline ();
        gen_call ("ccsxt");
    } else if ((sym->identity != POINTER)
               && (sym->type == UCHAR)) {
        output_with_tab ("lda ");
        output_string (sym->name);
        newline ();
        output_line ("sta l");
        output_line ("lda #0");
        output_line ("sta h");
    } else {
        output_with_tab ("lhl ");
        output_string (sym->name);
        newline ();
    }
}

// Load address of symbol into primary.
// @return which register pair contains
// the result.
gen_get_local (SYMBOL * sym)
{
    if (sym->storage == LSTATIC) {
        gen_immediate ();
        print_label (sym->offset);
        newline ();
        return HL_REG;
    } else {
        if (uflag && !(sym->identity == ARRAY)) {       /* ||
                                                           (sym->identity == VARIABLE && sym->type == STRUCT))) { */
            output_with_tab ("ldei ");
            output_number (sym->offset - stkp);
            newline ();
            return DE_REG;
        } else {
            gen_immediate ();
            output_number (sym->offset - stkp);
            newline ();
            gen_call ("add_sp");
            return HL_REG;
        }
    }
}

// Store primary register at symbol
// address.
void
gen_put_memory (SYMBOL * sym)
{
    if ((sym->identity != POINTER) && (sym->type & CCHAR)) {
        output_line ("lda l");
        output_with_tab ("sta ");
    } else
        output_with_tab ("sthl ");
    output_string (sym->name);
    newline ();
}

// Store the specified object type in
// primary at the address in the
// secondary register (on the top of the
// stack).
void
gen_put_indirect (char typeobj)
{
    gen_pop ();
    if (typeobj & CCHAR)
        gen_call ("ccpchar"); 
    else
        gen_call ("ccpint");
}

// Fetch the specified object type
// indirect through the primary register
// into the primary register.
void
gen_get_indirect (char typeobj, int reg)
{
    if (typeobj == CCHAR) {
        if (reg & DE_REG)
            gen_swap ();
        gen_call ("ccgchar");
    } else if (typeobj == UCHAR) {
        if (reg & DE_REG)
            gen_swap ();
        //gen_call("cguchar");
        output_line ("ldy #0");
        output_line ("lda (hl),y");
        output_line ("sta l");
        output_line ("sty h");
    } else {                    //int 
        if (uflag) {
            if (reg & HL_REG)
                gen_swap ();
            output_line ("lhlx");
        } else
            gen_call ("ccgint");
    }
}

// Swap the primary and secondary
// registers.
gen_swap ()
{
    gen_call ("xchg");
}

// Print partial instruction to get an
// immediate value into primary
// register.
gen_immediate ()
{
    output_with_tab ("lhli ");
}

// Push the primary register onto the
// stack.
gen_push (int reg)
{
    if (reg & DE_REG) {
        output_line ("push de");
        stkp = stkp - INTSIZE;
    } else {
        output_line ("push hl");
        stkp = stkp - INTSIZE;
    }
}

// Pop the top of the stack into
// secondary register.
gen_pop ()
{
    output_line ("pop de");
    stkp = stkp + INTSIZE;
}

// Swap the primary register and top
// of the stack.
gen_swap_stack ()
{
    output_line ("xthl");
}

// Call subroutine.
gen_call (char *sname)
{
    output_with_tab ("jsr ");
    output_string (sname);
    newline ();
}

// Entry point declaration.
declare_entry_point (char *symbol_name)
{
    output_string (symbol_name);
    output_label_terminator ();
    newline(); 
}

// Return from subroutine.
gen_ret ()
{
    output_line ("rts");
}

// Perform subroutine call to value on
// top of stack.
callstk ()
{
    gen_immediate ();
    output_string ("#.+5");
    newline ();
    gen_swap_stack ();
    gen_call ("callptr");
    stkp = stkp + INTSIZE;
}

// Jump.
gen_jump (label)
int label;
{
    output_with_tab ("jmp ");
    print_label (label);
    newline ();
}

// test the primary register and jumps
// to label conditionally.
// If 'ft' is true, jumps if not zero.
gen_test_jump (label, ft)
int label, ft;
{
    output_line ("lda h");
    output_line ("ora l");
    if (ft)
        output_with_tab ("bne ");
    else
        output_with_tab ("beq ");
    print_label (label);
    newline ();
}

// Define byte.
gen_def_byte ()
{
    output_with_tab (".byte ");
}

// Define word.
gen_def_word ()
{
    output_with_tab (".word ");
}

// Define BSS byte.
gen_def_storage ()
{
    output_with_tab (".res ");
}

// Update stack pointer to new position.
gen_modify_stack (int newstkp)
{
    int k;

    k = newstkp - stkp;
    if (k == 0)
        return (newstkp);
    if (k > 0) {
        if (k < 7) {
            if (k & 1) {
                output_line ("tsx");
                output_line ("inx");
                output_line ("txs");
                k--;
            }
            while (k) {
                output_line ("pla");
                output_line ("sta b");
                k = k - INTSIZE;
            }
            return (newstkp);
        }
    } else {
        if (k > -7) {
            if (k & 1) {
                output_line ("tsx");
                output_line ("dex");
                output_line ("txs");
                k++;
            }
            while (k) {
                output_line ("lda b");
                output_line ("pha");
                k = k + INTSIZE;
            }
            return (newstkp);
        }
    }
    gen_swap ();
    gen_immediate ();
    output_number (k);
    newline ();
    gen_call ("add_sp");
    output_line ("sphl");
    gen_swap ();
    return (newstkp);
}

// Multiply primary register by INTSIZE.
gen_multiply_by_two ()
{
    gen_call ("asl_hl");
}

// Divide primary register by INTSIZE.
gen_divide_by_two ()
{
    // push primary in prep for
    // gen_arithm_shift_right ().
    gen_push (HL_REG);
    gen_immediate ();
    output_number (1);
    newline ();
    gen_arithm_shift_right ();
}

// 'case' jump
gen_jump_case ()
{
    output_with_tab ("jmp cccase");
    newline ();
}

// Sum of primary and secondary register
// If lval2 is int pointer and lval is
// not, scale lval.
gen_add (lval, lval2)
int *lval, *lval2;
{
    gen_pop ();
    if (dbltest (lval2, lval)) {
        gen_swap ();
        gen_multiply_by_two ();
        gen_swap ();
    }
    gen_call ("add_de");
}

// Subtract primary register from
// secondary.
gen_sub ()
{
    gen_pop ();
    gen_call ("ccsub");
}

// Multiply primary and secondary
// registers (result in primary)
gen_mult ()
{
    gen_pop ();
    gen_call ("ccmul");
}

// Divide the secondary register by
// primary.  Quotient in primary,
// remainder in secondary.
gen_div ()
{
    gen_pop ();
    gen_call ("ccdiv");
}

// Unsigned divide secondary register
// by the primary.  Quotient in primary,
// remainder in secondary.
gen_udiv ()
{
    gen_pop ();
    gen_call ("ccudiv");
}

// Remainder (mod) of secondary register
// divided by the primary.  Remainder in
// primary, quotient in secondary.
gen_mod ()
{
    gen_div ();
    gen_swap ();
}

// Remainder (mod) of secondary divided
// by primary register.   Remainder in
// primary, quotient in secondary.
gen_umod ()
{
    gen_udiv ();
    gen_swap ();
}

// Inclusive 'or' the primary and
// secondary registers.
gen_or ()
{
    gen_pop ();
    gen_call ("ccor");
}

// Exclusive 'or' the primary and
// secondary registers.
gen_xor ()
{
    gen_pop ();
    gen_call ("ccxor");
}

// 'and' primary and secondary.
gen_and ()
{
    gen_pop ();
    gen_call ("ccand");
}

// Arithmetic shift right the secondary // register the number of times in the
// primary.  Result in primary.
gen_arithm_shift_right ()
{
    gen_pop ();
    gen_call ("ccasr");
}

// Logically shift right the secondary
// register the number of times in the
// primary register.  Result in primary.
gen_logical_shift_right ()
{
    gen_pop ();
    gen_call ("cclsr");
}

// Arithmetic shift left secondary
// register the number of times in the
// primary register.  Result in primary.
gen_arithm_shift_left ()
{
    gen_pop ();
    gen_call ("ccasl");
}

// Two's complement of primary.
gen_twos_complement ()
{
    gen_call ("ccneg");
}

// Logical complement of primary.
gen_logical_negation ()
{
    gen_call ("cclneg");
}

// One's complement of primary.
gen_complement ()
{
    gen_call ("cccom");
}

// Convert primary value into logical
// value (0 if 0, 1 otherwise)
gen_convert_primary_reg_value_to_bool ()
{
    gen_call ("ccbool");
}

// Increment the primary register by 1
// if char, INTSIZE if int.
gen_increment_primary_reg (LVALUE * lval)
{
    switch (lval->ptr_type) {
    case STRUCT:
        gen_immediate2 ();
        output_number (lval->tagsym->size);
        newline ();
        gen_call ("add_de");
        break;
    case CINT:
    case UINT:
    default:
        gen_call ("inc_hl");
        break;
    }
}

// Decrement the primary register by one
// if char, INTSIZE if int.
gen_decrement_primary_reg (LVALUE * lval)
{
    gen_call ("dec_hl");
    switch (lval->ptr_type) {
    case CINT:
    case UINT:
        gen_call ("dec_hl");
        break;
    case STRUCT:
        gen_immediate2 ();
        output_number (lval->tagsym->size - 1);
        newline ();
        // two's complement 
        output_line ("mov   a,d");
        output_line ("cma");
        output_line ("mov   d,a");
        output_line ("mov   a,e");
        output_line ("cma");
        output_line ("mov  e,a");
        output_line ("inx  d");
        // subtract 
        gen_call ("add_de");
        break;
    default:
        break;
    }
}

/*
  Following are the conditional
  operators.  They compare the secondary
  against the primary and put a literal
  1 in the primary if the condition is
  true.  Otherwise they clear primary.
 */

gen_equal ()
{
    gen_pop ();
    gen_call ("cceq");
}

gen_not_equal ()
{
    gen_pop ();
    gen_call ("ccne");
}

gen_less_than ()
{
    gen_pop ();
    gen_call ("cclt");
}

gen_less_or_equal ()
{
    gen_pop ();
    gen_call ("ccle");
}

gen_greater_than ()
{
    gen_pop ();
    gen_call ("ccgt");
}

gen_greater_or_equal ()
{
    gen_pop ();
    gen_call ("ccge");
}

gen_unsigned_less_than ()
{
    gen_pop ();
    gen_call ("ccult");
}

gen_unsigned_less_or_equal ()
{
    gen_pop ();
    gen_call ("ccule");
}

gen_usigned_greater_than ()
{
    gen_pop ();
    gen_call ("ccugt");
}

gen_unsigned_greater_or_equal ()
{
    gen_pop ();
    gen_call ("ccuge");
}

char *
inclib ()
{
#ifdef  cpm
    return ("B:");
#endif
#ifdef  unix
#ifdef  INCDIR
    return (INCDIR);
#else
    return "";
#endif
#endif
}

// Squirrel away argument count in a
// register that modstk doesn't touch.
gnargs (d)
int d;
{
    output_with_tab (";#arg");
    output_number (d);
    newline ();
}

int
assemble (s)
char *s;
{
#ifdef  ASNM
    char buf[100];
    strcpy (buf, ASNM);
    strcat (buf, " ");
    strcat (buf, s);
    buf[strlen (buf) - 1] = 's';
    return (system (buf));
#else
    return (0);
#endif

}

int
link ()
{
#ifdef  LDNM
    fputs ("I don't know how to link files yet\n", stderr);
#else
    return (0);
#endif
}

// Print partial instruction to get an
// immediate value into the secondary.
gen_immediate2 ()
{
    output_with_tab ("ldei ");
}

// Add offset to primary.
add_offset (int val)
{
    gen_immediate2 ();
    output_number (val);
    newline ();
    gen_call ("add_de");
}

// Multiply primary by the length of
// some variable.
gen_multiply (int type, int size)
{
    switch (type) {
    case CINT:
    case UINT:
        gen_multiply_by_two ();
        break;
    case STRUCT:
        gen_immediate2 ();
        output_number (size);
        newline ();
        gen_call ("ccmul");
        break;
    default:
        break;
    }
}
