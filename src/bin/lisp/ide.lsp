(iload '"ide.img")
(load '"edit-line.lsp")
(load '"edit.lsp")
(fn istart ()
  (edit))
(message "Cleaning up, please wait...")
(= *universe* (cons 'edit (member 'autoload *universe*)))
(= *macros* nil)
(print (gc))(out " bytes free.")(terpri)
(message "Making image \"ide.img\"...")
(isave "ide.img")
(edit)
(exit)
