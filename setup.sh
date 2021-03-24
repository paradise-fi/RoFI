# Backup a value of a variable in order to restore it during teardown
backup() {
    varcontent="$(eval echo \"'$'$1\")"
    backupcontent="$(eval echo \"'$ORIGINAL_'$1\")"
    # Do not overwrite backups (e.g., when re-invoking setup)
    if [ ! -n "$backupcontent" ]
    then
        eval export ORIGINAL_$1=\""$varcontent"\"
        TEARDOWN_CMD="$TEARDOWN_CMD export $1=\"\$ORIGINAL_$1\"; unset ORIGINAL_$1;"
    fi
}

teardown() {
    echo "Tearing down the RoFI environment"
    eval $TEARDOWN_CMD

    unset ROFI_BUILD_CONFIGURATION
    unset ROFI_ROOT
    unset ROFI_BUILD_DIR
    unset TEARDOWN_CMD

    unset -f teardown
}

configurationDesc() {
    case "$1" in
        "Debug") echo D;;
        "Release") echo R;;
        "RelWithDebInfo") echo RD;;
        ?) echo U
    esac
}

print_help() {
    cat << EOF
Usage:
    source setup.sh [-f] <Release|Debug|RelWithDebInfo>
        Setup environment for given configuration. Release is the default one.
    ./setup.sh -h
        Print help

Options:
    -f  Alternate prompt to indicate the environment.
EOF
}


############## Main script ##############

## Check if sourced - has to be called outside function
# Took from https://stackoverflow.com/questions/2683279/how-to-detect-if-a-script-is-being-sourced
([[ -n $ZSH_EVAL_CONTEXT && $ZSH_EVAL_CONTEXT =~ :file$ ]] ||
[[ -n $KSH_VERSION && $(cd "$(dirname -- "$0")" &&
    printf '%s' "${PWD%/}/")$(basename -- "$0") != "${.sh.file}" ]] ||
[[ -n $BASH_VERSION ]] && (return 0 2>/dev/null)) && sourced=1

run() {
    local OPTIND flag
    while getopts "fh" flag; do
    case "$flag" in
        f) ALTER_PROMPT=1;;
        h)
            print_help
            return 0
            ;;
        ?)
            print_help
            return 1
            ;;
    esac
    done

    export PS1 # Bring shell variable into env, so we can back it up
    backup PS1
    backup PATH

    ROFI_BUILD_CONFIGURATION="Release"

    if [ ! "$sourced" ]; then
        echo "Invalid usage; do not invoke directly, use 'source [-f] <Release|Debug|RelWithDebInfo>' instead\n"
        print_help
        teardown
        exit 1
    fi

    ARG1=${@:$OPTIND:1}
    if [ "${ARG1}" ];
        then ROFI_BUILD_CONFIGURATION=${ARG1};
    fi


    echo "Setting up environment for $ROFI_BUILD_CONFIGURATION."
    echo "   To go back, invoke teardown"
    echo "   To change the configuration, simply invoke setup again"

    if [ ! -z "$ALTER_PROMPT" ]; then
        PS1="🤖 $(configurationDesc $ROFI_BUILD_CONFIGURATION) $ORIGINAL_PS1"
    fi

    export ROFI_BUILD_CONFIGURATION
    export ROFI_ROOT=$(pwd)
    export ROFI_BUILD_DIR="$(pwd)/build.${ROFI_BUILD_CONFIGURATION}"


    export PATH=$(realpath releng/tools):$ORIGINAL_PATH
    ## Add bin directories of the suites to path
    for suite in $(find suites -maxdepth 1 -type d -exec basename {} \;); do
        export PATH="${ROFI_BUILD_DIR}/$suite/bin:$PATH"
    done

    ## Register tab completion
    if [ -n "$ZSH_VERSION" ]; then
        autoload bashcompinit
        bashcompinit
    fi
    source releng/tools/_registerCompletion.sh

    ## Cleanup namespace
    unset -f print_help
    unset -f backup
    unset -f configurationDesc
}

run $@
