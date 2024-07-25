(macro with (inits . body)
  ;"Local symbol values."
  $((,(carlist inits)
     ,@body)
    ,@(carlist (cdrlist inits))))

(out "Testing WITH...")(terpri)
(or (equal (macroexpand '(with ((a 1)
                                (b 2))
                           (print a)
                           (print b)))
           '(((a b)
               (print a)
               (print b))
             1 2))
    (error))
