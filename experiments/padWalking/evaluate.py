import os
import sys 
sys.path.append("../../tools/padWalking")
from main import main
from actions import Action, printStatisticsToFile, computeTotalCost
import utils
from common import Pad
from enums import Ori
import csv
from actions import Action
import statistics





def addToActions(totalActions, newActions):
    for action, number in newActions.items():
        totalActions[action] += number

def getEmptyActions():
    actions = {}
    for action in Action:
        actions[action] = 0
    return actions

# {rotate_90 : [5,2,3], rotate_180 : [0,0,1], etc}
def getEmptyActionsCountList():
    actionsCounts = {}
    for action in Action:
        actionsCounts[action] = []
    return actionsCounts

def addToActionsCounts(actionsCounts, newActions):
    for action, number in newActions.items():
        actionsCounts[action].append(number)

def getAvgActions(totalActions, count):
    avgActions = getEmptyActions()
    for action, number in totalActions.items():
        avgActions[action] = number/count
    return avgActions

def getAvgActionsFromCounts(actionsCounts, count):
    avgActions = getEmptyActions()
    for action, listCount in actionsCounts.items():
        avgActions[action] = sum(listCount) / count
    return avgActions

def getNeighbours(pad, a, b):
    neighbours = []
    if (b < pad.y - 1):
        if (pad.map[a][b+1]):
            neighbours.append(Ori.N)
    if (a < pad.x - 1):
        if (pad.map[a + 1][b]):
            neighbours.append(Ori.E)
    if (b > 0):
        if (pad.map[a][b-1]):
            neighbours.append(Ori.S)
    if (a > 0):
        if (pad.map[a - 1][b]):
            neighbours.append(Ori.W)
    return neighbours

def getAllPossibleInits(pad, rofibot):
    inits = []
    for a in range(pad.x):
        for b in range(pad.y):
            if (pad.map[a][b]):
                neighbours = getNeighbours(pad, a, b)
                if (len(neighbours) > 0):
                    oris = [Ori.N, Ori.E, Ori.S, Ori.W]
                    if (rofibot == "single" and len(neighbours) == 1):
                        oris.remove(utils.getRightOri(neighbours[0]))
                    for ori in oris:
                        inits.append((a, b, ori.value))
    return inits

def getAllPossibleInitsForDouble(padFilename):
    pad = Pad(padFilename)
    return getAllPossibleInits(pad, "double")

def getAllPossibleInitsForSingle(padFilename):
    pad = Pad(padFilename)
    return getAllPossibleInits(pad, "single")

def getAllAlgorithms():
    algos = []
    for algo in ["direct", "zigZag", "directZigZag"]:
        algos.append((algo, None))
    for algo in ["dfs", "dfsBounds", "dfsLook", "dfsLookBounds"]:
        for strategy in ["s", "se", "sb", "sbe", "r", "re", "rb", "rbe"]:
            algos.append((algo, strategy))
    for algo in ["shortestBacktrack", "shortestBacktrackLook"]:
        for strategy in ["s", "r", "rb"]:
            algos.append((algo, strategy))
    return algos

def getHoleAlgorithms():
    algos = []
    for algo in ["dfs", "dfsLook"]:
        for strategy in ["s", "se", "sb", "sbe", "r", "re", "rb", "rbe"]:
            algos.append((algo, strategy))
    for algo in ["shortestBacktrack", "shortestBacktrackLook"]:
        for strategy in ["s", "r", "rb"]:
            algos.append((algo, strategy))
    return algos

def getAlgoName(algo, strategy):
    if (strategy is None):
        return algo
    else:
        return algo + "_" + strategy

def getAlgoNames(algoList):
    names = []
    for item in algoList:
        names.append(getAlgoName(item[0], item[1]))
    return names

def createDirectioriesForRectangle():
    """
    Prepares files structure for rectangle algorithms
    """
    pathToMap = os.path.join(".", "testData", "maps", "rectangle")
    #pathToConfigs = os.path.join(".", "testData", "configurations", "rectangle")
    pathToEvals = os.path.join(".", "testData", "evaluations", "rectangle")
    
    files = os.listdir(pathToMap)

    for f in files:
        print(f)
        if (f == "init"):
            continue
        fname = f.split(".")[0]
        #print(os.path.join(pathToConfigs,fname))
        algos = getAlgoNames(getAllAlgorithms())
        for algo in algos:
            for rofi in ["single_y", "single_n", "double"]:
                #print(os.path.join(pathToConfigs, fname, algo, rofi))
                #os.makedirs(os.path.join(pathToConfigs, fname, algo, rofi))
                os.makedirs(os.path.join(pathToEvals, fname, algo, rofi))

#createDirectioriesForRectangle()

def createDirectioriesForHoles():
    """
    Prepare files structure for holes algorithms
    """

    for padType in ["singleHole", "someHoles", "manyHoles"]:
        pathToMap = os.path.join(".", "testData", "maps", padType)
        #pathToConfigs = os.path.join(".", "testData", "configurations", padType)
        pathToEvals = os.path.join(".", "testData", "evaluations", padType)

        files = os.listdir(pathToMap)
        for f in files:
            print(f)
            if (f == "init"):
                continue
            fname = f.split(".")[0]
            #print(os.path.join(pathToConfigs,fname))
            algos = getAlgoNames(getHoleAlgorithms())
            for algo in algos:
                for rofi in ["single_y", "single_n", "double"]:
                    #print(os.path.join(pathToConfigs, fname, algo, rofi))
                    #os.makedirs(os.path.join(pathToConfigs, fname, algo, rofi))
                    os.makedirs(os.path.join(pathToEvals, fname, algo, rofi))

#createDirectioriesForHoles()

def createDirectories():
    createDirectioriesForHoles()
    createDirectioriesForRectangle()

#createDirectories()



def printAvgStatistics(mapName, mapType, avgStatisticsForMap):
    """
    Print rounded average statistics to file and to stdout
    """
    
    print("----------------------------------")
    rows = []
    rows.append(["", "single_y"] + [""] * 17 + ["single_n"] + [""] * 17 + ["double"] + [""] * 17) 
    rows.append([""] + (["AB90fr", "AB90fx", "AB180fr", "AB180fx", "G90", "G180", "addE", "remE", "cost"] + ["NCAB90fr", "NCAB90fx", "NCAB180fr", "NCAB180fx", "NCG90", "NCG180", "NCaddE", "NCremE", "NCcost"]) * 3)
    for algoName, rofiDict in avgStatisticsForMap.items():
        print(algoName, end=" ")
        algoRow = [algoName]
        for rofiname in ["single_y", "single_n", "double"]:
            rofiActions = rofiDict[rofiname]
            for action in [Action.ROTATE_FREE_A_B_90, Action.ROTATE_FIXED_A_B_90, Action.ROTATE_FREE_A_B_180, Action.ROTATE_FIXED_A_B_180, Action.ROTATE_GAMMA_90, Action.ROTATE_GAMMA_180, Action.ADD_EDGE, Action.REMOVE_EDGE]:
                countRounded = round(rofiActions["all"][action])
                algoRow.append(countRounded)
                print(countRounded, end = " ")
            cost = computeTotalCost(rofiActions)
            algoRow.append(cost)
            print("cost", end = "")
            print("|", end = "")
            for action in [Action.ROTATE_FREE_A_B_90, Action.ROTATE_FIXED_A_B_90, Action.ROTATE_FREE_A_B_180, Action.ROTATE_FIXED_A_B_180, Action.ROTATE_GAMMA_90, Action.ROTATE_GAMMA_180, Action.ADD_EDGE, Action.REMOVE_EDGE]:
                countRounded = round(rofiActions["nonConcurent"][action])
                algoRow.append(countRounded)
                print(countRounded, end = " ")
            algoRow.append("NCcost")
            print("NCcost", end = "")
            print("|", end = "")
        rows.append(algoRow)
        print("")
    
    with open(os.path.join("testData", "evaluations", mapType, mapName + ".csv"), 'w', encoding='UTF8') as f:
        writer = csv.writer(f)
        # write the data
        writer.writerows(rows)  


def writeCSVAvgAndSdStatistics(mapName, mapType, name, statisticsForMap, costsForMap):
    """
    Print average and standard deviation statistics to file and to stdout
    """

    rows = []
    if (costsForMap is not None):
        rows.append(["", "single_y"] + [""] * 17 + ["single_n"] + [""] * 17 + ["double"] + [""] * 17) 
        rows.append([""] + (["AB90fr", "", "AB90fx", "", "AB180fr", "", "AB180fx", "", "G90", "", "G180", "", "addE", "", "remE", "", "cost", ""]) * 3)
    else:
        rows.append(["", "single_y"] + [""] * 15 + ["single_n"] + [""] * 15 + ["double"] + [""] * 15) 
        rows.append([""] + (["AB90fr", "", "AB90fx", "", "AB180fr", "", "AB180fx", "", "G90", "", "G180", "", "addE", "", "remE", "",]) * 3)
    for algoName, rofiDict in statisticsForMap.items():
        algoRow = [algoName]
        for rofiname in ["single_y", "single_n", "double"]:
            rofiActionsCounts = rofiDict[rofiname]
            for action in [Action.ROTATE_FREE_A_B_90, Action.ROTATE_FIXED_A_B_90, Action.ROTATE_FREE_A_B_180, Action.ROTATE_FIXED_A_B_180, Action.ROTATE_GAMMA_90, Action.ROTATE_GAMMA_180, Action.ADD_EDGE, Action.REMOVE_EDGE]:
                avg = statistics.mean(rofiActionsCounts[action])
                sd = statistics.stdev(rofiActionsCounts[action])
                algoRow.append(avg)
                algoRow.append(sd)
            if (costsForMap is not None):
                algoRow.append(statistics.mean(costsForMap[algoName][rofiname]))
                algoRow.append(statistics.stdev(costsForMap[algoName][rofiname]))
        rows.append(algoRow)

    with open(os.path.join("testData", "evaluations", mapType, mapName + "_" + name + ".csv"), 'w', encoding='UTF8') as f:
        writer = csv.writer(f)
        # write the data
        writer.writerows(rows)  

def printAvgAndSdStatistics(mapName, mapType, statisticsForMap):
    """
    Print average and standard deviation statistics to file and to stdout
    """

    print("----------------------------------")
    rows = []
    rows.append(["", "single_y"] + [""] * 35 + ["single_n"] + [""] * 35 + ["double"] + [""] * 35) 
    rows.append([""] + (["AB90fr", "", "AB90fx", "", "AB180fr", "", "AB180fx", "", "G90", "", "G180", "", "addE", "", "remE", "", "cost", ""] + ["NCAB90fr", "", "NCAB90fx", "", "NCAB180fr", "", "NCAB180fx", "", "NCG90", "", "NCG180", "", "NCaddE", "", "NCremE", "", "NCcost", "",]) * 3)
    for algoName, rofiDict in statisticsForMap.items():
        print(algoName, end=" ")
        algoRow = [algoName]
        for rofiname in ["single_y", "single_n", "double"]:
            rofiActionsCounts = rofiDict[rofiname]
            for action in [Action.ROTATE_FREE_A_B_90, Action.ROTATE_FIXED_A_B_90, Action.ROTATE_FREE_A_B_180, Action.ROTATE_FIXED_A_B_180, Action.ROTATE_GAMMA_90, Action.ROTATE_GAMMA_180, Action.ADD_EDGE, Action.REMOVE_EDGE]:
                avg = statistics.mean(rofiActionsCounts["all"][action])
                sd = statistics.stdev(rofiActionsCounts["all"][action])
                algoRow.append(avg)
                algoRow.append(sd)
                print(avg, sd, end=" ")
            algoRow.append("cost")
            algoRow.append("cost sd")
            print("cost", "cost_sd", end = "")
            print("|", end = "")
            for action in [Action.ROTATE_FREE_A_B_90, Action.ROTATE_FIXED_A_B_90, Action.ROTATE_FREE_A_B_180, Action.ROTATE_FIXED_A_B_180, Action.ROTATE_GAMMA_90, Action.ROTATE_GAMMA_180, Action.ADD_EDGE, Action.REMOVE_EDGE]:
                avg = statistics.mean(rofiActionsCounts["nonConcurent"][action])
                sd = statistics.stdev(rofiActionsCounts["nonConcurent"][action])
                algoRow.append(avg)
                algoRow.append(sd)
                print(avg, sd, end=" ")
            algoRow.append("NCcost")
            algoRow.append("NCcost sd")
            print("NCcost", "NCcost_sd", end = "")
            print("|", end = "")
        rows.append(algoRow)
        print("")
    
    with open(os.path.join("testData", "evaluations", mapType, mapName + "_sd.csv"), 'w', encoding='UTF8') as f:
        writer = csv.writer(f)
        # write the data
        writer.writerows(rows)  




def generateAllData():
    """
    Generate all evaluation data for all pads

    Does not write the result configurations, only the statistics of actions
    """

    for mapType in ["rectangle", "singleHole", "someHoles", "manyHoles"]:
    #for mapType in ["rectangle"]:
        pathToMap = os.path.join(".", "testData", "maps", mapType)
        pathToConfigs = os.path.join(".", "testData", "configurations", mapType)
        pathToEvals = os.path.join(".", "testData", "evaluations", mapType)
        
        #mapFiles = os.listdir(pathToMap)[:1] #only single map per type
        mapFiles = os.listdir(pathToMap)

        # for each map
        for mapFilename in mapFiles:
            print(mapFilename)
            if (mapFilename == "init"):
                continue
            mapName = mapFilename.split(".")[0]
            #avgStatisticsForMap = {}
            statisticsForMap = {}
            nonConcurentStatisticsForMap = {}
            costsForMap = {}

            if (mapType == "rectangle"):
                algos = getAllAlgorithms()
            else:
                algos = getHoleAlgorithms()
            for algo in algos:
                algoName = getAlgoName(algo[0], algo[1])
                #avgStatisticsForMap[algoName] = {}
                statisticsForMap[algoName] = {}
                nonConcurentStatisticsForMap[algoName] = {}
                costsForMap[algoName] = {}
                print(algoName)

                for rofi in ["single", "double"]:
                    for think in ["y", "n"]:
                        count = 0
                        totalActions = getEmptyActions()
                        totalNonConcurentActions = getEmptyActions()
                        allActionsCounts = getEmptyActionsCountList()
                        allNonConcurentActionsCounts = getEmptyActionsCountList()
                        if (rofi == "single"):
                            inits = getAllPossibleInitsForSingle(os.path.join(pathToMap, mapFilename))
                            rofiname = rofi + "_" + think
                        else:
                            if (think == "n"):
                                continue
                            inits = getAllPossibleInitsForDouble(os.path.join(pathToMap, mapFilename))
                            rofiname = rofi
                        #avgStatisticsForMap[algoName][rofiname] = {}
                        statisticsForMap[algoName][rofiname] = {}
                        nonConcurentStatisticsForMap[algoName][rofiname] = {}
                        costsForMap[algoName][rofiname] = []

                        for init in inits:
                            x = str(init[0])
                            y = str(init[1])
                            ori = init[2]
                            maxCount = 1
                            random = False
                            if (algo[1] is not None and 'r' in algo[1]):
                                random = True
                                maxCount = 10
                            for i in range(maxCount):
                                filename = "-x_" + x + "-y_" + y + "-ori_" + ori
                                if (random):
                                    filename += "_" + str(i + 1)

                                configsFilename = os.path.join(pathToConfigs, mapName, algoName, rofiname, filename + ".in")
                                evalsFilename = os.path.join(pathToEvals, mapName, algoName, rofiname, filename + ".txt")
                                """
                                if (algo[1] is None):
                                    configs, actions, nonConcurentActions = main(["-p", os.path.join(pathToMap, mapFilename), "-x", x, "-y", y, "-o", ori, "-f", think, "-t", rofi, "-a", algo[0], "-s", "file", "-v", evalsFilename, "-c", "nothing"])
                                else:
                                    configs, actions, nonConcurentActions = main(["-p", os.path.join(pathToMap, mapFilename), "-x", x, "-y", y, "-o", ori, "-f", think, "-t", rofi, "-a", algo[0], "-g", algo[1], "-s", "file", "-v", evalsFilename, "-c", "nothing"])
                                """
                                if (algo[1] is None):
                                    configs, actions, nonConcurentActions = main(["-p", os.path.join(pathToMap, mapFilename), "-x", x, "-y", y, "-o", ori, "-f", think, "-t", rofi, "-a", algo[0], "-s", "n", "-c", "nothing"])
                                else:
                                    configs, actions, nonConcurentActions = main(["-p", os.path.join(pathToMap, mapFilename), "-x", x, "-y", y, "-o", ori, "-f", think, "-t", rofi, "-a", algo[0], "-g", algo[1], "-s", "n", "-c", "nothing"])
                                
                                count += 1
                                addToActions(totalActions, actions)
                                addToActions(totalNonConcurentActions, nonConcurentActions)
                                addToActionsCounts(allActionsCounts, actions)
                                addToActionsCounts(allNonConcurentActionsCounts, nonConcurentActions)
                                cost, _ = computeTotalCost(actions)
                                costsForMap[algoName][rofiname].append(cost)


                        if (count > 0):
                            avgActions = getAvgActions(totalActions, count)
                            avgNonConcuretnActions = getAvgActions(totalNonConcurentActions, count)
                            avgFilename = os.path.join(pathToEvals, mapName, algoName, rofiname, "avg.txt")
                            #printStatisticsToFile(avgFilename, avgActions, avgNonConcuretnActions)
                            #avgStatisticsForMap[algoName][rofiname]["all"] = avgActions
                            #avgStatisticsForMap[algoName][rofiname]["nonConcurent"] = avgNonConcuretnActions
                            statisticsForMap[algoName][rofiname] = allActionsCounts
                            nonConcurentStatisticsForMap[algoName][rofiname] = allNonConcurentActionsCounts


            #printAvgStatistics(mapName, mapType, avgStatisticsForMap)
            writeCSVAvgAndSdStatistics(mapName, mapType, "all", statisticsForMap, costsForMap)
            writeCSVAvgAndSdStatistics(mapName, mapType, "nonConcurent", statisticsForMap, None)


generateAllData()

def getCaptionAndLabel(filename, mapType):
    caption = ""
    label = "tab:map_"
    if (mapType == "rectangle"):
        caption = "Obdélníková mapa"
        label += "rect"
    elif (mapType == "singleHole"):
        caption = "Mapa s jednou dírou"
        label += "single"
    elif (mapType == "someHoles"):
        caption = "Mapa s několika dírami"
        label += "some"
    elif (mapType == "manyHoles"):
        label += "many"
        caption = "Mapa s mnoha dírami"

    _, csvName = os.path.split(filename)
    csvName = csvName.replace('p', 'x')
    csvName = csvName.replace('_', 'x')
    print(csvName)
    splitted = csvName.split('x')
    x = splitted[1]
    y = splitted[2]

    label += "_" + x + "_" + y
    caption += " o rozměrech $" + x + " \\times " + y + "$" 

    if (len(splitted) > 4):
        version = splitted[3]
        label += "_" + version
        caption += " verze " + version
    #print(x, y)

    caption += "."

    return caption, label



#getCaptionAndLabel("rectangle", "./testData/evaluations/rectangle/map3x15_all.csv")
    


def csvToLatexTable(filename, mapType):
    algorithms = []
    singleYCosts = []
    singleYSds = []
    singleNCosts = []
    singleNSds = []
    doubleCosts = []
    doubleSds = []
    with open(filename, newline='') as csvfile:
        csvReader = csv.reader(csvfile, delimiter=',')
        for row in csvReader:
            if (row[0] == ""):
                continue
            algorithms.append(row[0])
            singleYCosts.append(round(float(row[17]), 1))
            singleYSds.append(round(float(row[18]), 1))
            singleNCosts.append(round(float(row[35]), 1))
            singleNSds.append(round(float(row[36]), 1))
            doubleCosts.append(round(float(row[53]), 1))
            doubleSds.append(round(float(row[54]), 1))
    
    minSingleYCost = min(singleYCosts)
    minSingleNCost = min(singleNCosts)
    minDoubleCost = min(doubleCosts)

    latexTable = "\\begin{table}[h]\n" +  "\t\\footnotesize\n" + "\t\\centering\n" + "\t\\ttfamily\n" + "\t\\begin{tabular}{|l|r r|r r|r r|}\n \t\t\\hline \n"
    latexTable += "\t\t  & \\multicolumn{2}{c|}{single Y} & \\multicolumn{2}{c|}{single N} & \\multicolumn{2}{c|}{double} \\\\ \n"
    latexTable += "\t\t  & cena & $\\sigma$ & cena & $\\sigma$ & cena & $\\sigma$ \\\\ \\hline \\hline \n"

    for j in range(2):
        areSomeRows = False
        for i in range(len(singleYCosts)):
            if (j == 0 and "Bounds" not in algorithms[i] and "irect" not in algorithms[i] and "zig" not in algorithms[i]):
                continue
            if (j == 1 and ("Bounds" in algorithms[i] or "irect" in algorithms[i] or "zig" in algorithms[i])):
                continue
            row  = algorithms[i].replace("_", "\_")
            if (singleYCosts[i] == minSingleYCost):
                row += " & \\cheap "
            else:
                row += " & "
            row += str(singleYCosts[i]) + " & " + str(singleYSds[i])

            if (singleNCosts[i] == minSingleNCost):
                row += " & \\cheap "
            else:
                row += " & "
            row += str(singleNCosts[i]) + " & " + str(singleNSds[i])

            if (doubleCosts[i] == minDoubleCost):
                row += " & \\cheap "
            else:
                row += " & "
            row += str(doubleCosts[i]) + " & " + str(doubleSds[i])
            areSomeRows = True

            latexTable += "\t\t" + row +  " \\\\ \\hline " + "\n"
            
        if (j == 0 and areSomeRows):
            latexTable += "\t\t\\hline \n"
    
    caption, label = getCaptionAndLabel(filename, mapType)
    latexTable += "\t\\end{tabular} \n" + "\t\\caption{" + caption  + "}\n" + "\t\\label{" + label + "}\n" + "\\end{table}\n"

    print(latexTable)
    return latexTable

#csvToLatexTable("./testData/evaluations/rectangle/map3x3_all.csv")

def getSection(mapType):
    if (mapType == "rectangle"):
        return "Obdélníkové mapy"
    elif (mapType == "singleHole"):
        return "Mapy s jednou dírou"
    elif (mapType == "someHoles"):
        return "Mapy s několika dírami"
    elif (mapType == "manyHoles"):
        return "Mapy s mnoha dírami"
 

def writeAllTables():
    with open(os.path.join(".", "testData", "latexEval", "allTables.txt"), "w", encoding="UTF8") as allFile:
        for mapType in ["rectangle", "singleHole", "someHoles", "manyHoles"]:
            allFile.write("\\section{" + getSection(mapType) + "}\n")
            allFile.write("\\label{sec:tab_" + mapType + "}\n")
            pathToDir = os.path.join(".", "testData", "evaluations", mapType)
            print(pathToDir)
            files = os.listdir(pathToDir)
            for f in sorted(files):
                #print(f)
                if ("_all.csv" not in f):
                    continue
                pathToCSV = os.path.join(pathToDir, f)
                print(pathToCSV)

                latexTable = csvToLatexTable(pathToCSV, mapType)
                latexName = f.split(".")[0] + ".txt"

                with open(os.path.join(".", "testData", "latexEval", mapType, latexName), 'w', encoding='UTF8') as latexFile:
                    latexFile.write(latexTable)
                allFile.write(latexTable)
                print("***", f)
                print(sorted(files))

                allFile.write("\\clearpage")
                allFile.write("\n")
                allFile.write("\n")

 
#writeAllTables()
