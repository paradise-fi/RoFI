use std::str::FromStr;

use proc_macro2::{ TokenStream, TokenTree, Group, Delimiter };
use quote::{ ToTokens, TokenStreamExt };
use syn::{ bracketed, parse_macro_input, Ident, Token };
use syn::punctuated::Punctuated;
use syn::parse::{ Parse, ParseStream };
use syn::token;

macro_rules! outer_sep {
    () => { Token![;] };
}

struct Replacement {
    mod_name: Ident,
    _sep: Token![:],
    replacement: TokenStream,
}
impl std::fmt::Debug for Replacement {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let Self { mod_name, _sep, replacement } = self;
        f.debug_struct("Replacement")
            .field("mod_name", mod_name)
            .field("replacement", replacement)
            .finish()
    }
}
impl Parse for Replacement {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        Ok(Replacement {
            mod_name: input.parse()?,
            _sep: input.parse()?,
            replacement: {
                let mut repl = TokenStream::new();
                while !input.is_empty() && !input.peek(outer_sep!()) {
                    repl.append(input.parse::<TokenTree>()?);
                }
                repl
            },
        })
    }
}
impl Replacement {
    fn add_mod_with_repl(
        &self,
        out_stream: &mut TokenStream,
        repl_ident: &Ident,
        inner_stream: TokenStream
    ) {
        Ident::new("mod", self.mod_name.span()).to_tokens(out_stream);
        self.mod_name.to_tokens(out_stream);

        let pub_use_all = TokenStream::from_str("pub use super::*;").unwrap();
        let repl_stream = replace_ident(inner_stream, repl_ident, &self.replacement);
        Group::new(
            Delimiter::Brace,
            TokenStream::from_iter(pub_use_all.into_iter().chain(repl_stream))
        ).to_tokens(out_stream);
    }
}

fn replace_ident(input: TokenStream, repl_ident: &Ident, replacement: &TokenStream) -> TokenStream {
    input
        .into_iter()
        .map(|token| {
            match token {
                TokenTree::Ident(ident) => {
                    if ident.to_string() == repl_ident.to_string() {
                        replacement.clone()
                    } else {
                        TokenTree::Ident(ident).into_token_stream()
                    }
                }
                TokenTree::Group(group) => {
                    let new_stream = replace_ident(group.stream(), repl_ident, replacement);
                    let mut new_group = Group::new(group.delimiter(), new_stream);
                    new_group.set_span(group.span());
                    TokenTree::Group(new_group).into_token_stream()
                }
                token => token.into_token_stream(),
            }
        })
        .collect()
}
struct MulticodeArgs {
    ident: Ident,
    _sep: Token![,],
    _repl_bracket: token::Bracket,
    replacements: Punctuated<Replacement, outer_sep!()>,
}
impl std::fmt::Debug for MulticodeArgs {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let Self { ident, _sep, _repl_bracket, replacements } = self;
        f.debug_struct("MulticodeArgs")
            .field("ident", ident)
            .field("replacements", &replacements.iter().collect::<Vec<_>>())
            .finish()
    }
}
impl Parse for MulticodeArgs {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        let repl_content;
        Ok(MulticodeArgs {
            ident: input.parse()?,
            _sep: input.parse()?,
            _repl_bracket: bracketed!(repl_content in input),
            replacements: Punctuated::parse_terminated(&repl_content)?,
        })
    }
}

pub fn multicode_impl(
    attr: proc_macro::TokenStream,
    item: proc_macro::TokenStream
) -> proc_macro::TokenStream {
    let args = parse_macro_input!(attr as MulticodeArgs);
    let item = TokenStream::from(item);

    let mut result = TokenStream::new();
    for repl in &args.replacements {
        repl.add_mod_with_repl(&mut result, &args.ident, item.clone());
    }
    result.into()
}
