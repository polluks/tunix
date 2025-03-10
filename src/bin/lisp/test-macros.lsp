(message '"Testing macro-expansion of dotted pair...")
(or (equal (macroexpand '(v . body))
           '(v . body))
    (error (macroexpand '(v . body))))

(message '"Testing macro-expansion of unquoted list...")
(or (equal (macroexpand ',(a))
           ',(a))
    (error (macroexpand ',(a))))
(or (equal (macroexpand '(,(a)))
           '(,(a)))
    (error (macroexpand '(,(a)))))
(or (equal (macroexpand '((,(a))))
           '((,(a))))
    (error (macroexpand '((,(a))))))
(or (equal (.. (macroexpand '((,(a)))))
           '(((,(a)))))
    (error (.. (macroexpand '((,(a)))))))
(or (equal (.. 2 (macroexpand '((,(a)))))
           '(2 ((,(a)))))
    (error (.. 2 (macroexpand '((,(a)))))))

(message '"Testing %REQUOTE...")
(or (equal (%requote '(,a))
           '(,a))
    (error (%requote '(,a))))
(or (equal (%requote '(,(a)))
           '(,(a)))
    (error (%requote '(,(a)))))

(message '"Testing MACROEXPAND...")
(or (equal (macroexpand '$(a ,(b) ,@(c)))
           '$(a ,(b) ,@(c)))
    (error '"%requote went wrong"))
