(fn .. x x)

(fn list? (x)
  (or (not x)
      (cons? x)))

(fn caar (x)
  (car (car x)))

(fn cadr (x)
  (car (cdr x)))

(fn cdar (x)
  (cdr (car x)))

(fn cddr (x)
  (cdr (cdr x)))

(fn cadar (x)
  (car (cdr (car x))))

(fn caddr (x)
  (car (cdr (cdr x))))
