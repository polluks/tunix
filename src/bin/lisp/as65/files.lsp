(load 'as65/package.lsp)
(require 'when 'progn 'prog1 'do 'dolist
         'while 'awhile
         'with-queue 'enqueue
         'with-in 'with-out)

(fn pass (o addr pathnames first-pass?)
  ; "Single-pass assembly to stream O if not NIL."
  (= *as65-pc* addr)
  (dolist (pn pathnames)
    (awhen (progn
             (out " ")(print pn)
             (with-in i (open pn 'r)
               (with-queue toks
                 (enqueue toks (read))
                 (while .toks
                   (awhile (and (< (length toks) 3)
                                (read))
                     (enqueue toks !))
                   (!= (asm .toks first-pass?)
                     (when o
                       (out .!))
                     (= toks (. !. !.)))))))
      (error "Can't open " pn ": " !))))

(fn files (bin-path addr pathnames)
  ; "Assemble a set of files in multiple passes."

  ; First pass, building label database.
  (message '"First pass...")
  (= *as65-labels* nil)
  (pass nil addr pathnames t)

  ; Secondary passes, until code size settled.
  (message '"Secondary pass...")
  (let (old-end *as65-pc*)
    (while t
      (pass nil addr pathnames nil)
      (when (== old-end *as65-pc*)
        (return nil))
      (= old-end *as65-pc*)))

  ; Final pass, outputting the binary.
  (message "Assembling to " bin-path '...)
  (when (with-out o (open bin-path 'w)
          (pass o addr pathnames nil))
    (error "Can't write " bin-path)))

(in-package nil)
