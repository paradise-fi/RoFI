import json
import numpy as np
import matplotlib.pyplot as plt


class TestResult:

    def __init__(self, testName_, timeout_, outOfMemory_, time_, memUsage_):
        self.testName = testName_
        self.timeout = timeout_
        self.outOfMemory = outOfMemory_
        self.memUsage = memUsage_
        self.success = False
        self.time = time_
        self.layerNodes = None
        self.maxQueueSize = None
        self.descendantsGenerated = None
        self.nodesTotal = None
        self.maxDescendants = None
        self.pathFound = None
        self.pathLength = None

    def __init__(self, testName_, timeout_, outOfMemory_, time_, memUsage_, layerNodes_, maxQueueSize_, descendantsGenerated_, nodesTotal_, maxDescendants_, pathFound_, pathLength_):
        self.testName = testName_
        self.timeout = timeout_
        self.outOfMemory = outOfMemory_
        self.memUsage = memUsage_
        self.success = not (timeout_ or outOfMemory_)
        self.time = time_
        self.layerNodes = layerNodes_
        self.maxQueueSize = maxQueueSize_
        self.descendantsGenerated = descendantsGenerated_
        self.nodesTotal = nodesTotal_
        self.maxDescendants = maxDescendants_
        self.pathFound = pathFound_
        self.pathLength = pathLength_

    def __repr__(self):
        output = ""
        output += 'Name: ' + str(self.testName) + '\n'
        output += 'timeout: ' + str(self.timeout) + '\n'
        output += 'outOfMemory: ' + str(self.outOfMemory) + '\n'
        output += 'memUsage: ' + str(self.memUsage) + '\n'
        output += 'success: ' + str(self.success) + '\n'
        output += 'time: ' + str(self.time) + '\n'
        output += 'pathFound: ' + str(self.pathFound) + '\n'
        output += 'pathLength: ' + str(self.pathLength) + '\n'
        output += 'layerNodes: ' + str(self.layerNodes) + '\n'
        output += 'maxQueueSize: ' + str(self.maxQueueSize) + '\n'
        output += 'descendantsGenerated: ' + str(self.descendantsGenerated) + '\n'
        output += 'nodesTotal: ' + str(self.nodesTotal) + '\n'
        output += 'maxDescendants: ' + str(self.maxDescendants)
        return output

# result = np.array(np.void(['Name', 'timeout', 'outOfMemory', 'time', 'memUsage', 'maxQSize', 'generatedDescendants', 'totalNodes', 'maxDescendants', 'foundPath', 'pathLength']))
def loadFileResults(filePath):
    with open(filePath) as f:
        data = json.load(f)

    i = 0
    result = []
    for task in data['tasks']:
        # commandParts = task['command'].split()
        # taskName = commandParts[3] + " to " + commandParts[4]
        taskName = str(i)
        i = i + 1
        timeout = task['stats']['timeout']
        outOfMemory = task['stats']['outOfMemory'] 
        time = task['stats']['cpuTime'] // 1000000 # [s]
        memUsage = task['stats']['memUsage'] // 1024 // 1024 # [MB]
        success = not (outOfMemory or timeout)

        if success:
            output = json.loads(task['output'])
            next_row = taskName, timeout, outOfMemory, time, memUsage, output['maxQSize'], output['generatedDescendants'], output['totalNodes'], output['maxDescendants'], output['foundPath'], output['pathLength']
        else:
            next_row = taskName, timeout, outOfMemory, time, memUsage, None, None, None, None, None, None

        result.append(next_row)

    dt = np.dtype([
        ('name', 'U10'), 
        ('timeout', bool), 
        ('outOfMemory', bool), 
        ('time', 'i4'), 
        ('memUsage', 'i4'), 
        ('maxQSize', 'i4'), 
        ('generatedDescendants', 'i4'), 
        ('totalNodes', 'i4'), 
        ('maxDescendants', 'i4'), 
        ('foundPath', bool), 
        ('pathLength', 'i4')])

    result = np.array(result, dtype=dt)

    return result

oldShapestar = loadFileResults('./experiments/isoreconfig/graphs/shapestarOldDesc.json')
quickShapestar = loadFileResults('./experiments/isoreconfig/graphs/shapestarQDesc.json')
bfshapeOldDesc = loadFileResults('./experiments/isoreconfig/graphs/bfshapeOldDesc.json')
bfshapeNewDesc = loadFileResults('./experiments/isoreconfig/graphs/bfshapeNewDesc.json')
bfseigen = loadFileResults('./experiments/isoreconfig/graphs/bfseigen.json')

# plt.setp(ax.get_xticklabels(), rotation=90, horizontalalignment='right')
cdict = {'before': 'green', 'after': 'red'}

def graphScatter(data, x_atr, y_atr, label_leg):
    plt.scatter(data[x_atr], data[y_atr], alpha=0.5, color=cdict[label_leg], label=label_leg)

def graphBar(data, x_atr, y_atr, label_leg):
    plt.bar(data[x_atr], data[y_atr], width=0.9, alpha=0.5, color=cdict[label_leg], label=label_leg)

# Time
fig, ax = plt.subplots()
graphBar(bfshapeNewDesc, 'name', 'time', 'before')
graphBar(bfseigen, 'name', 'time', 'after')
ax.legend(loc='upper left') 
plt.savefig('experiments/isoreconfig/graphs/bfsShapeEigenTime.png')
fig.clf()

# Memory
fig, ax = plt.subplots()
graphBar(bfshapeNewDesc, 'name', 'memUsage', 'before')
graphBar(bfseigen, 'name', 'memUsage', 'after')
ax.legend(loc='upper left') 
plt.savefig('experiments/isoreconfig/graphs/bfsShapeEigenMem.png')
fig.clf()

# Descendants generated 
fig, ax = plt.subplots()
graphBar(bfshapeNewDesc, 'name', 'generatedDescendants', 'before')
graphBar(bfseigen, 'name', 'generatedDescendants', 'after')
ax.legend(loc='upper left') 
plt.savefig('experiments/isoreconfig/graphs/bfsShapeEigenDesc.png')
fig.clf()

# Total nodes 
fig, ax = plt.subplots()
graphBar(bfshapeNewDesc, 'name', 'totalNodes', 'before')
graphBar(bfseigen, 'name', 'totalNodes', 'after')
ax.legend(loc='upper left') 
plt.savefig('experiments/isoreconfig/graphs/bfsShapeEigenNodes.png')
fig.clf()

# Path length
fig, ax = plt.subplots()
graphBar(bfshapeNewDesc, 'name', 'pathLength', 'before')
graphBar(bfseigen, 'name', 'pathLength', 'after')
ax.legend(loc='upper left') 
plt.savefig('experiments/isoreconfig/graphs/bfsShapeEigenPathLen.png')
fig.clf()
