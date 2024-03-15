mapping = {
    "3 A" : "1 A",
    "3 B" : "1 B",
    "7 A" : "6 B",
    "7 B" : "6 A",
    "6 B" : "7 B",
    "6 A" : "7 A",
    "2 B" : "3 A",
    "2 A" : "3 B",
    "1 A" : "5 A",
    "1 B" : "5 B",
    "5 A" : "0 B",
    "5 B" : "0 A",
    "4 B" : "2 A",
    "4 A" : "2 B",
    "0 B" : "4 B",
    "0 A" : "4 A"
}
mapping2 = {
    "3 A" : "4 A",
    "3 B" : "4 B",
    "7 A" : "2 B",
    "7 B" : "2 A",
    "6 B" : "0 A",
    "6 A" : "0 B",
    "2 B" : "5 B",
    "2 A" : "5 A",
    "1 A" : "3 B",
    "1 B" : "3 A",
    "5 A" : "7 A",
    "5 B" : "7 B",
    "4 B" : "6 A",
    "4 A" : "6 B",
    "0 B" : "1 B",
    "0 A" : "1 A"
}
chair2_spider = {
    "12 A" : "3 A",
    "12 B" : "3 B",
    "10 B" : "7 A",
    "10 A" : "7 B",
    "8 A" : "6 B",
    "8 B" : "6 A",
    "5 B" : "2 B",
    "5 A" : "2 A",
    "11 B" : "1 A",
    "11 A" : "1 B",
    "7 A" : "5 A",
    "7 B" : "5 B",
    "6 A" : "4 B",
    "6 B" : "4 A",
    "9 B" : "0 B",
    "9 A" : "0 A"
}
chair3_spider = {y: x for x, y in chair2_spider.items()}
def parse():
    file = open( "/home/patrick/RoFI/data/configurations/kinematics/spider_to_snake.rofi", "r" )
    out = open( "/home/patrick/RoFI/data/configurations/kinematics/remapped.rofi", "w" )

    config = ""
    for line in file:
        chars = line.split()
        new = ""
        if( line == "\n" or line == "animate\n" ):
            out.write( line )
            continue
        if( chars[ 0 ] == "C" ):
            new += "C"
        if( chars[ 0 ] == "M" ):
            new += "M "
            sideA = chair3_spider[ chars[ 1 ] + " A" ]
            new += sideA.split()[ 0 ] + " "
            if( sideA.split()[ 1 ] == "A" ):
                new += chars[ 2 ] + " " + chars[ 3 ]
            else:
                new += chars[ 3 ] + " " + chars[ 2 ]
            new += " " + chars[ 4 ]
        if( chars[ 0 ] == "E" ):
            new += "E "
            new += chair3_spider[ ( chars[ 1 ] + " " + chars[ 2 ] ) ]
            new += " " + chars[ 3 ] + " " + chars[ 4 ] + " " + chars[ 5 ] + " "
            other = chair3_spider[ ( chars[ 7 ] + " " + chars[ 6 ] ) ].split()
            new += other[ 1 ] + " " + other[ 0 ]

        out.write( new + "\n" )


if __name__ == "__main__":
    parse()
