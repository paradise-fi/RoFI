SCRIPT_DIR=`dirname "$(readlink -f "$0")"`

export GAZEBO_MODEL_PATH=`readlink -f $SCRIPT_DIR/models`:$GAZEBO_MODEL_PATH
export GAZEBO_PLUGIN_PATH=`readlink -f $SCRIPT_DIR/build/plugins/rofiModule`:$GAZEBO_PLUGIN_PATH
export GAZEBO_PLUGIN_PATH=`readlink -f $SCRIPT_DIR/build/plugins/roficom`:$GAZEBO_PLUGIN_PATH
export GAZEBO_PLUGIN_PATH=`readlink -f $SCRIPT_DIR/build/plugins/distributor`:$GAZEBO_PLUGIN_PATH

>&2 echo GAZEBO_MODEL_PATH = $GAZEBO_MODEL_PATH
>&2 echo GAZEBO_PLUGIN_PATH = $GAZEBO_PLUGIN_PATH
>&2 echo ""

$@
