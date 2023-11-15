# Reconfiguration test cases

Each reconfiguration test case contains files `init.in` and `goal.in` defining initial 
and goal configuration respectively. It may also contain outputs from reconfiguration
and visualization program such as reconfiguration paths, images, camera settings and videos.

Each folder name with a test case starts with a number of modules in configurations.

List and short description of test cases:

* `2-climb`: Two modules, where one is supposed to climb on the other.

![2-climb initial](2-climb/init.png?raw=true "2-climb initial")
![2-climb goal](2-climb/goal.png?raw=true "2-climb goal")

* `3-attach`: Starting from a tower of three modules one is supposed to get attached on 
the side of the middle one.

![3-attach initial](3-attach/init.png?raw=true "3-attach initial")
![3-attach goal](3-attach/goal.png?raw=true "3-attach goal")

* `8-parallel`: Eight modules, which form a twisted snake, should unfold by two rotations
 (light blue and pink modules in the center), which have to occur in parallel.

![8-parallel initial](8-parallel/init.png?raw=true "8-parallel initial")
![8-parallel goal](8-parallel/goal.png?raw=true "8-parallel goal")
