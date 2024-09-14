(macro !? x
  ;"Like '?' but assigning results of conditions to '!'.
  (let rec '((x)
              (and x
                   (? (cdr x)
                      $((= ! ,(car x))
                        ,(cadr x)
                        ,@(rec (cddr x)))
                      (list (car x)))))
    $(let ! nil
       (? ,@(rec x)))))