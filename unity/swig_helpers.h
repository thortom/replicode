// NB: in a .h because .i files seem to demand their own modules which is annoying
// %including via .h works fine though

////////////////////////////////////////////////////////////////////////////
// cs_callback is used to marshall callbacks. It allows a C# function to
// be passed to C++ as a function pointer through P/Invoke, which has the
// ability to make unmanaged-to-managed thunks. It does NOT allow you to
// pass C++ function pointers to C#.
//
// I would have liked to support FastDelegate<...> as the C++ argument
// type; this would have involved the cs_callback2 macro... but it turns
// out not to work under default project settings because .NET functions
// use the __stdcall calling convention, but FastDelegate uses the default
// convention which tends to be something else (__fastcall?). So nevermind.
//
// Anyway, to use this macro you need to declare the function pointer type
// TYPE in the appropriate header file (including the calling convention),
// declare a delegate named after CSTYPE in your C# project, and use this
// macro in your .i file. Here is an example:
//
// in C++ header file (%include this header in your .i file):
// typedef void (__stdcall *Callback)(PCWSTR);
// void Foo(Callback c);
//
// in C# code:
// public delegate void CppCallback([MarshalAs(UnmanagedType.LPWStr)] string message);
//
// in your .i file:
// %cs_callback(Callback, CppCallback)
//
// Remember to invoke %cs_callback before any code involving Callback
// DJM: INCLUDING TYPEDEFS OF Callback

%define %cs_callback(TYPE, CSTYPE)
    %typemap(ctype) TYPE, TYPE& "void*"
    %typemap(in) TYPE  %{ $1 = (TYPE)$input; %}
    %typemap(in) TYPE& %{ $1 = (TYPE*)&$input; %}
    %typemap(imtype, out="IntPtr") TYPE, TYPE& "CSTYPE"
    %typemap(cstype, out="IntPtr") TYPE, TYPE& "CSTYPE"
    %typemap(csin) TYPE, TYPE& "$csinput"
%enddef

%define %cs_callback2(TYPE, CTYPE, CSTYPE)
    %typemap(ctype) TYPE "CTYPE"
    %typemap(in) TYPE %{ $1 = (TYPE)$input; %}
    %typemap(imtype, out="IntPtr") TYPE "CSTYPE"
    %typemap(cstype, out="IntPtr") TYPE "CSTYPE"
    %typemap(csin) TYPE "$csinput"
%enddef
