mod multicode_impl;

use proc_macro::TokenStream;

#[proc_macro_attribute]
pub fn multicode(attr: TokenStream, item: TokenStream) -> TokenStream {
    multicode_impl::multicode_impl(attr, item)
}
