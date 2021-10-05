#!/usr/bin/env python3

import click
import os
import copy
from lxml import objectify, etree

def stripNamespace(tree):
    # Taken from https://stackoverflow.com/questions/18159221/remove-namespace-and-prefix-from-xml-in-python-using-lxml
    for elem in tree.getiterator():
        # Skip comments and processing instructions,
        # because they do not have names
        if not (
            isinstance(elem, etree._Comment)
            or isinstance(elem, etree._ProcessingInstruction)
        ):
            # Remove a namespace URI in the element's name
            elem.tag = etree.QName(elem).localname

    # Remove unused namespace declarations
    etree.cleanup_namespaces(tree)
    return tree

def normalizeGpioName(name):
    return name.split("-", 1)[0]

def normalizeAf(name):
    return int(name.split("_")[1][2:])

def collectMcu(db, filter=lambda refName: True):
    """
    Given a path to DB, extract all available processor in the form of a
    dictionary RefName -> Name.

    You have the possibility to add a filter function.
    """
    mcus = {}

    tree = etree.parse(os.path.join(db, "mcu", "families.xml"))
    families = tree.getroot()
    for family in families:
        for subfamily in family:
            for mcu in subfamily:
                refName = mcu.attrib["RefName"]
                name = mcu.attrib["Name"]
                if filter(refName):
                    mcus[refName] = name
    return mcus

def extractPinAf(db, mcuName, filter=lambda pinName, peripheral, signal: True):
    """
    Given a path to DB, extract a dictionary (pinName, peripheral, signal) ->
    AF. Works only for families that have multiple remappings through alternate
    functions. Does not, e.g., work on STM32F1xx that features only single
    remapping.
    """
    mcuTree = etree.parse(os.path.join(db, "mcu", f"{mcuName}.xml"))
    mcuTree = stripNamespace(mcuTree)
    version = mcuTree.find("./IP[@Name='GPIO']").attrib["Version"]
    pinTree = etree.parse(os.path.join(db, "mcu", "IP", f"GPIO-{version}_Modes.xml"))
    pinTree = stripNamespace(pinTree)
    mapping = {}
    for pin in pinTree.findall(".//GPIO_Pin"):
        for x in pin.findall("./PinSignal"):
            name = normalizeGpioName(pin.attrib["Name"])
            peripheral, signal = tuple((x.attrib["Name"].split("_", 1) + [""])[:2])
            af = normalizeAf(x.find(".//PossibleValue").text)
            if filter(name, peripheral, signal):
                mapping[(name, peripheral, signal)] = af
    return mapping

def firstDigitIndex(s):
    return [x.isdigit() for x in s].index(True)

def functionKey(args):
    pinName, peripheral, af = args

    perSplit = firstDigitIndex(peripheral)
    perName = peripheral[:perSplit]
    perNum = int(peripheral[perSplit:])

    port, pinNum = splitPinName(pinName)

    return perNum, perName, port, pinNum, af

def splitPinName(pinName):
    # There are some notes about pin names
    pinName = pinName.split("/")[0]
    pinName = pinName.split("[")[0]

    pinSplit = firstDigitIndex(pinName)
    port = pinName[:pinSplit]
    pinNum = int(pinName[pinSplit:])
    return port, pinNum

def extreactSignalRemapping(remapping, signalName):
    """
    Given a remapping (dictionary refname -> (pinName, peripheral, signal)) and
    signal name, generate function tables.

    Returns a dictionary of [remapping] -> [applicable refnames]
    """
    remapping = copy.deepcopy(remapping)
    for mcu, pinMap in remapping.items():
        remapping[mcu] = {
            (pinName, peripheral, signal): af
            for (pinName, peripheral, signal), af in pinMap.items()
            if signal == signalName
        }

    separateFunctions = {}
    for mcu, pinMap in remapping.items():
        l = [(pinName, peripheral, af)
                for (pinName, peripheral, _), af in pinMap.items()]
        l.sort(key=functionKey)
        l = tuple(l)
        if l in separateFunctions:
            separateFunctions[l].append(mcu)
        else:
            separateFunctions[l] = [mcu]

    return separateFunctions

def mappingPerPeripheral(mappingList):
    mapping = {}
    for pinName, peripheral, af in mappingList:
        if peripheral not in mapping:
            mapping[peripheral] = []
        port, offset = splitPinName(pinName)
        mapping[peripheral].append((port, offset, af))
    for x in mapping.values():
        x.sort()
    mapping = [(peripheral, pins) for peripheral, pins in mapping.items()]
    mapping.sort(key=lambda x: x[0])
    return mapping

def renderSignalRemapping(output, signalName, separateFunctions, indentLevel=1, indentStep=4):
    i0 = (indentLevel * indentStep) * " "
    i1 = i0 + indentStep * " "
    i2 = i1 + indentStep * " "
    i3 = i2 + indentStep * " "
    for mapping, mcus in separateFunctions.items():
        output.write(f"{i0}#if ")
        CHUNK_SIZE = 3
        mcuChunks = [mcus[i:i + CHUNK_SIZE] for i in range(0, len(mcus), CHUNK_SIZE)]
        mcuChunks = [" || ".join([f"defined({x})" for x in chunk]) for chunk in mcuChunks]
        mcuString = f" || \\\n{i0}    ".join(mcuChunks) + "\n"
        output.write(mcuString)
        for peripheral, pins in mappingPerPeripheral(mapping):
            output.write(f"{i1}if ( periph == {peripheral} ) {{\n")
            for port, offset, af in pins:
                output.write(f"{i2}if ( port == {port.replace('P', 'GPIO')} && pos == {offset} )\n")
                output.write(f"{i3}return {af};\n")
            output.write(f"{i1}}}\n")
        output.write(f"{i0}#endif\n")
    output.write(f"{i0}assert( false && \"Incorrect {signalName} pin\" );\n")
    output.write(f"{i0}__builtin_trap();\n")

def renderUartSignalFunction(output, signal, mapping, indentLevel=1, indentStep=4):
    i0 = (indentLevel * indentStep) * " "
    i1 = i0 + indentStep * " "
    output.write(f"{i0}int uartAlternateFun{signal}( USART_TypeDef *periph, GPIO_TypeDef *port, int pos ) {{\n")
    separateFunctions = extreactSignalRemapping(mapping, signal)
    renderSignalRemapping(output, signal, separateFunctions, indentLevel + 1, indentStep)
    output.write(f"{i0}}}\n\n")

@click.command()
@click.option("--db", type=click.Path(dir_okay=True, file_okay=False, exists=True),
    default="/opt/STM32CubeMX/db", help="Path to DB directory of CubeMX")
@click.option("--family", type=str)
@click.argument("output", type=click.File("w"))
def uart(db, family, output):
    mcus = collectMcu(db, lambda refName: refName.startswith(family.replace("X", "")))
    uartMapping = {
        refname: extractPinAf(db, name,
            lambda pinName, peripheral, signal: peripheral.startswith("USART") or peripheral.startswith("UART"))
        for refname, name in mcus.items()}

    output.write(f"#include <{family.lower()}_ll_bus.h>\n")
    output.write(f"#include <{family.lower()}_ll_usart.h>\n")
    output.write(f"#include <{family.lower()}_ll_gpio.h>\n")
    output.write(f"#include <cassert>\n")
    output.write("\n")

    output.write("namespace detail {\n")
    renderUartSignalFunction(output, "TX", uartMapping, 1)
    renderUartSignalFunction(output, "RX", uartMapping, 1)
    renderUartSignalFunction(output, "CTS", uartMapping, 1)
    renderUartSignalFunction(output, "RTS", uartMapping, 1)
    renderUartSignalFunction(output, "DE", uartMapping, 1)
    output.write("} // namespace detail\n\n")

@click.group()
def run():
    pass

run.add_command(uart)

if __name__ == "__main__":
    run()
