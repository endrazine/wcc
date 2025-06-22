use std::ffi::{CStr, CString};
use std::os::raw::c_char;

#[no_mangle]
pub extern "C" fn rust_demangle(mangled: *const c_char) -> *mut c_char {
    if mangled.is_null() {
        return std::ptr::null_mut();
    }
    
    let c_str = unsafe { CStr::from_ptr(mangled) };
    let mangled_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    
    let demangled = rustc_demangle::demangle(mangled_str).to_string();
    
    match CString::new(demangled) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn rust_demangle_free(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe { CString::from_raw(ptr) };
    }
}
