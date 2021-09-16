# Visualization toolkit for the RoFI platform

The tool can visualize configurations, create animations and save pictures.

The input file contains representation of one or more configurations separated by an empty line.

You can choose whether to visualize model(s) of configuration(s) in 3D on the screen
or save picture(s) of configuration(s) using `-s` or `--save` option. You can also specify a path where to save the pictures using `-p` or `--path` option.

If there are more configurations in the input file, you can choose `-a` or `--animation` option which will generate smoother changes between the configurations. If you do not want to animate it, you have to use `-n` or `--many` to specify that there are more than one configuration in the file.

If the animation is chosen, you can specify:

* framerate of the animation using one of the `-f` or `--framerate` options
* angle velocity of modules using one of the `-v`, `--velocity`, `-g` or `--angle` options
* reconnection time / number of pictures using one of the `-t`, `--recTime`, `-e` or `--recPics` options

You can also specify size of the render window using `-r`, `--resolution` option in format <num>x<num> (e.g. 1920x1080). If the save option is used, you can specify magnification (`-m`, `--magnify`), which will enlarge the resolution of the final saved picture. The resolution of saved picture is resolution * magnification. E.g. window resolution = 1920x1080 and magnification = 2 -> result picture resolution = 3840x2160.

You can also specify settings for camera in a separate file.
The file can contain following lines:

* `CP x y z`: camera position or
* `CPM xs xe ys ye zs ze`: camera position move xstart -> xend, ystart -> yend, zstart -> zend
* `CF x y z`: camera focal point or
* `CFM xs xe ys ye zs ze`: camera focal point move xstart -> xend, ystart -> yend, zstart -> zend
* `CV x y z`: camera view up or
* `CVM xs xe ys ye zs ze`: camera view up move xstart -> xend, ystart -> yend, zstart -> zend

You can specify color definitions in a separate file too. 

The file can contain following lines:

* `S id r g b`: set rgb for single module (e.g. `S 1 255 255 255`)
* `M id1,id2,id3,id4 r g b`: set the same rgb for multiple modules (e.g. `M 1,17,8 255 255 255`)
* `R idFrom idTo r g b`: set the same rgb for modules with id in range [idFrom, idTo] (e.g. `R 0 10 255 255 255`)

Default values use mass center of the first configuration.

```
RoFI Visualizer: Tool for visualization of configurations and creating animations.
Usage:
  rofi-vis [OPTION...]

  -h, --help            Print help
  -i, --input arg       Input config file
  -s, --save            Save picture to file
  -a, --animation       Create animation from configurations
  -c, --camera arg      Camera settings file
  -p, --path arg        Path where to save pictures
  -f, --framerate arg   Number of pictures per second
  -v, --velocity arg    Maximal angular velocity in 1°/s
  -g, --angle arg       Maximal angle diff in ° per picture
  -t, --recTime arg     Time in seconds for reconnection
  -e, --recPics arg     Number of pictures for reconnection
  -n, --many            Many configurations in one file
  -r, --resolution arg  Size of the window on the screen or resolution of the
                        saved picture in format numberxnumber
  -m, --magnify arg     Magnification of saved pictures
  -q, --color arg       Colors definiton file
```

Examples:

```
./rofi-vis -i ../data/test.in
```

Opens a new window with a visualization of the configuration.

```
./rofi-vis -i ../data/res.out -s --path ../data/res -a
```

Generates some intersteps (animation) and saves pictures to the ../data/res directory.

```
./rofi-vis -i ../data/res.out -s --path ../data/res -a -c ../data/1.cam
```

Same as above with specified camera settings.

```
./rofi-vis -i ../data/res.out -s --path ../data/res -a -c ../data/1.cam --velocity 15 --recTime 2
```

## How to visualize the result of reconfiguration

```
./rofi-reconfig -i ../data/init.in -g ../data/goal.in > ../data/res.out
./rofi-vis -i ../data/res.out -n
```

Writes a sequence of configurations to a separate file, then draws many `-n` configurations from one file.

## How to create a video

Use script buildVideo.sh in visualizer directory.

```
./rofi-reconfig -i ../data/init.in -g ../data/goal.in > ../data/res.out
./rofi-vis -i ../data/res.out -a -s --path ../data/res -c ../data/1.cam
cd ../visualizer
./buildVideo.sh -i ../data/res -o ../data/animation/output.mp4
```

Writes sequence of configurations to a separate file, then saves pictures to ../data/res directory
and creates video output.mp4 in data/animation directory.

You can specify the directory with saved pictures, framerate (default = 24) of the animation,
path + filename of the output animation, whether to clean the input directory -
delete the pictures and whether to create animation with or without RoFI logo.


Options:

```
  -h, --help           Prints help
  -i, --input  arg     Directory with pictures to be animated
  -o, --output  arg    Output file (path/videoName.mp4)
  -f, --framerate arg  Number of pictures per second
  -d, --delete         Delete pictures in the input directory
  -l, --nologo         Video without RoFI logo
```

Examples:

Spicify output file and input directory:

```
./videoCreator.sh -o ../data/animation/output.mp4 -i ../data/res
```

Specify output file, input directory and framerate:

```
./videoCreator.sh -o ../data/animation/output.mp4 -i ../data/res -f 24
```

Specify output file, input directory and delete pictures in input directory

```
./videoCreator.sh -o ../data/animation/output.mp4 -i ../data/res -d
```

## How to create an animation in one step

Use script createAnimation.sh in visualizer directory.

Options:

```
  -h, --help            Prints help
  -i, --input arg       Input config file
  -o, --output arg      Output file (path/videoName.mp4)
  -c, --camera arg      Camera settings file
  -f, --framerate arg   Number of pictures per second
  -v, --velocity arg    Maximal angular velocity in 1°/s
  -g, --angle arg       Maximal angle diff in ° per picture
  -t, --recTime arg     Time in seconds for reconnection
  -e, --recPics arg     Number of pictures for reconnection
  -r, --resolution arg  Resolution of the animation
  -m, --magnify arg     Magnification of the resolution
  -l, --nologo          Video without RoFI logo
  -q, --color arg       Colors definition file
```

Examples:

Create a video out.mp4 from config file res.out:

```
./createAnimation.sh -i ../data/res.out -o ../data/animation/out.mp4
```

Specify also camera settings:

```
./createAnimation.sh -i ../data/res.out -o ../data/animation/out.mp4 -c ../data/1.cam
```