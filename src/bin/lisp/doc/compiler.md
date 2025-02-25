TUNIX Lisp compiler
===================

***By Sven Michael Klose <pixel@hugbox.org>.***

The TUNIX Lisp interpreter is far too slow on 6502-based systems to do anything
useful with.  Compiling to stack-based bytecode functions removes the overhead
of interpretation and reduces the code size.

Essentially, this compiler is the smaller sister of the
[tré compiler](https://github.com/SvenMichaelKlose/tre/), which has the same
architecture.

# Architecture

The compiler has a _micropass_ architecture where each of the 21 passes is as
small and independent as possible, making the compiler easier to maintain.  Its
_frontend_ translates the input into a simpler, machine-level (but still
machine-independent) _intermediate representation (IR)_ for the _middleend_,
where every function is a flat list of statements, assignments and (conditional)
jumps.  That IR is then optimized by the middleend.  The _backend_ finally does
target-specific adjustments and generates the desired output: serializable
bytecode functions.

It illustrate the passes we have our compiler process some highly interesting
and nerdy function, printing a countdown:

~~~lisp
(fn countdown (n-total)
  (dotimes (n n-total)
    (print n)))
~~~

~~~
; Use:
(countdown 10)

; Output:
10 9 8 7 6 5 4 3 2 1
~~~

## Frontend

1. **Dot expansion**: Dot notation for CAR, CDR and SLOT-VALUE.
2. **Unquote expansion**: Top-level unquotes to generate code at compile-time.
3. **Macro expansion**: Standard macro expansion.
4. **Compiler macro expansion**: Breaks down AND, OR, ?, BLOCK, RETURN, GO,
   QUOTE, QUASIQUOTE, and so on, into IR.   After this pass there must be no
   more macros.
5. **Quote expansion**: Makes consing out of QUOTEs and QUASIQUOTEs.
6. **Lambda expansion**: Inlines binding lamdas and exports closures to own
   functions while building a function info tree required to proceed.

When compiling multiple files, all must have been processed up to the previous
pass, so all calls to known functions can be compiled in the next:

7. **Call expasions**: Arguments are expanded and rest arguments are made
   consing.
8. **Expression expansion**: To _single statement assignments_: All arguments
   of function calls are assigned to temporary variables.
9. **Block folding**: Collapses nested %BLOCKs.
10. **Assigment compaction**: Removes %= from assignments' heads.
11. **Tag compaction**: Replaces %TAGs by numbers.

## Middleend

12. **Jump optimization**: Remove chained jumps, unnecessary and unused tags,
    and unreachable code.
13. **Constant elimination**: Do calculations at compile time.
14. **Common code elimination**: Remove double calculations.
15. **Unused place elimination**
16. **Peephole optimization**: Made of child passes.
17. **Tailcall optimization**: Make jumps to start of function instead of
    having it call itself if possible.

## Back-end

Translate the IR to code.

18. **Wrapping tags**: Wrap tags (numbers) in %TAGs for code generation.
19. **Place expansion**: Variables are mapped to the lexical scope.
20. **Place assignment**: Stack frames and environment vectors are laid out.
21. **Code generation**: Macros generating bytecode

# The bytecode

Interpreting the bytecode must remove most of the overhead that is not doing the
program's actual business.  Most of that is calling functions, jumping, moving around
Lisp object pointers on the stack, and returning from functions.
Bytecode only comes along as _bytecode functions_, because stack space has to be
reserved before running any: the bytecode is _stack-based_ with no _registers_,
except the last return value of a function call.  Aside from stack places objects
in the function's object list can be referenced.

## Codes


* 1-251:   Reference: an offset into the object list or stack.
* 128-251: Copy to stack
* 252:     Jump if not NIL
* 253:     Jump if NIL
* 254:     Jump
* 255:     Return from function

References are used in function calls and assignments only.  Function calls are
lists of references.  The first reference is the destination where the return
value will go, followed by the function and its arguments.

~~~
<destination> <function> [<arg>*] ; * last one marked
~~~

The last argument is marked as such.  The destination can only reference a
stack place though to make the function's object list _immutable_.
Assignments are performed if the function reference is tagged as being the last
argument.

Reference offsets are even – object offset have their lowest bit set to tell them
apart from stack offsets:

* 0: (%s 0)
* 1: Offset 0/object 0
* 2: (%s 1)
* 3: Offset 2/object 1
* 4: (%s 2)
* 5: Offset 4/object 2

...and so on.  **Last arguments and assignment values have their highest bit set.**

## Spoiler: COUNTDOWN compiled to bytecode

~~~lisp
((a b)            ; Argument definition
 (== 0 print - 1) ; Object list
 nil              ; Stack frame size (args & locals)
 (2 128     ; ((%s 1) (%s 0))       ; Assignment
  0 3 2 129 ; (%0 (== (%s 1) 0))    ; Function call
  253 17    ; (%go-nnil 1)          ; Conditional jump
  0 5 130   ; (%0 (print (%s 1)))   ; Function call
  2 7 2 137 ; ((%s 1) (- (%s 1) 1)) ; Function call
  254 3)    ; (%go 3)               ; Unconditional jump
  255                               ; end of function
~~~

# The frontend passes

As mentioned before, the frontend is responsible for translating its input to a
simpler _intermediate IR_ and to gather information required for generating
fast code.

## Dot expansion

Converts abbreviations of CAR, CDR and SLOT-VALUE back to regular expressions.
See [TUNIX dot-expansion](manual.md#dot-expansion).

## Standard macro expansion

Expands all macros that are in the environemnt.  Required macros must be loaded
in advance using REQUIRE when compiling files, otherwise they'll be compiled as
function calls.

~~~
(fancymacro "Hi there!") ; Function doesn't exist error at run-time.
(require 'fancymacro)    ; Load FANCYMACRO at compile-time.
(fancymacro "Hi there!")
~~~

## Making IR: Compiler macro expansion

Turns special forms into ensembles of simpler IR code, using a separate,
machine-independent set of _compiler macros_.

### Control flow: BLOCK, GO, RETURN, ?, AND, OR

The special forms BLOCK, GO, RETURN, ?, AND, and OR are converted to simpler
ensembles of IR statements, also introducing %0 as a placeholder for the latest
return value.  Tags are wrapped up into %TAGs to tell them apart
from literals.

#### IR statements for flow control
| Metacode     | Description                           |
|--------------|---------------------------------------|
| (%go s)      | Unconditional jump.                   |
| (%go-nil s)  | Jump if last result is NIL.           |
| (%go-nnil s) | Jump if last result is not NIL.       |
| (%tag s)     | Jump destination.                     |

The new IR statements have no return values, nor do they modify %0, so there is
always a return value, even if a body ends on a %TAG.

~~~lisp
; Before:
(? (a)
   (b)
   (c)

; After:
(%= %0 (a))
(%go-nil 1)
(%= %0 (b))
(%go 2)
(%tag 1)
(%= %0 (c))
(%tag 2)
~~~

### BLOCK expansion

A RETURN makes its parent BLOCK, whose name is NIL, return immediately with the
RETURN's argument:

~~~lisp
(block nil
  (? (eql 'n (read))
     (return))  ; Return NIL from BLOCK of name NIL.
  (do-something))
~~~

With RETURN-FROM the BLOCK's name to return from can be specified, so BLOCKs can
be told apart when nested:

~~~lisp
(block outer-block
  (block nil    ; Cannot be reached.
    (block nil
      ...
      (return)  ; Must return from the closer BLOCK of name NIL.
      (return-from outer-block))))
~~~

BLOCKs are turned into anonymous IR %BLOCKs holding everything together.  They
will be gone with the _block folding_ pass, leaving only the jump instructions.

~~~lisp
(%block
  (%block
    (%block
      (%go nil)
      (%go outer-block)
      (%tag nil)))
  (%tag nil)
  (%tag outer-block))
~~~

~~~lisp
(%block
  (%block
    (%block
      (%go 2)
      (%go 0)
      (%tag 2)))
  (%tag 1)
  (%tag 0))
~~~

### COUNTDOWN after compiler macro expansion

~~~lisp
(lambda countdown (n-total)
  (%block
    (%= n total)
    (%tag 0)
    (%= %0 (== n 0)
    (%go-nnil t)
    (print n)
    (%= n (- n 1))
    (go 0)
    (%tag 1))))
~~~

## Quote expansion

QUOTEs are turned into CONSes:

~~~lisp
; Before:
'(a b c)

; After:
(. 'a (. 'b (. 'c nil)))
~~~

QUASIQUOTEs are translated to code using LIST and APPEND instead:

~~~lisp
; From:
$(1 2 ,@x ,y 4 5)

; To:
(append (.. 1 2) x (. y (.. 4 5)))
~~~


## Lambda expansion

Inlines binding lambdas and turns closures into regular functions with an
environment argument for lexical scope while also building a tree of FUNINFO
objects that describe each function.  FUNINFOs are looked up by function name,
so closures are baptized with uniquely generated names.  The FUNINFOs are
essential to process functions further.

~~~lisp
; TODO example of anonymous function first in expression.
~~~

~~~lisp
(lambda countdown (n-total)
  (%block
    (%= n total)
    (%tag 0)
    (%= %0 (== n 0)
    (%go-nnil t)
    (print n)
    (%= n (- n 1))
    (go 0)
    (%tag 1))))
~~~

## Call expansion

Checks and expands arguments and turns rest arguments into CONSes.  If
functions are unknown, calls of \*> are generated, which are expensive.
Re-compiling a file after a function became known is correcting that.

## Expression expansion

**The exit point of the front end.**  Breaks up nested function calls into a list
of single statement assignments to new temporary variables.

It does nothing for our COUNTDOWN example, so let's take a look at this nested
function call and what it's being transformed into:

~~~
(fun1 arg1 (fun2 (fun3) (fun4)) (fun5))

(%= %2 (fun3))
(%= %3 (fun4))
(%= %1 (fun2 2 3)
(%= %4 (fun5))
(%= %0(fun1 arg1 1 4)
~~~

## Block folding

Epression expansion is the last pass generating %BLOCKs to simplify expansions.
Now we're in need for pure lists of IR statements as bodies to work with them
in the middleend.

With _block folding_ nested %BLOCKs are collapsed and the remaining %BLOCKs are
dissolved into their function bodies – gone.

~~~lisp
(lambda countdown (n-total)
  (%= n total)
  (%tag 0)
  (%= %0 (== n 0))
  (%go-nnil 1)
  (%= %0 (print n))
  (%= n (- n 1))
  (%go 0)
  (%tag 1))
~~~

Now, if you'd want to optimize tags for example all you have to do is to scan a
pure list of statements for tags and jumps, no extra logic for nested structure
required.

## Assignment compaction

Symbol %= is not needed to tell function calls apart from jumps and tags, so
that is removed.

~~~lisp
(lambda countdown (n-total)
  (n total)
  (%tag 0)
  (%0 (== n 0)
  (%go-nnil 1)
  (%0 (print n))
  (n (- n 1))
  (%go 0)
  (%tag 1))))
~~~

Admittedly, it's easier to read with %= around.

## Tag compaction

%TAG statements are replaced by numbers, saving space and a bit of function
calling overhead in the following passes (which is probably not too notable).

~~~lisp
(lambda countdown (n-total)
  (n total)
  0             ; No %TAG any more.
  (%0 (== n 0))
  (%go-nnil t)
  (%0 (print n))
  (n (- n 1))
  (%go 0)
  1)            ; Same here.
~~~

The tré compiler brings back %TAGs for _code generation macros_ in its backend.

# The middleend passes

## Optimization

Basic compression of what the macro expansions messed up at least, like
removing assignments with no effect or chained jumps.

# The backend passes

## Place assignment

Wraps a variable in %STACK alongside the FUNINFO containing the variable.

## Place expansion

Replaces name/FUNINFO pairs in %STACKs by numerical stack indexes.

~~~lisp
(lambda countdown (n-total)
  ((%s 1) (%s 0))
  0
  (%0 (== (%s 1) 0))
  (%go-nnil 1)
  (%0 (print (%s 1)))
  ((%s 1) (- (%s 1) 1))
  (%go 0)
  1)
~~~

## Code generation

Once again: macro expansion to the rescue!  By merely expanding named LAMBDAs,
translating tags to offsets and making references to stack places and into a
functions list of literals, serializable bytecode is made - bytecode functions
must be PRINT- and READ-able:

~~~lisp
((a b)            ; Argument definition
 (== 0 print - 1) ; Object list
 nil              ; Stack frame size (args & locals)
 (2 0      ; ((%s 1) (%s 0))
  0 3 2 1  ; (%0 (== (%s 1) 0))
  253 17   ; (%go-nnil 1)
  0 5 2    ; (%0 (print (%s 1)))
  2 7 2 9  ; ((%s 1) (- (%s 1) 1))
  254 3)   ; (%go 3)
  255      ; end of function
~~~

Unlike our original COUNTDOWN made of 13 conses of 5 byte each totalling to
65 bytes, the bytecode function amounts to 18 bytecodes, plus an object table
of 5 pointers (10B) a stack frame size of 1B, totalling to 29 bytes.

A set of code generating macros could be used to generate strings of assembly
language.  But all we need to do now is to comb out literal objects which must be
referenced in the bytecode function's object table.
