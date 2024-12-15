(load 'compiler/package.lsp)
(require 'let 'prog1 'when 'awhen 'do 'while 'with-in 'with-out 'while)

(fn pass (pin pout f . reset?)
  (with-in i (open pin 'r)
    (with-out o (open pout 'w)
      (while (not (eof))
        (awhen (read)
          (setout stdout)
          (print !.)
          (setout o)
          (when reset?.
            (print 'reset)
            (reset!))
          (print (. !. (. .!. (@ f ..!))))
          (when reset?.
            (print 'reset)
            (reset!)))))))

(in-package nil)
