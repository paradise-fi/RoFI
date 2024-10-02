Voxel Serialization
===================

Voxel configuration has its own serialization format based on
`json <https://www.json.org/json-en.html>`_.
:cpp:class:`rofi::voxel::VoxelWorld` is implicitly convertible
to and from `nlohman::json <https://github.com/nlohmann/json>`_.

Json Grammar
------------

.. code-block:: json

    {
        "bodies" : [
            < array of voxels >
        ]
    }

where *voxel* is

.. code-block:: json

    {
        "body_dir": {
            "axis": < "X" | "Y" | "Z" >,
            "is_positive": < true | false >
        },
        "joint_pos": < 0 | 90 | -90 >,
        "pos": [ < array of 3 integers > ],
        "shoe_rotated": < true | false >
    }

Voxel world is valid if there exists a connected RoFI world
that is represented by the voxel world.
