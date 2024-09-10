(load "let.lisp")
(load "with.lisp")
(load "aif.lisp")
(load "!=.lisp")
(load "!++.lisp")
(load "!--.lisp")
(load "cbm-keycode.lisp")
(load "con-cbm.lisp")
(load "con.lisp")

(load "do.lisp")
(load "prog1.lisp")
(load "progn.lisp")
(load "while.lisp")
(load "awhile.lisp")
(load "when.lisp")
(load "awhen.lisp")
(load "unless.lisp")
(load "make-queue.lisp")
(load "enqueue.lisp")
(load "queue-list.lisp")
(load "mapcar.lisp")
(load "mapcan.lisp")
(load "nth.lisp")
(load "cut-at.lisp")
(load "case.lisp")
(load "with-global.lisp")
(load "with-in.lisp")
(load "with-out.lisp")
(load "with-queue.lisp")

;;; State

(var *line* nil)
(var *oline* nil)   ; Original line.
(var *mod?* nil)    ; Line modified?
(var *update?* nil) ; Line needs update in screen?
(var *lx* 0)    ; Line X position, relative to *OX*.

;;; Display

(var *spaces* nil)
(dotimes (i *con-w*)
  (push \  *spaces*))

(fn llen (x)
  (? (list? x)
     (length x)
     (slength x)))

(fn update-line (l)
  (when l
    (outlim *con-w*)
    (out l))
  (!? (nthcdr (llen l) *spaces*)
      (out !)))

;;; Line editing

(fn mkcharline ()
  (= *saved?* nil)
  (= *update?* t)
  (= *mod?* t)
  (when (symbol? *line*)
    (= *line* (symbol-name *line*))))

(fn del-char (x)
  (mkcharline)
  (= *line* (nconc (subseq *line* 0 x)
                   (subseq *line* (++ x)))))

(fn go-eol ()
  (let n (llen *line*)
    (= *lx* (? (> n 0) n 0))))

; Edit line.
; Return new line if an unrelated char has been input.
(fn edit-line (l)
  (con-crs t)
  (con-direct t)
  (= *mod?* nil)
  (= *line* (or l ""))
  (= *oline* *line*)
  (with ((x (con-x))
         (y (con-y)))
    ; Don't have cursor past line end.
    (and (> *lx* (llen *line*))
         (go-eol))
    (while (not (eof))
      (when *update?*
        (con-xy x y)
        (update-line *line*)
        (= *update?* nil))
      (con-xy (+ x *lx*) y)
      (with ((len  (llen *line*))
             (c    (while (not (eof))
                     (awhen (conin)
                       (return !)))))
        (case c
          +arr-left+
            (!-- *lx*)
          +arr-right+
            (!++ *lx*)
          +bs+
            (progn
              (when (== 0 *lx*)
                (putback)
                (con-direct nil)
                (return (? *mod?* (symbol *line*) *oline*)))
              (? (< 0 *lx*)
                 (del-char (!-- *lx*))))
          1 ; Ctrl-A
            (= *lx* 0)
          4 ; Ctrl-D
            (progn
              (= *mod?* t)
              (= *update?* t)
              (= *line* nil)
              (= *lx* 0))
          5 ; Ctrl-E
            (go-eol)
          (progn
            ; Put back unknown key and return line.
            (when (or (< c \ ) (> c 126))
              (putback)
              (con-direct nil)
              (return (? *mod?* (symbol *line*) *oline*)))
            ; Insert char and step right.
            (mkcharline)
            (= *line* (? (== 0 *lx*)
                         (nconc (list c) *line*)
                         (!= (cut-at *lx* *line*)
                           (nconc *line* (list c) !))))
            (!++ *lx*)))))))