(out "TUNIX Lisp (nightly)")(terpri)
(print (gc))(out "B free.")(terpri)

(load "smoke-test.lisp")
(load "equality.lisp")
(load "test.lisp")
(load "list.lisp")
(load "quasiquote.lisp")
(load "macroexpand.lisp")
;(load "alist.lisp")
(load "prog.lisp")
;(load "stack.lisp")
;(load "let.lisp")
;(load "with.lisp")
;(load "whenunless.lisp")
;(load "while.lisp")
;(load "dolist.lisp")
;(load "set.lisp")

(print (gc))(out "B free.")(terpri)
