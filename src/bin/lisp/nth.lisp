(fn nth (n l)
  (? (or (not l)
         (== 0 n))
     (car l)
     (nth (-- n) (cdr l))))

(message "Testing NTH...")
(or (equal (nth 0 '(l i s p))
           'l)
    (error))
(or (equal (nth 2 '(l i s p))
           's)
    (error))
(and (nth 4 '(l i s p))
     (error))