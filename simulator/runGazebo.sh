export GAZEBO_MODEL_PATH=`readlink -f models`:$GAZEBO_MODEL_PATH
export GAZEBO_PLUGIN_PATH=`readlink -f build/plugins/universalModule`:$GAZEBO_PLUGIN_PATH

echo GAZEBO_MODEL_PATH = $GAZEBO_MODEL_PATH
echo GAZEBO_PLUGIN_PATH = $GAZEBO_PLUGIN_PATH

gazebo $@
