(macro dotimes (init . body)
  (with ((i (car init))
         (n (symbol)))
    $(let ,n ,(cadr init)
       (do ((,i 0 (+ ,i 1)))
           ((>= ,i ,n) ,@(cddr init))
         ,@body))))
