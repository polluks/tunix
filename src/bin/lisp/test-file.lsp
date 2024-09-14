(message "Testing OPEN on missing file...")
(and (open "k87sfdasdj9" 'r)
     (error))

(message "Testing OPEN file write to \"test.out\"...")
(let c (open "test.out" 'w)
  (or c (message "Cannot open \"test.out\" for writing."))
  (setout c)
  (print message)
  (terpri)
  (setout stdout)
  (close c))

(message "Testing OPEN file read from \"test.out\"...")
(with ((c (open "test.out" 'r)
        e nil))
  (or c (message "Cannot open \"test.out\" for reading."))
  (setin c)
  (= e (read))
  (setin stdin)
  (close c)
  (or (equal e message)
      (error)))