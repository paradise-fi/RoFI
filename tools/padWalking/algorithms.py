from enum import Enum
from directZigZag import walkDirect, walkZigZag, walkDirectZigZag
from shortestBacktrack import shortestBacktrack, shortestBacktrackLook
from strategies import DFSStrategy, BacktrackStrategy
from dfs import dfs, dfsWithBounds, dfsLookWithBounds, dfsLook

# can walk pad with holes
# strategy type
# function
# possible namings from cmd
class Algorithm(Enum):
    """
    Enum class representing possible algorithms used for walking the pad

    The value is a quaternion
    The first value says if the algorithm can walk the pad with holes (False if only rectangle is possible)
    The second value specifies type of possible strategies or None if strategies are not used
    The third value is the algorithm function
    The fourth value is a list of possible names of the algorithm used while running the whole program
    """
    DFS = (True, DFSStrategy, dfs, ["dfs"])
    DFS_WITH_BOUNDS = (False, DFSStrategy, dfsWithBounds, ["dfs-with-bounds", "dfs_with_bounds", "dfsb", "dfswithbounds", "dfsbounds"]) # pad without holes
    DIRECT = (False, None, walkDirect, ["direct", "d", "walkdirect"])
    ZIGZAG = (False, None, walkZigZag, ["zigzag", "z", "walkzigzag"])
    DIRECT_ZIGZAG = (False, None, walkDirectZigZag, ["direct-zigzag", "dz", "zd", "walkdirectzigzag", "directzigzag"])
    DFS_LOOK = (True, DFSStrategy, dfsLook, ["dfs-look", "dfsl", "dfslook", "dfs_look"]) # dfs with look around
    DFS_LOOK_WITH_BOUNDS = (False, DFSStrategy, dfsLookWithBounds, ["dfs-look-with-bounds", "dfslb", "dfsbl", "dfslookwithbounds", "dfs_look_with_bounds", "dfslookbounds"])
    SHORTEST_BACKTRACK_LOOK = (True, BacktrackStrategy, shortestBacktrackLook, ["shortets-backtrack-look", "sbl", "shortestbacktracklook"])
    SHORTEST_BACKTRACK = (True, BacktrackStrategy, shortestBacktrack, ["shortets-backtrack", "sb", "shortestbacktrack"]) 


def getAlgorithmByName(name):
    """
    Get the algorithm instance by its name

    :param name: name of the algorithm
    :return: algorithm instance
    """
    for algo in Algorithm:
        if (name.lower() in algo.value[3]):
            return algo
    return None