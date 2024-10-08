(load "prog1.lsp")
(load "!=.lsp")
(load "with.lsp")
(load "with-in.lsp")
(load "with-out.lsp")

(message "Testing OPEN on missing file...")
(and (open "k87sfdasdj9" 'r)
     (error "File \"k87sfdasdj9\" should be missing."))

(message "Testing OPEN file write to \"test.out\"...")
(with-out o (open "test.out" 'w)
  (setout o)
  (print message)
  (terpri))

(message "Testing OPEN file read from \"test.out\"...")
(with-in i (open "test.out" 'r)
  (!= (read)
    (or (equal ! message)
        (error "Expression read from \"test.out\" does not match MESSAGE: " !))))

(message "Testing READ/PRINT copy...")
(message "Copying...")
(with-in i (open "test.out" 'r)
  (with-out o (open "test2.out" 'w)
    (print (read))))
(message "Verifying...")
(with-in i (open "test2.out" 'r)
  (!= (read)
    (or (equal ! message)
        (error "Unexpected expression read from \"test2.out\": " !))))
