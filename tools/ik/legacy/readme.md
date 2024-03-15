# A tool for running inverse kinematics

This tool utilizes the kinematic_rofibot class, allowing you to view the results of the implemented inverse kinematics algorithms. It takes a configuration and a target on input, and returns the result of trying to reach the target with the specified algorithm.

The program lets you either specify a fixed, single-arm configuration and supply a target to it, or a multiple-arm configuration, where you can choose to connect specified arms.

## How to use the tool

To run the program, go to the RoFI folder, build the desktop suite according to the documentation at [https://paradise-fi.github.io/RoFI/](https://paradise-fi.github.io/RoFI/) and compile rofi-ik.

Then, the program can be run with:

`rofi-ik -i inputFile [options]`

<b>Examples:</b>

`rofi-ik -i data/configurations/old/kinematics/on_top.rofi -f -r 3 3 3 0 0 0 -pi > data/configurations/old/kinematics/result.rofi`
This command takes a configuration of 5 modules on top of each other, where the bottom is considered fixed to the ground. Then it reaches for the position `(3, 3, 3)` with a rotation around the 3 axes of `(0, 0, 0)`, utilising the Jacobian Pseudoinverse algorithm. The resulting configuration is sent to a file, which can be viewed with our visualiser.

`rofi-ik -i data/configurations/old/kinematics/connect3.rofi -f -c 0 1 > data/configurations/old/kinematics/result.rofi`
This command takes a configuration with two arms and connects them, utilising the FABRIK algorithm.

## Options

Currently, the compile options are:
```
-i || --input fileName : Path to the initial configuration (necessary)
-r || --reach x y z rx ry rz : Reach for the target
-c || --connect a b : Connect arms a and b
-f || --fixed : Arm is fixed to the ground
-res || --results fileName : A file containing the results of specified operations
-t || --targets fileName : A file containing multiple commands for the arm
-v || --verbose : Additional information (for debugging purposes only)
-a || --animate : Saves intersteps of the algorithms, to let you visualise the procedure
And one of the three (defaults to FABRIK):
-ccd : use the Cyclic Coordinate Descent algorithm
-fabrik : use FABRIK
-pi : use Jacobian Pseudoinverse
--random n : generate and try to reach `n` random targets
```

## Expected input

Since it mostly served for testing, the tool expects fairly specific inputs. For fixed arms, the `-f` option is required, and the arm needs to start with the lowest ID at the bottom, then continue in an ascending fashion. See `data/configurations/old/kinematics/on_top.rofi`.

If we wish to connect arms, at least one module has to be connected to more than 2 other modules (we cannot connect a straight line right now). FABRIK expects the arms to be connected in the natural way: A shoes at the base and a B at the end of each (see `data/configurations/old/kinematics/connect3.rofi`). If we wanted to use the pseudoinverse, the second arm needs to be reversed.
