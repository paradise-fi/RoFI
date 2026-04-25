Flags for Configurations in Tools
=================================

- `-f VALUE` or `--format VALUE`
    Specifies the format of the configuration file.

    .. seealso::
        See :cpp:enum:`rofi::parsing::RofiWorldFormat`
        for all possible configuration formats
        (not all formats have to be supported).

- `--seq` or `--sequence`
    If specified, the configuration file will be treated as it contains
    a sequence of configurations.

    For Json formats this means a Json array of configurations.

- `-b` or `--by-one`
    .. note:: This flag is only applicable for voxel format.

    Specifies whether to fixate each module to the world separately
    instead of connecting the modules together.

    This can be useful when e.g. visualizing
    :cpp:class:`rofi::voxel::VoxelWorld` that is not connected.

    .. seealso:: The flag corresponds to the `fixateModulesByOne` parameter
        of :cpp:func:`rofi::voxel::VoxelWorld::toRofiWorld`.
