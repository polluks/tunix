(fn position (x l . f)
  (= f (? f (car f) eql))
  (do ((n 0 (++ n))
       (i l (cdr i)))
      ((not i))
    (? (f x (car i))
       (return n))))
