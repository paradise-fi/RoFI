# Walking the pad for the RoFI platform

This tool can generate configurations of simple rofibots walking on a pad.

Run this tool using `python main.py [OPTION...]`or as an ordinary python script. 

The option `-h` shows this help:

```
Computes sequence of configurations for rofibot walking on a pad.
    Usage: 
      python main.py [OPTION...]

      -h, --help            Print this help
      -t, --type arg        Rofibots type 
      -p, --padFile arg     Path to file with defined pad
      -x, --x arg           Initial x coordinate of the rofibot
      -y, --y arg           Initial y coordinate of the rofibot
      -o, --ori arg         Initial orientation of the rofibot (N, E, S or W)
      -a, --algorithm arg   Algorithm used for walking the pad
      -s, --statistics arg  Function to output statistics
      -v, --statFile arg    Path to file to save statistics
      -c, --configs arg     Function to output configurations
      -u, --configsFile arg Paht to file to save configurations
      -g, --strategy arg    Strategy for dfs or shortest backtrack algorithm
      -f, --thinkFurther    Think one step further, prepare rotation

```


You must specify path to file with pad (`-p`), other options are optional. 



Supported types of rofibots are:

- `s` , `single` - rofibot with single modules (default)
- `d`, `double` - rofibot with two modules



Supported algorithms for rectangle pads only (without holes):

- `direct`, `d`, `walkdirect` - direct walking
- `zigzag`, `z`, `walkzigzag` - zigzag walking
- `direct-zigzag`, `dz`, `zd`, `walkdirectzigzag`, `directzigzag` - direct zigzag walking
- `dfs-with-bounds`,`dfs_with_bounds`, `dfsb`, `dfswithbounds`, `dfsbounds` - depth first search with rectangle bounds
- `dfs-look-with-bounds`, `dfslb`, `dfsbl`, `dfslookwithbounds`, `dfs_look_with_bounds`, `dfslookbounds` - depth first search with looking around and rectangle bounds

Supported algorithms for all pads:

- `dfs` - depth first search (default)
- `dfs-look`, `dfsl`, `dfslook`, `dfs_look` - depth first search with looking around
- `shortets-backtrack`, `sb`, `shortestbacktrack` - shortest backtrack
- `shortets-backtrack-look`, `sbl`, `shortestbacktracklook` - shortest backtrack with looking around

Supported strategies for all dfs algorithms:

- `strict`, `s` - strict N, E, S, W order (default)
- `ranodom`, `r` - random order
- `strict-best`, `sb`, `strict_best`, `strictbest` - strict + how the rofibot is rotated
- `random-best`, `rb`, `random_best`, `randombest` - random + how the rofibot is rotated
- `strict-early`, `se`, `strict_early`, `strictearly`  - strict + early end
- `ranodom-early`, `re`, `random_early`, `randomearly` - random + early end
- `strict-best-early`, `sbe`, `seb`, `strict_best_early`, `strictbestearly` -  strict + how the rofibot is rotated + early end
- `random-best-early`, `rbe`, `reb`, `random_best_early`, `randombestearly` - random + how the rofibot is rotated + early end

Supported strategies for all shortest backtrack algorithms:

- `scan`, `s` - scanning order (default)
- `random`, `r` - random order
- `random-best`, `rb` - random + how the rofibot is rotated



Supported statistics/configurations functions:

- `stdout`, `o`, `out` - prints statistics/configurations to standard output (default)
- `file`, `f` - writes statistics/coonfigurations to file (need also `-v`/`-u` specified)
- `nothing`, `n`, `none` - does nothing



Option `-t` takes value `y`, `yes`  (default) or `n` ,`no`. 



Examples:

```
python main.py -p ./maps/map.txt -x 1 -y 2 -t single -f y --algorithm dfsl -g strict -s none -c file -u ./configurations/config.in
```

```
python main.py -p ./maps/map.txt 
```



## Map generator

Tool for generating map files. 

Run this tool using `python mapGenerator.py x y filename [mapType]`

- `x`, `y` - dimensions of the map
- `filename` - path where to store the map
- `mapType` (optional) - type of map:
  - `r`, `rectangle` - rectangle pad (without holes, default)
  - `o`, `1`, `one` - single random hole
  - `s`, `some` - some random holes (from one hole to 1/4 of positions)
  - `m`, `many` - many random holes (from 1/4 of positions to n-1)



Example:


```
python mapGenerator.py 5 4 ./maps/map5x4.txt r
```