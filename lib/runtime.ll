; jupiter runtime header for version 0.0.10

declare void @ju_init ()                        ; void init ()
declare void @ju_destroy ()                     ; void destroy ()
declare void @juGC_root (i8**)                  ; void gc_root (juc* ptr)
declare void @juGC_unroot (i32)                 ; void gc_unroot (int ntimes)
declare void @juGC_store (i8**, i8*)            ; void gc_store (juc* ptr, juc val)
declare i8* @ju_make_buf (i32, i32, i32, ...)   ; juc  make_buf (int tag, int augment, int nmems, ...)
declare i8* @ju_make_str (i8*, i32)             ; juc  make_str (char* buf, int len)
declare i8* @ju_make_real (double)              ; juc  make_real (real n)
declare i8* @ju_get (i8*, i32)                  ; juc  get (juc cell, int idx)
declare i8* @ju_safe_get (i8*, i8*, i32, i32)   ; juc  safe_get (juc cell, char* tagname, int tag, int idx)
declare i32 @ju_get_tag (i8*)                   ; int  get_tag (juc cell)
declare i8* @ju_closure (i8*, i32, ...)         ; juc  closure (cb fn, int nvars, ...)
declare i8* @ju_get_fn (i8*)                    ; cb   get_fn (juc cell)

declare i8* @ju_make_box (i8*)                  ; juc  make_box (juc cell)
declare void @ju_put (i8*, i32, i8*)            ; void put (juc cell, int idx, juc val)
declare void @ju_safe_put (i8*, i8*, i32, i32, i8*) ; void safe_put (juc cell, char* tagname, int tag, int idx, juc val)
