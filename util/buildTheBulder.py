#!/usr/bin/env python3

import sys
import re

# https://stackoverflow.com/questions/4303492/how-can-i-simplify-this-conversion-from-underscore-to-camelcase-in-python
def u2c(name, all=False):
    pieces = name.split("_")
    if all:
        pieces = [x.capitalize() for x in pieces]
    else:
        pieces = [pieces[0]] + [x.capitalize() for x in pieces[1:]]
    return "".join(pieces)

def indent(n):
    return " " * 4 * n;

text = sys.stdin.read()
text = re.sub(r"//.*\n", "", text)
text = re.sub(r"/\*.*\*/", "", text)
text = text.replace("\n", "")

m = re.match(r"typedef\s+struct\s+\{(.*)\}\s*([a-zA-Z_]\w*);", text)

struct_name = m.group(2)
class_name = u2c(m.group(2).replace("_config_t", ""), all=True)

entries = []
for x in m.group(1).split(";"):
    m = re.match(r"\s*([a-zA-Z_]\w*)\s+([a-zA-Z_]\w*)\s*", x)
    if not m:
        continue
    entries.append( (m.group(1), m.group(2)) )

print("class " + class_name + " {")
print("public:")
print(indent(1) + class_name + "() {}")
print(indent(1) + "operator " + struct_name + "() const { return _s; }")
for t, n in entries:
    print(indent(1) + "{c}& {nn}( {t} p ) {{ _s.{on} = p; return *this; }}"
        .format(c=class_name, nn=u2c(n), on=n, t=t))

print("private:")
print(indent(1) + struct_name + " _s;")
print("};")

print("")
print("std::ostream& operator<<( std::ostream& o, {n} s ) {{".format(n=struct_name))
for t, n in entries:
    if t.startswith("uint8_t"):
        print(indent(1) + 'o << "{n}: " << static_cast< int >( s.{n} ) << "\\n";'.format(n=n))
    else:
        print(indent(1) + 'o << "{n}: " << s.{n} << "\\n";'.format(n=n))
print(indent(1) + "return o;")
print("}")