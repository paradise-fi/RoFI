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
            eval export ORIGINAL_$1=" "
            TEARDOWN_CMD="$TEARDOWN_CMD unset $1; unset ORIGINAL_$1;"
        fi
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

setGazeboVariables() {
    backup GAZEBO_NAME

    backup GAZEBO_MASTER_URI
    backup GAZEBO_MODEL_DATABASE_URI
    backup GAZEBO_RESOURCE_PATH
    backup GAZEBO_PLUGIN_PATH
    backup GAZEBO_MODEL_PATH

    if [ -z "$GAZEBO_NAME" ]; then
        GAZEBO_NAME="gazebo"
    fi

    local GAZEBO_INFO=$(whereis $GAZEBO_NAME | cut -d: -f2)
    # xargs trims whitespace
    # local GAZEBO_BIN=$(echo $INFO | xargs | cut -d' ' -f1)
    local GAZEBO_SHARE=$(echo $GAZEBO_INFO | xargs | cut -d' ' -f2)

    if [ -z "$GAZEBO_SHARE" ]; then
        return
    fi

    GAZEBO_MASTER_URI=$ORIGINAL_GAZEBO_MASTER_URI
    GAZEBO_MODEL_DATABASE_URI=$ORIGINAL_GAZEBO_MODEL_DATABASE_URI
    GAZEBO_RESOURCE_PATH=$ORIGINAL_GAZEBO_RESOURCE_PATH
    GAZEBO_PLUGIN_PATH=$ORIGINAL_GAZEBO_PLUGIN_PATH
    GAZEBO_MODEL_PATH=$ORIGINAL_GAZEBO_MODEL_PATH

    source $GAZEBO_SHARE/setup.sh
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
    local ALTER_PROMPT
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

    setGazeboVariables

    ROFI_BUILD_CONFIGURATION="Release"

    if [ -z "$sourced" ]; then
        echo "Invalid usage; do not invoke directly, use 'source [-f] <Release|Debug|RelWithDebInfo>' instead\n"
        print_help
        teardown
        exit 1
    fi

    ARG1=${@:$OPTIND:1}
    if [ -n "${ARG1}" ];
        then ROFI_BUILD_CONFIGURATION=${ARG1};
    fi


    echo "Setting up environment for $ROFI_BUILD_CONFIGURATION."
    echo "   To go back, invoke teardown"
    echo "   To change the configuration, simply invoke setup again"

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
    export GAZEBO_PLUGIN_PATH="$ROFI_BUILD_DIR/desktop/gazebosim/rofiModule:$GAZEBO_PLUGIN_PATH"
    export GAZEBO_PLUGIN_PATH="$ROFI_BUILD_DIR/desktop/gazebosim/roficom:$GAZEBO_PLUGIN_PATH"
    export GAZEBO_PLUGIN_PATH="$ROFI_BUILD_DIR/desktop/gazebosim/distributor:$GAZEBO_PLUGIN_PATH"
    export GAZEBO_RESOURCE_PATH="$ROFI_ROOT/data/gazebo:$GAZEBO_RESOURCE_PATH"
    export GAZEBO_RESOURCE_PATH="$ROFI_BUILD_DIR/data/gazebo:$GAZEBO_RESOURCE_PATH"


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
}

run $@

## Cleanup namespace
unset -f print_help
unset -f backup
unset -f configurationDesc
unset -f setGazeboVariables
unset -f run
