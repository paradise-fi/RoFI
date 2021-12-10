from enum import Enum, auto

class Ori(Enum):
	"""
	Enum class representing possible orientations and directions
	"""

	N = "N"
	E = "E"
	S = "S"
	W = "W"

class Side(Enum):
	"""
	Enum class representing sides of universal module
	"""

	A = "A"
	B = "B"

class Dock(Enum):
	"""
	Enum class representing docks of universal module
	"""

	PLUS_X = "+X"
	MINUS_X = "-X"
	MINUS_Z = "-Z"
	
class Degree(Enum):
	"""
	Enum class representing possible rotation angles of degrees of freedom of rofibots
	""" 

	NINETY = 90
	ZERO = 0
	MINUS_NINETY = -90
	ONEHUNDREDEIGHTY = 180
	# MINUS_ONEHUNDREDEIGHTY = -180

class RofibotType(Enum):
	"""
	Enum class representing possible rofibot types
	""" 

	SINGLE = auto()
	DOUBLE = auto()
