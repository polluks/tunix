devcbm_ops_directory:
    <devcon_error >devcon_error ; read
    <devcon_error >devcon_error ; write
    <devcon_error >devcon_error ; lookup

devcbm_make_root:
    lda #FILE_OPENED
    sta @(+ 3 vfile_states)
    lda #<devcbm_ops_directory
    sta @(+ 3 vfile_ops_l)
    lda #>devcbm_ops_directory
    sta @(+ 3 vfile_ops_h)
    lda #1
    sta @(+ 3 vfile_refcnts)
    sta vfile_root
    lda $ba     ; Get default device number.
    sta devcbm_device_numbers
    ldx #3
    lda #<path_root
    sta s
    lda #>path_root
    sta @(++ s)
    jsr devcbm_read_directory
    rts

path_root:  "$" 0

; X: vfile index
devcbm_read:
    jsr stop_task_switching

    lda vfile_handles,x
    tax
    lda devcbm_logical_file_numbers,x
    tax
    jsr chkin
    bcs +error
    jsr chrin
    bcs +error

    jmp start_task_switching

devcbm_write:
    jsr stop_task_switching

    pha
    lda vfile_handles,x
    tax
    lda devcbm_logical_file_numbers,x
    tax
    jsr chkout
    bcs +error
    pla
    jsr chrout
    bcs +error

    jmp start_task_switching

error:
    jmp set_cbm_error
