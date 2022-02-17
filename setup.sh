# RoFI environment setup script

SILENT=

silentEcho() {
    if [ ! $SILENT ]; then
        echo $@
    fi
}

# Backup a value of a variable in order to restore it during teardown
backup() {
    local varcontent="$(eval echo \"'$'$1\")"
    local backupcontent="$(eval echo \"'$ORIGINAL_'$1\")"
    # Do not overwrite backups (e.g., when re-invoking setup)
    if [ -z "$backupcontent" ]
    then
        if [ -n "$varcontent" ]
        then
            eval export ORIGINAL_$1=\""$varcontent"\"
            TEARDOWN_CMD="$TEARDOWN_CMD export $1=\"\$ORIGINAL_$1\"; unset ORIGINAL_$1;"
        else
            # Set backup so it's not overwritten
            eval export ORIGINAL_$1=\"" "\"
            TEARDOWN_CMD="$TEARDOWN_CMD unset $1; unset ORIGINAL_$1;"
        fi
    fi
}

teardownImpl() {
    eval $TEARDOWN_CMD

    unset ROFI_BUILD_CONFIGURATION
    unset ROFI_ROOT
    unset ROFI_BUILD_DIR
    unset TEARDOWN_CMD

    unset -f teardown
    unset -f teardownImpl
}

teardown() {
    silentEcho "Tearing down the RoFI environment"
    teardownImpl
}

configurationDesc() {
    case "$1" in
        "Debug") echo D;;
        "Release") echo R;;
        "RelWithDebInfo") echo RD;;
        "MinSizeRel") echo MSR;;
        *) echo U
    esac
}

setGazeboVariables() {
    backup GAZEBO_NAME

    # backup GAZEBO_MASTER_URI
    # backup GAZEBO_MODEL_DATABASE_URI
    backup GAZEBO_RESOURCE_PATH
    backup GAZEBO_PLUGIN_PATH
    backup GAZEBO_MODEL_PATH

    if [ -z "$GAZEBO_NAME" ]; then
        GAZEBO_NAME="gazebo"
    fi

    local GAZEBO_BIN=$(command -v "$GAZEBO_NAME")
    local GAZEBO_SHARE=$(dirname "$(dirname "$GAZEBO_BIN")")/share/gazebo

    if [ ! -d "$GAZEBO_SHARE" ]; then
        return
    fi

    # GAZEBO_MASTER_URI=$ORIGINAL_GAZEBO_MASTER_URI
    # GAZEBO_MODEL_DATABASE_URI=$ORIGINAL_GAZEBO_MODEL_DATABASE_URI
    GAZEBO_RESOURCE_PATH=$ORIGINAL_GAZEBO_RESOURCE_PATH
    GAZEBO_PLUGIN_PATH=$ORIGINAL_GAZEBO_PLUGIN_PATH
    GAZEBO_MODEL_PATH=$ORIGINAL_GAZEBO_MODEL_PATH

    source $GAZEBO_SHARE/setup.sh
}

setupIdf() {
    mkdir -p build.deps
    export IDF_PATH=$ROFI_ROOT/build.deps/esp-idf
    export IDF_TOOLS_PATH=$ROFI_ROOT/build.deps/esp-tools
    export PYTHONPATH="${IDF_PATH}/components/partition_table:$PYTHONPATH"
    if [ ! -d $IDF_PATH ]; then
        git clone --depth 1 --branch v4.3.2 --recursive \
            https://github.com/espressif/esp-idf.git $IDF_PATH
        $IDF_PATH/install.sh
    fi
    source $IDF_PATH/export.sh
}

print_help() {
    cat << EOF
Usage:
    source setup.sh [-f] <Release|Debug|RelWithDebInfo|MinSizeRel>
        Setup environment for given configuration. Release is the default one.
    ./setup.sh -h
        Print help

Options:
    -f  Alternate prompt to indicate the environment.
    -s  Make the script silent (e.g., when running from CI)
    -i  Setup tools for rofi firmware development
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
    local ALTER_PROMPT
    local SETUP_IDF
    while getopts "fhsi" flag; do
        case "$flag" in
            f) ALTER_PROMPT=1;;
            h)
                print_help
                return 0
                ;;
            s)  SILENT=1;;
            i)  SETUP_IDF=1;;
            ?)
                return 1
                ;;
        esac
    done

    CWD=$(pwd)
    if [ ! -e ${CWD}/setup.sh ] || [ "$(head -n1 ${CWD}/setup.sh)" != "# RoFI environment setup script" ]; then
        >&2 echo "The script needs to be invoked from the root of the project."
        return 1
    fi

    ## Source user defines
    if [ -f ~/.rofi.pre.env ]; then
        echo "Applying extra setting from ~/.rofi.pre.env"
        source ~/.rofi.pre.env
    fi

    export PS1 # Bring shell variable into env, so we can back it up
    backup PS1
    backup PATH
    backup PYTHONPATH
    backup MAKEFLAGS
    backup CMAKE_GENERATOR

    export MAKEFLAGS="${MAKEFLAGS} --no-print-directory"

    # Use Ninja if it is available as it is faster
    if [ $(command -v ninja) ]; then
        export CMAKE_GENERATOR="Ninja"
    else
        export CMAKE_GENERATOR="Unix Makefiles"
    fi

    setGazeboVariables

    ROFI_BUILD_CONFIGURATION="Release"

    if [ -z "$sourced" ]; then
        >&2 echo "Invalid usage; do not invoke directly, use 'source [-f] <Release|Debug|RelWithDebInfo>' instead"
        >&2 echo
        return 1
    fi

    ARG1=${@:$OPTIND:1}
    if [ -n "${ARG1}" ];
        then ROFI_BUILD_CONFIGURATION=${ARG1};
    fi

    case "$ROFI_BUILD_CONFIGURATION" in
        Release|Debug|RelWithDebInfo|MinSizeRel);;
        *)
            >&2 echo "Unsupported build configuration $ROFI_BUILD_CONFIGURATION"
            return 1
            ;;
    esac

    silentEcho "Setting up environment for $ROFI_BUILD_CONFIGURATION."
    silentEcho "   To go back, invoke teardown"
    silentEcho "   To change the configuration, simply invoke setup again"

    if [ -n "$ALTER_PROMPT" ]
    then
        PS1="🤖 $(configurationDesc $ROFI_BUILD_CONFIGURATION) $ORIGINAL_PS1"
    else
        PS1="$ORIGINAL_PS1"
    fi

    export ROFI_BUILD_CONFIGURATION
    export ROFI_ROOT=$(pwd)
    export ROFI_BUILD_DIR="$(pwd)/build.${ROFI_BUILD_CONFIGURATION}"

    export GAZEBO_MODEL_PATH="$ROFI_ROOT/data/gazebo/models:$GAZEBO_MODEL_PATH"
    export GAZEBO_PLUGIN_PATH="$ROFI_BUILD_DIR/desktop/lib:$GAZEBO_PLUGIN_PATH"
    export GAZEBO_RESOURCE_PATH="$ROFI_ROOT/data/gazebo:$GAZEBO_RESOURCE_PATH"
    export GAZEBO_RESOURCE_PATH="$ROFI_BUILD_DIR/desktop/data/gazebo:$GAZEBO_RESOURCE_PATH"

    export PATH="$(realpath releng/tools):$ORIGINAL_PATH"
    ## Add bin directories of the suites to path
    for suite in $(find suites -maxdepth 1 -type d -exec basename {} \;); do
        export PATH="${ROFI_BUILD_DIR}/$suite/bin:$PATH"
        export PYTHONPATH="${ROFI_BUILD_DIR}/$suite/lib:$PYTHONPATH"
    done

    if [ -n "$SETUP_IDF" ]; then
        setupIdf
    fi

    ## Register tab completion
    if [ -n "$ZSH_VERSION" ]; then
        autoload bashcompinit
        bashcompinit
    fi
    source releng/tools/_registerCompletion.sh

    ## Source user defines
    if [ -f ~/.rofi.post.env ]; then
        echo "Applying extra setting from ~/.rofi.post.env"
        source ~/.rofi.post.env
    fi
}

run $@
if [ $? -ne 0 ]; then
    >&2 echo "No changes have been made"
    >&2 print_help
    teardownImpl
fi


## Cleanup namespace
unset -f print_help
unset -f backup
unset -f configurationDesc
unset -f setGazeboVariables
unset -f run
