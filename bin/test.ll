; compiled: Mon Apr 27 05:19:08 2015

; jupiter runtime header for version 0.0.1
declare i32 @ju_to_int (i8*)
declare i1  @ju_to_bool (i8*)
declare i8* @ju_from_int (i32)
declare i8* @ju_from_bool (i1)
declare i1  @ju_is_int (i8*)
declare i8* @ju_make_buf (i32, i32, i32, ...)
declare i8* @ju_make_str (i8*, i32)
declare i8* @ju_make_real (double)
declare i8* @ju_get (i8*, i32)


declare i8* @juStd_addInt (i8*, i8*)
declare i8* @juStd_negInt (i8*)
declare i8* @juStd_ltInt (i8*, i8*)
declare i8* @juStd_eqInt (i8*, i8*)
declare i8* @juStd_printString (i8*)
declare i8* @juStd_printInt (i8*)
declare i8* @juStd_printReal (i8*)
declare i8* @juStd_println ()
declare i8* @juStd_nil ()
declare i8* @juStd_hd (i8*)
declare i8* @juStd_tl (i8*)



;;;   #<entry> () -> ()
define i8* @fn_u0 () unnamed_addr
{
%.r = call i8* @fn_u1 ()
ret i8* %.r
}



;;;   main () -> ()
define i8* @fn_u1 () unnamed_addr
{
%.t0 = alloca i8*
%.t1 = alloca i8*
%.r = call i8* @fn_u2 (i8* inttoptr (i32 21 to i8*), i8* inttoptr (i32 7 to i8*))
store i8* %.r, i8** %.t0
%.r2 = call i8* @fn_u2 (i8* inttoptr (i32 15 to i8*), i8* inttoptr (i32 9 to i8*))
store i8* %.r2, i8** %.t1
%.r3 = call i8* @juStd_addInt (i8* %.r, i8* %.r2)
store i8* %.r3, i8** %.t0
%.r4 = call i8* @fn_u3 (i8* %.r3)
ret i8* %.r4
}



;;;   - (Int, Int) -> Int
define i8* @fn_u2 (i8* %x, i8* %y) unnamed_addr
{
%.t0 = alloca i8*
%.r = call i8* @juStd_negInt (i8* %y)
store i8* %.r, i8** %.t0
%.r2 = call i8* @juStd_addInt (i8* %x, i8* %.r)
ret i8* %.r2
}



;;;   println (Int) -> ()
define i8* @fn_u3 (i8* %x) unnamed_addr
{
%.r = call i8* @juStd_printInt (i8* %x)
%.r2 = call i8* @juStd_println ()
ret i8* %.r2
}

;;;   jupiter entry point -> main()
define ccc i32 @main (i32 %argc, i8** %argv)
{
call i8* @fn_u0 ()
ret i32 0
}

