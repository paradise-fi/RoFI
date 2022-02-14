_external_completion() {
    while IFS= read -r line; do
        COMPREPLY+=( "$line" )
    done < <( ${ROFI_ROOT}/releng/tools/$1 $COMP_CWORD ${COMP_WORDS[@]} )
}

_rcfg_completion() {
    _external_completion _rcfg.completion.py
}

_rmake_completion() {
    _external_completion _rmake.completion.py
}

_img_completion() {
    _external_completion _img.completion.py
}


complete -F _rcfg_completion rcfg
complete -F _rmake_completion rmake
complete -F _img_completion rflash
complete -F _img_completion rmonitor
