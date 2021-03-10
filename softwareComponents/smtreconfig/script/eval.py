#!/usr/bin/env python3

from pysmt.shortcuts import read_smtlib, get_formula_size, get_env, Solver
from pysmt.oracles import SizeOracle
from pysmt.logics import QF_NRA, NRA
from pysmt.shortcuts import Symbol, And, GE, LT, Plus, Times, Equals, NotEquals, Int, Real
from pysmt.typing import INT, REAL
from tabulate import tabulate
import os
import subprocess
import json
import pathlib
import click
import resource
import time
import sys
import datetime
import multiprocessing
from multiprocessing import Process, Manager, Pool
from multiprocessing.pool import ThreadPool
from textwrap import dedent

ENV = {
    "reconfigTool": "../../build/smtreconfig/smt-reconfig",
    "workingDir": "results",
    "timeout": 30 * 60,
    "memoryLimit": 20 * 1024,
    "extraGenParams": []
}

# stolen from: https://gist.github.com/s3rvac/f97d6cbdfdb15c0a32e7e941f7f4a3fa
def limit_virtual_memory(mbLimit):
    bytesLimit = mbLimit * 1024 * 1024
    resource.setrlimit(resource.RLIMIT_AS, (bytesLimit, bytesLimit))

class Benchmark:
    @staticmethod
    def fromJson(json, env=None):
        benchmark = Benchmark()
        benchmark.name = json["name"]
        benchmark.initCfgFile = json["initCfgFile"]
        benchmark.targetCfgFile = json["targetCfgFile"]
        benchmark.maxSteps = json["maxSteps"]
        benchmark.env = env
        return benchmark

    def directory(self):
        return os.path.join(self.env["workingDir"], self.name)

    def createBenchmarkDir(self):
        pathlib.Path(self.directory()).mkdir(parents=True, exist_ok=True)

    def formulaFile(self, steps):
        return os.path.join(self.directory(), "{}-{}.smtlib2".format(self.name, steps))

    def buildStatFile(self, steps):
        return os.path.join(self.directory(), "{}-{}-build.json".format(self.name, steps))

    def runStatFile(self, steps):
        return os.path.join(self.directory(), "{}-{}-run.json".format(self.name, steps))

    def readFormulaGenStat(self, text):
        stat = {
            "timeout": self.env["timeout"],
            "timeouted": False
        }
        for line in text.decode("utf-8").split("\n"):
            if "Formula build-time" in line:
                x = [int(s) for s in line.split() if s.isdigit()][0]
                stat["build time"] = x
        return stat

    def generateFormula(self, steps, forceRegenerate=False):
        self.createBenchmarkDir()
        formulaFile = self.formulaFile(steps)
        buildStatFile = self.buildStatFile(steps)
        if not forceRegenerate and os.path.exists(formulaFile) and os.path.exists(buildStatFile):
            return
        print("Generating {}".format(formulaFile))
        cmd = [self.env["reconfigTool"],
            "smt",
            "-i", self.initCfgFile,
            "-f", self.targetCfgFile,
            "-l", str(steps)] + self.env["extraGenParams"]
        try:
            proc = subprocess.run(cmd,
                capture_output=True,
                timeout=self.env["timeout"],
                check=True,
                preexec_fn=lambda: limit_virtual_memory(self.env["memoryLimit"])
            )
            with open(formulaFile, "w") as f:
                f.write(proc.stdout.decode("utf-8"))
            stat = self.readFormulaGenStat(proc.stderr)
            formula = read_smtlib(formulaFile)
            # stat["nodeSize"] = get_formula_size(formula, SizeOracle.MEASURE_TREE_NODES)
            # stat["dagSize"] = get_formula_size(formula, SizeOracle.MEASURE_DAG_NODES)
            # stat["varCount"] = get_formula_size(formula, SizeOracle.MEASURE_SYMBOLS)
            # stat["fileSize"] = os.path.getsize(formulaFile) / 1024 / 1024
        except subprocess.TimeoutExpired as e:
            stat = {
                "timeout": self.env["timeout"],
                "timeouted": True
            }
        except subprocess.CalledProcessError as e:
            stat = {
                "timeout": self.env["timeout"],
                "timeouted": False,
                "error": e.stderr.decode("utf-8"),
                "code": e.returncode
            }
        with open(buildStatFile, "w") as f:
            f.write(json.dumps(stat, indent=4))

    def generateFormulas(self, forceRegenerate=False):
        for i in self.steps():
            self.generateFormula(i, forceRegenerate)

    def benchmarkList(self):
        return [(self, steps) for steps in self.steps()]

    # Run benchmark in separate process to limit the available memory
    def _runBenchmark(self, solverName, steps, stat):
        limit_virtual_memory(self.env["memoryLimit"])
        sys.stdout = open("/tmp/" + str(os.getpid()) + ".out", "w")
        sys.stderr = open("/tmp/" + str(os.getpid()) + "_error.out", "w")
        formula = read_smtlib(self.formulaFile(steps))
        with Solver(name=solverName, logic=QF_NRA) as solver:
            solver.add_assertion(formula)
            result = solver.solve()
            stat["result"] = result

    def runBenchmark(self, solverName, steps):
        with Manager() as manager:
            stat = manager.dict()
            stat["name"] = self.name
            stat["steps"] = steps
            stat["solver"] = solverName
            stat["timeout"] = self.env["timeout"]
            p = Process(target=self._runBenchmark, args=(solverName, steps, stat))
            startTime = time.time()
            p.daemon=True
            p.start()
            while time.time() - startTime < self.env["timeout"]:
                if not p.is_alive():
                    break
                time.sleep(0.5)
            endTime = time.time()
            duration = endTime - startTime
            if duration > self.env["timeout"]:
                p.kill()
                stat["timeouted"] = True
            else:
                stat["solve time"] = int((endTime - startTime) * 1000)
                code = p.exitcode
                if code != 0:
                    stat["code"] = code
                    with open("/tmp/" + str(p.pid) + "_error.out", "r") as f:
                        stat["error"] = f.read()
                stat["timeouted"] = False
            p.join()
            return dict(stat)

    def storeResult(self, result):
        self.createBenchmarkDir()
        statFile = self.runStatFile(result["steps"])
        if os.path.exists(statFile):
            with open(statFile, "r") as f:
                stat = json.load(f)
        else:
            stat = {}
        result.update({
            "date": datetime.datetime.now().isoformat()
        })
        stat[result["solver"]] = result
        with open(statFile, "w") as f:
            f.write(json.dumps(stat, indent=4))

    def steps(self):
        return range(2, self.maxSteps)

    def loadRunStats(self):
        stats = [{}, {}]
        for i in self.steps():
            with open(self.runStatFile(i), "r") as f:
                stats.append(json.load(f))
        self.runStat = stats

    def loadBuildStats(self):
        stats = [{}, {}]
        for i in self.steps():
            with open(self.buildStatFile(i), "r") as f:
                stats.append(json.load(f))
        self.buildStat = stats

    def collectSolvers(self):
        solvers = set()
        for i in self.steps():
            solvers.update(self.runStat[i].keys())
        return solvers

    def solveResult(self, solver, step):
        try:
            entry = self.runStat[step][solver]
        except KeyError:
            return -1, "Missing stat"
        if "error" in entry:
            return -1, "error"
        if entry["timeouted"]:
            return entry["timeout"], "timeout"
        return entry["solve time"] / 1000, "sat" if entry["result"] else "unsat"

def loadBenchmarks(jsonFileName, env=None):
    benchmarks = []
    with open(jsonFileName, "r") as f:
        for benchmarkJson in json.load(f):
            benchmarks.append(Benchmark.fromJson(benchmarkJson, env))
    return benchmarks

def loadSolvers(solversFile):
    smtEnv = get_env()
    with open(solversFile, "r") as f:
        solvers = json.load(f)
        for solver in solvers:
            smtEnv.factory.add_generic_solver(solver["name"],
                list(solver["args"]), [NRA, QF_NRA])
        return solvers

def benchmarkByName(benchmarks, name):
    for b in benchmarks:
        if b.name == name:
            return b

def runBenchmark(arg):
    solverName, benchmark, steps = arg
    benchmark.generateFormula(steps)
    return benchmark.runBenchmark(solverName, steps)

def runBenchmarks(solvers, benchmarks, nodeCount):
    benchmarkList = [bench for bInst in benchmarks for bench in bInst.benchmarkList()]
    benchmarkList = [(solver["name"], benchmark, steps) for benchmark, steps in benchmarkList for solver in solvers]
    benchmarkList = list(benchmarkList)

    count = 1
    total = len(benchmarkList)
    print("Running {} benchmarks on {} workers".format(total, nodeCount))
    with ThreadPool(nodeCount) as pool:
        for result in pool.imap_unordered(runBenchmark, benchmarkList):
            print("Done {: >4}/{}: {} on {} steps with solver {}".format(
                count, total, result["name"], result["steps"], result["solver"]))
            count += 1
            benchmarkByName(benchmarks, result["name"]).storeResult(result)
    print("Done")

def getEnv(envPath):
    if envPath is None:
        return ENV
    env = dict(ENV)
    print(env)
    env.update(json.load(open(envPath, "r")))
    return env

@click.command()
@click.argument("solvers", type=click.Path(exists=True))
@click.argument("benchmarks", type=click.Path(exists=True))
@click.option('--environment', '-e', type=click.Path(exists=True), default=None)
def eval(benchmarks, solvers, environment):
    env = getEnv(environment)

    print("Generating solvers...")
    solvers = loadSolvers(solvers)
    print("    Done")
    print("Generating benchmarks...")
    benchmarks = loadBenchmarks(benchmarks, env)
    print("     Done")

    print("Generating formulas...")
    for benchmark in benchmarks:
        benchmark.generateFormulas(forceRegenerate=False)
    print("    Done")

    runBenchmarks(solvers, benchmarks, multiprocessing.cpu_count() - 2)

def solve(solverName, formula):
    with Solver(name=solverName, logic=QF_NRA) as solver:
        solver.add_assertion(formula)
        return solver.solve()

@click.command()
@click.argument("solvers", type=click.Path(exists=True))
def testSolvers(solvers):
    solvers = loadSolvers(solvers)
    x = Symbol("x", REAL)
    formula1 = Equals(Times(x, x), Real(2))
    y = Symbol("y")
    z = Symbol("z")
    formula2 = And(z, y)
    pool = Pool(1)
    for s in solvers:
        try:
            name = s["name"]
            print("Testing solver {}".format(name))
            res = pool.apply_async(solve, args=(name, formula1))
            try:
                if not res.get(5):
                    print("    {} is not OK - wrong result returned".format(name))
                    continue
            except TimeoutError:
                print("    {} timeouted".format(name))
            res = pool.apply_async(solve, args=(name, formula2))
            try:
                if not res.get(5):
                    print("    {} is not OK - wrong result returned".format(name))
                    continue
            except TimeoutError:
                print("    {} timeouted".format(name))
            print("    {} is OK".format(name))
        except Exception as e:
            print("Exception {}", type(e))
            print(e)

def collectSolvers(benchmarks):
    solvers = set()
    for b in benchmarks:
        solvers = solvers.union(b.collectSolvers())
    return solvers

def resultToTex(results, timeout=500):
    columns = len(results[0])
    table = r'\setlength\tabcolsep{1pt}' + "\n"
    table += r'\begin{tabularx}{\linewidth}{r@{\hskip 6pt}' + "C" * columns + '}\n'
    table += "    " + " & ".join(["Steps:"] + list([str(2 + x) for x in range(columns - 1)])) + r' \\ \toprule'

    prevSolver = ""
    for row in results:
        if row[0] in ["cvc4-1.3", "cvc4-1.4", "smtrat-18.12", "smtrat-19.10", "smtrat-upstream"]:
            continue
        if prevSolver != "" and not row[0].startswith(prevSolver[:2]):
            table += "\midrule "
        table += '\n'
        prevSolver = row[0]

        table += "    " + row[0].replace("upstream", "up")
        for solveTime, result  in row[1:]:
            if result == "error":
                content = r"\cellcolor{tblPurple}E"
            elif result == "timeout":
                content = r'\cellcolor{tblRed}T'
            else:
                red = int(solveTime / timeout * 100)
                if red > 100:
                    red = 100
                content = r'\cellcolor{{tblRed!{}!tblGreen}}{:.0f}'.format(red, solveTime)
            table += " & " + content
        table += r' \\ '

    table += r'    \bottomrule' + "\n"
    table += r'\end{tabularx}' + "\n"
    return table

def generateBenchmarkTable(env, benchmark, solvers):
    solvers = list(solvers)
    solvers.sort()
    tableContent = []
    tableContentRaw = []
    for solver in solvers:
        rawRow = [benchmark.solveResult(solver, step) for step in benchmark.steps()]
        row = map(lambda x:
            x[1] if x[1] in ["error", "timeout"] else "{} ({})".format(x[0], x[1]),
            rawRow)
        row = list(row)
        tableContent.append([solver] + row)
        tableContentRaw.append([solver] + list(rawRow))
    with open(os.path.join(benchmark.directory(), "table.tex"), "w") as f:
        f.write(resultToTex(tableContentRaw))

@click.command()
@click.argument("benchmarks", type=click.Path(exists=True))
@click.option('--environment', '-e', type=click.Path(exists=True), default=None)
def generateTables(benchmarks, environment):
    env = getEnv(environment)
    benchmarks = loadBenchmarks(benchmarks, env)
    for b in benchmarks:
        b.loadBuildStats()
        b.loadRunStats()
    solvers = collectSolvers(benchmarks)

    for b in benchmarks:
        generateBenchmarkTable(env, b, solvers)
    summary = dedent(r"""
        \documentclass[11pt,a4paper]{article}
        \usepackage{times}

        % numbers option provides compact numerical references in the text.
        \usepackage[numbers]{natbib}
        \usepackage{multicol}
        \usepackage[bookmarks=true]{hyperref}
        \usepackage{tikz}
        \usetikzlibrary{arrows,shapes,backgrounds,fit}
        \usepackage{tabularx}
        \usepackage{xurl}
        \usepackage{mathtools}
        \usepackage{booktabs}
        \usepackage{tabularx}
        \usepackage{colortbl}
        \usepackage{diagbox}

        \begin{document}

        \definecolor{tblRed}{rgb}{0.92, 0.43, 0.43}
        \definecolor{tblGreen}{rgb}{0.74 0.85 0.43}
        \definecolor{tblPurple}{rgb}{0.85, 0.59, 0.81}
        \newcolumntype{C}{>{\centering\arraybackslash}X}
        """)

    summary += r"\title{\Huge{" + env["workingDir"] + "}}\n\\maketitle\n\n"

    for b in benchmarks:
        summary += dedent(r"""
            \begin{table}[]
            \centering
            \input{""")
        summary += os.path.join("..", b.directory(), "table.tex")
        summary += "}\n"
        summary += r"\caption{"
        summary += b.name
        summary += "}\n\\end{table}\n\n"

    summary += r"\end{document}"
    summaryTableFile = os.path.join(env["workingDir"], "table.tex")
    with open(summaryTableFile, "w") as f:
        f.write(summary)

    os.system("cd {}; latexmk -pdf table.tex".format(env["workingDir"]))

@click.group()
def cli():
    pass

cli.add_command(eval)
cli.add_command(testSolvers)
cli.add_command(generateTables)

if __name__ == "__main__":
    cli()
