; Wrap expander in special form as it is recursive and needs
; to call itself with arguments evaluated.
(special quasiquote (qqx)
  (%qq qqx))

(fn %qeval (qqx)
  (eval qqx))

(fn %qq (qqx)
  (?
    ; End of list.
    (atom qqx)
      qqx

    ; Catch atoms.
    (atom (car qqx))
      (cons (car qqx) (%qq (cdr qqx)))

    ; Replace UNQUOTE expression by its evaluated argument.
    (eq (caar qqx) 'unquote)
      (cons (%qeval (cadar qqx)) (%qq (cdr qqx)))

    ; Insert evaluated argument of UNQUOTE-SPLICE into
    ; the list.
    (eq (caar qqx) 'unquote-spliced)
      (append (%qeval (cadar qqx)) (%qq (cdr qqx)))

    ; Just copy then...
    (cons (%qq (car qqx)) (%qq (cdr qqx)))))

(message "Testing QUASIQUOTE...")
(message "(Part 1)")
(or (equal $(1 2 ,3 ,4)
           '(1 2 3 4))
    (error $(1 2 ,3 ,4)))
(message "(Part 2)")
(or (equal $(1 2 ,@'(3 4))
           '(1 2 3 4))
    (error $(1 2 ,@'(3 4))))
(message "(Part 3)")
(or (equal $(((,1) ,@'(2)) ,3)
           '(((1) 2) 3))
    (error $(((,1) ,@'(2)) ,3)))
(message "(Part 4)")
(= x '(n v . body))
(or (equal $(fn ,x bla)
           '(fn (n v . body) bla))
    (error "Error unquoting dotted pair"))
