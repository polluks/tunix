# Changelog

All notable changes to this project will be documented in
this file.

The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## [v0.0.18] - 2024-09-14

### Lisp

#### Interpreter

##### Fixed

- End-of-file check in REPL ignored LOAD channel, leading to
  heisenbugs.

##### Added

- ASSOC as built-in.

#### Macros

- \*MACROS\* is now an associative list, and macro functions
  are not defined any longer, so their names may clash with
  procedure definitions.

#### Environment

##### Changed

- File suffix for Lisp code is not ".lsp" instead of ".lisp".

##### Added

- IDE: Load editor, save image, and start it.


## [v0.0.17] - 2024-09-12

### Lisp

#### Interpreter

##### Added

- OUT: Output buffer for chars and char lists.
- Compile-time option MICROSCOPIC to strip off almost all
  optional features.
- Type check: failed object in debugger's program info.

##### Fixed

- TIME for C128, C64 and VIC-20.
- Simplified internal I/O channel handling that separates
  program and REPL channels.
- PRINT prints empty strings ("") AKA anonymous symbols.
- Garbage collector doesn't relocate symbol list links.

##### Changed

- Disabled images for the C16 and VIC-20.

#### Environment

##### Fixed

- AUTOLOAD recognizes functions passed by argument.
- Cleaned up to boot on the VIC-20 again.

##### Changed

- MACRO does not define functions any more.
- EDIT-LINE, split out from the edior.
- Queue functions are in separate file for AUTOLOAD.


## [v0.0.16] - 2024-09-06

AUTOLOAD and EDIT.

### Lisp

#### Debugger

##### Added

- ONERROR handler can delegate errors to the debugger to
  be handled as usual, by returning symbol %FAIL.

##### Changed

- Set breakpoints on any kind of object with a name, not
  only user-defined functions.

#### Environment

##### Added

- AUTOLOAD: Loads missing functions and macros, but not
  functions passed as arguments.
- EDIT: Simple text editor.
- COUNT-IF to count by predicate.
- TUNIX terminal control functions ("con.lisp").

##### Fixed

- WITH-GLOBAL: Return value of body.
- SOURCE

#### Interpreter

##### Fixed

- SYMBOL-VALUE of NIL.
- SYMBOL-NAME of NIL.

##### Changed

- No verbose LOAD, VAR and FN.  Would generate unwanted
  output during AUTOLOAD and the like.
- Most built-in functions are opt-out.

##### Added

- NCONC: native implementation to choose instead of the
  built-in version.
- READ-LINE: Reads line as a symbol.
- WITH-IN, WITH-OUT: Redirect channel for body.

### libsimpleio-cbm

- No "direct" mode when writing to non-standard I/O.


## [v0.0.15] - 2024-09-02

### libsimpleio

#### Added

- Terminal control codes to get the cursor position.
- putbackc() to put back another char than the last one.
  Used internally to return cursor positions so far.
- Direct mode with no scrolling and separate CR/LF.

### Interpreter

#### Changed

- TIME supports TARGET\_PET.
- SUBSEQ: Optional to be built-in.
- LENGTH: No not work on (symbol) names any more.
- OUT: Applies limit, set with OUTLIM, to printing object
  names as well.

#### Added

- Optionally built-in APPEND.
- SLENGTH returns the name length of a named object.
  (symbol, special form, built-in)
- Compile-time options NO\_APPEND, NO\_NCONC, NO\_SUBSEQ.


## [v0.0.14] - 2024-08-31

Optimizations to make the upcoming editor usable.  Also, a
simple terminal emulation has been added, which should do
for Unices and CBMs.  The latter via cc65's conio functions.

The C128 and Plus/4 are the only CBMs that work at the
moment.

### libsimpleio

#### Added

- Terminal control codes (see manual section)
 - clear screen
 - position cursor
 - clear/set flags (cursor visibility, reverse mode)

### TUNIX Lisp

#### Interpreter

##### Fixed

- Revived lost tests of APPEND.

##### Added

- NCONC: Destructively concatenates lists.
- OUTLIM limits number of printed character values.

##### Changed

- Now built in (for endurable performance):
 - NTHCDR
 - SUBSEQ
- LENGTH also returns the length of symbol names or names
  of built-ins.

#### Environment

##### Added

- CUT-AT: Destructively splits a list at position.


## [v0.0.13] - 2024-08-29

### Interpeter

#### Added

- Error on lost RETURN or GO.
- Function bodies treat RETURNs by passing them on instantly.

### Environment

- WITH-* macros do not use BLOCK any more.


## [v0.0.12] - 2024-08-28

### General

#### Added

- Manual section on how to add new targets.
- New targets (**untested**):
 - Apple II (TARGET\_APPLE2)
 - Apple II enhanced (TARGET\_APPLE2ENH)
- New targets (**not working**):
 - Atari XL (TARGET\_ATARIXL) - Need adjusted ld65 config.

### TUNIX Lisp

#### Interpreter

##### Changed

- CONIN returns NIL instead of 0 if there's no input,
  so the heap won't get filled with 0's.
- ERROR prints its arguments like OUT, prefixed by
  "ERROR: ".

#### Environment

##### Fixed

- Revived +V+.

##### Added

- +VB+ contains the Git branch name for use with conditional
  code in "user-*-image.lisp".


## [v0.0.11] - 2024-08-27

### TUNIX Lisp

#### Interpreter

##### Changed

- Compile-time option TEST makes all environment tests load.
- OUT: Take any number of arguments and traverse lists.

#### Environment

##### Added

- File 'user-pre-image.lisp' and 'user-post-image.lisp' are
  loaded before and after saving the default image.
  'user-post-image.lisp' is also loaded at image start.
- Macro != (file "alet.lisp").
- Macros !++ and !--: destructive versions of ++ and --.

##### Changed

- SUBSEQ can take negative positions.
- WITH-QUEUE handles only one queue and returns its list.


## [v0.0.10] - 2024-08-26

You'll want the stack checks, young Jedi!

### TUNIX Lisp

#### Interpreter

##### Added

- Do object and stack overflow checks at least once per
  expression evaluation.  Did that with every PUSH/POP in
  development versions.

##### Changed
- Reduced object stack consumption per evaluation (1
  object).

##### Fixed

- Compile-time option NO\_DEBUGGER also excludes code for
  highlighting.

#### Environment

##### Added

- COMPRESS-TREE finds and replaces duplicates subtrees
  across \*UNIVERSE\* symbol definitions.  (See manual.)


## [v0.0.9] - 2024-08-25

Lost pointer fix makes it worth this release.

### libdirectory

#### Changed

- Was 'libdirectory-list'.

#### Added

- Basic I/O functions to open, read and close the current
  directory.

### TUNIX Lisp

#### Interpreter

##### Fixed

- Lost pointer is evaluation of rest arguments.  Happened
  during GC stress test.

##### Added

- EXPERIMENTAL!: CBMs only: OPENDIR, READDIR and CLOSEDIR.

#### Debugger

##### Added

- Short command 'q' to exit the running program and return
  to the top-level REPL.

#### Environment

##### Changed

- PROGN does not use BLOCK.  That caught RETURNs
  unintentionally.

##### Added

- Macro AWHILE: Anaphoric equivalent to WHILE.
- CBMs only: LS to list the current directory.
- Macro WITH-PROF to time stop (aka "to profile")
  expressions.


## [v0.0.8] - 2024-08-24

User experience has been improved so dramatically that
delaying a release wouldn't be acceptable really.

### TUNIX Lisp

#### Environment

##### Changed

- Moved tests into own files.  No test is loaded in
  releases any more.  If compile-time option TEST was
  set, all tests are run as usual (plus a few internal
  interpreter tests at program start.
  Global variable +T?+ tells if TEST was set.
- Initial load time for saving the first image is down
  to less than a sixth of what it was before.
- Prerequisites aren't loaded on demand.  That can be
  made automatic.

#### Interpreter

- On 6502-CPU platforms, only high bytes of pointers are
  checked to tell if they are NIL or not.  Except for NIL
  there are never any objects on the zeropage.  NIL isn't
  either but it could be done to reduce code size a bit or
  two.


## [v0.0.7] - 2024-08-24

Mostly for fixing missing file accident and cleaned up
build scripts.

### Build

- Thorough clean-up of Makefiles.
- Complain if there are foureign files before doing a
  release.

### libsimpleio-cbm

- Print '\' instead of British Pound sign.

### TUNIX Lisp

#### Interpreter

- Increased object stack size to 1K for Comodore C128, C64
  and Plus/4.

#### Environment

##### Fixed

- Lisp environment files for POSITION and SPLIT were missing.


## [v0.0.6] - 2024-08-24

### TUNIX Lisp

#### Environment

##### Fixed

- Macro !? reimplemented and tests added.

##### Added

- Macro CASE: Evaluate conditionally by matching value.
- POSITION: Find position of object in list.
- SPLIT: Split list where object occurs, removing that object.

#### Debugger

##### Fixed

- GC/tag stack over-/underflow checks.
- Show faulty value on type error.
- Tell to which built-in arguments are missing.

##### Added

- New error code ERROR\_NO\_BLOCK\_NAME.

#### Interpreter

##### Fixed

- Global list start/last pointers weren't cleared on program
  start, which is bad with zeropage locations (6502-CPU).

##### Added

- Compile-time option GC\_DIAGNOSTICS to detect zeropage
  issues (undefined globals).


## [v0.0.5] - 2024-08-23

Most essential fixes.

### TUNIX Lisp

#### General

- Configure Comodore C128 to be able to load "all.lisp".
  (SLOW, COMPRESSED\_CONS).

#### Environment

##### Fixed

- APPEND with no arguments.

##### Added

- CDAR, MAPCAR, MAPAN

>>>>>>> development

## [v0.0.4] - 2024-08-22

Heavy I/O reworks make this release worth a new checkout.

### libsimpleio

#### Changed

- Last input/output and put back chars are stored for each
  channel separately.
- eof() and err() are tied to their channels.

#### Added

- conio(): Returns waiting char or 0.

### TUNIX Lisp

#### Interpreter

##### Changed

- READ accepts negative numbers.
- READ with end of file checks on the spot.
- REPL has its own set of channels apart from the program.
- Default image is saved after environment has loaded.

##### Added

- CONIO: Unbuffered input.
- READ accepts char notation '\<char>' as promised in the
  manual.
- Compile-time option VERBOSE\_READ to have expression read
  by the REPL printed.

#### Debugger

##### Added

- TARGET\_UNIX: Highlighting with terminal reverse mode instead
  of triple chevrons, '<<<' and '>>>'.  There are kept for new
  targets.

## [v0.0.3] - 2024-08-18

### libsimpleio

### Fixed

- Building with clang.  Courtesy of pulluks.  Thanks!

### libsimpleio-cbm

- Print ASCII underscore '\_' as PETSCII graphics.

### TUNIX Lisp

#### Interpreter

##### Fixed

- Topmost REPL is not exited when a child debugger REPL
  issues a break.

##### Changed

- LOAD returns NIL on error, T otherwise.
- MEMBER uses EQL as the predicate to match number values.
  Otherwise working with character value lists would become
  a rather hairy issue.
- AND, LAST, OR, and SYMBOL issue an error for dotted pairs.

##### Added

- EXPERIMENTAL: Issue regular error when out of heap, but
  return to the current REPL and do a garbage collection
  before calling an ONERROR handler or the debugger.


## [v0.0.2] - 2024-08-17

### TUNIX Lisp

#### Build system

##### Changed

- Revived full stress test.
- src/config is not required any more.
- Added TARGET=sim6502 (cc65's simulator).

##### Removed

- oscar64 submodule.  It's never been used.

#### Debugger

##### Changed

- Make it more to the user if an error has to be fixed of if
  one is stepping through.
- Block continuing unless an alternative for an erroraneous
  expression has been provided.
- Step to next expression if alternative value has been
  provided.
- Tell if program is continuing.
- Evaluate alternative expression with program's I/O
  channels.

#### Interpreter

##### Changed

- READ breaks immediately on errors.
- REPL handles READ errors.
- Expects end of dotted pair.
- cc65: Smaller initializing parts for more heap.
- COPY-LIST, REMOVE, and BUTLAST do not support dotted pairs
  any more.

#### Environment

##### Added

- +V+ contains the Git tag.  It's printed when loading the
  environment.
- AWHEN assigns result of condition to local !.

### libsimpleio

#### Fixed

- fresh\_line() only if not NUL, CR or LF before.


## [v0.0.1] - 2024-08-15

Changes since this file has been created.

### General

#### Removed

- D64 images as the have become too small for some targets.
  Let's spare the trouble.

#### Added

- Directories with binaries to replace D64 images.

### TUNIX Lisp

#### Debugger

##### Fixed

- Keep highlighting of current expression when evaluating
  argument of short command.
- Short ommand 'p' does not modify the return value.

##### Added

- Breakpoints (new short commands).
- Improved REPL return value handling.
- Additional error info (expression) like lists of missing
  arguments.

#### Interpreter

##### Fixed

- Fixed GC trigger.  Did not take end-of-heap marker into
  account.
- Detect if relocation table is full when switching to the
  next heap.
- Fixed OPEN's write mode without NDEBUG.
- READ: Do not put 0 back into buffer on end of file.

##### Changed

- Exits if out of heap instead of going vodka.
  This is temporary.
- BUTLAST, COPY-LIST, REMOVE: Function to handle all three
  has been rewritten.
- REMOVE can handle atoms and dotted pairs.
- VALUE became SYMBOL-VALUE.
- Compile-time option VERBOSE\_DEFINES not set by default.
- Compile-time error if VERBOSE\_COMPRESSED\_CONS without
  COMPRESSED\_CONS.

##### Added

- Check if argument names are symbols as well as
  ERROR\_ARGNAME\_TYPE.
- Compression of conses in user-triggered garbage
  collection.  (Please see manual for details.)
- Add PARANOID relocation table overflow check to GC sweep
  phase.
- Built-in ISAVE and ILOAD to save and load the heap.  Can
  be disables by compile-time option NO\_IMAGES.
- Internal error: Print address of faulty pointer.
- Faster checks of NOT on 8/16-bit platforms.
- SYMBOL-NAME and CHAR-AT return character value numbers of
  a symbol's name.
- Unix also: Built-in function "time" and constant "+bps+".

###### Compile-time options

- NO\_CHECK\_CPU\_STACK to not check CPU stack on overflow.
- VERBOSE\_COMPRESSED\_CONS for diagnostic printing a 'C'
  for each compressed cons.
- Compile-time option TEST enables all tests at program
  initialization.
- CHECK\_OBJ\_POINTERS at compile-time will enable quick
  sanity checks that is suitable for use on small machines.
  On TARGET\_UNIX it's thorough and slow, but easy to regret
  if not enabled during tests.

#### Environment

- Macro WITH-GLOBAL to temporarily change the value of a
  symbol.
- Unix: Environment file "unix.lisp".

### libsimpleio

#### Fixed

- File errors are reset.

#### Added

- simpleio\_open(): Check DOS status code.
- outhn(), outhb(), outhw(): Print hexadecimal nibble, byte
  or word.

### libsimpleio-cbm

- Fixed opening control channel #15.

### libsimpleio-stdlib

#### Fixed

- out(): Set err() if channel is invalid.
