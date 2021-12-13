#!/bin/bash

python ../../tools/padWalking/../../tools/padWalking/main.py -h


echo "Rectangles"
python ../../tools/padWalking/../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a direct -s none -c file -u ./data/reference/rectangle/map5x6-single_y-direct.in
python ../../tools/padWalking/../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f n -a direct -s none -c file -u ./data/reference/rectangle/map5x6-single_n-direct.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t double -a direct -s none -c file -u ./data/reference/rectangle/map5x6-double-direct.in

python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a directZigZag -s none -c file -u ./data/reference/rectangle/map5x6-single_y-directZigZag.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f n -a directZigZag -s none -c file -u ./data/reference/rectangle/map5x6-single_n-directZigZag.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t double -a directZigZag -s none -c file -u ./data/reference/rectangle/map5x6-double-directZigZag.in

python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a zigzag -s none -c file -u ./data/reference/rectangle/map5x6-single_y-zigzag.in

python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfs -g s -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfs -g se -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfs -g seb -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfs_seb.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfs -g reb -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfs_reb.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfs -g re -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfs_re.in


python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f n -a dfs -g s -s none -c file -u ./data/reference/rectangle/map5x6-single_n-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f n -a dfs -g se -s none -c file -u ./data/reference/rectangle/map5x6-single_n-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f n -a dfs -g seb -s none -c file -u ./data/reference/rectangle/map5x6-single_n-dfs_seb.in

python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t double -a dfs -g s -s none -c file -u ./data/reference/rectangle/map5x6-double-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t double -a dfs -g se -s none -c file -u ./data/reference/rectangle/map5x6-double-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t double -a dfs -g seb -s none -c file -u ./data/reference/rectangle/map5x6-double-dfs_seb.in


python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfsb -g s -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfsb_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfsb -g se -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfsb_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfsb -g seb -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfsb_seb.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfsb -g reb -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfsb_reb.in


python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfsl -g s -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfsl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfsl -g se -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfsl_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfsl -g seb -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfsl_seb.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfsl -g reb -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfsl_reb.in

python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t double -a dfsl -g s -s none -c file -u ./data/reference/rectangle/map5x6-double-dfsl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t double -a dfsl -g se -s none -c file -u ./data/reference/rectangle/map5x6-double-dfsl_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t double -a dfsl -g seb -s none -c file -u ./data/reference/rectangle/map5x6-double-dfsl_seb.in


python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfslb -g s -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfslb_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfslb -g se -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfslb_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfslb -g seb -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfslb_seb.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a dfslb -g reb -s none -c file -u ./data/reference/rectangle/map5x6-single_y-dfslb_reb.in


python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a sb -g s -s none -c file -u ./data/reference/rectangle/map5x6-single_y-sb_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f n -a sb -g s -s none -c file -u ./data/reference/rectangle/map5x6-single_n-sb_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t double -a sb -g s -s none -c file -u ./data/reference/rectangle/map5x6-double-sb_s.in

python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a sbl -g s -s none -c file -u ./data/reference/rectangle/map5x6-single_y-sbl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f n -a sbl -g s -s none -c file -u ./data/reference/rectangle/map5x6-single_n-sbl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t double -a sbl -g s -s none -c file -u ./data/reference/rectangle/map5x6-double-sbl_s.in

python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a sb -g r -s none -c file -u ./data/reference/rectangle/map5x6-single_y-sb_r.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -t single -f y -a sb -g rb -s none -c file -u ./data/reference/rectangle/map5x6-single_y-sb_rb.in

echo "Some holes"

python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a dfs -g s -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a dfs -g se -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a dfs -g seb -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-dfs_seb.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a dfs -g reb -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-dfs_reb.in

python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f n -a dfs -g s -s none -c file -u ./data/reference/someHoles/map7x9_2-single_n-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f n -a dfs -g se -s none -c file -u ./data/reference/someHoles/map7x9_2-single_n-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f n -a dfs -g seb -s none -c file -u ./data/reference/someHoles/map7x9_2-single_n-dfs_seb.in

python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t double -a dfs -g s -s none -c file -u ./data/reference/someHoles/map7x9_2-double-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t double -a dfs -g se -s none -c file -u ./data/reference/someHoles/map7x9_2-double-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t double -a dfs -g seb -s none -c file -u ./data/reference/someHoles/map7x9_2-double-dfs_seb.in

python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a dfsl -g s -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-dfsl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a dfsl -g se -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-dfsl_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a dfsl -g seb -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-dfsl_seb.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a dfsl -g reb -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-dfsl_reb.in

python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t double -a dfsl -g s -s none -c file -u ./data/reference/someHoles/map7x9_2-double-dfsl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t double -a dfsl -g se -s none -c file -u ./data/reference/someHoles/map7x9_2-double-dfsl_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t double -a dfsl -g seb -s none -c file -u ./data/reference/someHoles/map7x9_2-double-dfsl_seb.in

python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a sb -g s -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-sb_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f n -a sb -g s -s none -c file -u ./data/reference/someHoles/map7x9_2-single_n-sb_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t double -a sb -g s -s none -c file -u ./data/reference/someHoles/map7x9_2-double-sb_s.in

python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a sbl -g s -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-sbl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f n -a sbl -g s -s none -c file -u ./data/reference/someHoles/map7x9_2-single_n-sbl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t double -a sbl -g s -s none -c file -u ./data/reference/someHoles/map7x9_2-double-sbl_s.in

python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a sb -g r -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-sb_r.in
python ../../tools/padWalking/main.py -p ./testData/maps/someHoles/map7x9_2.txt -x 1 -y 2 -t single -f y -a sb -g rb -s none -c file -u ./data/reference/someHoles/map7x9_2-single_y-sb_rb.in

echo "Single hole"

python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a dfs -g s -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a dfs -g se -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a dfs -g seb -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-dfs_seb.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a dfs -g reb -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-dfs_reb.in

python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f n -a dfs -g s -s none -c file -u ./data/reference/singleHole/map5x6_2-single_n-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f n -a dfs -g se -s none -c file -u ./data/reference/singleHole/map5x6_2-single_n-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f n -a dfs -g seb -s none -c file -u ./data/reference/singleHole/map5x6_2-single_n-dfs_seb.in

python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t double -a dfs -g s -s none -c file -u ./data/reference/singleHole/map5x6_2-double-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t double -a dfs -g se -s none -c file -u ./data/reference/singleHole/map5x6_2-double-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t double -a dfs -g seb -s none -c file -u ./data/reference/singleHole/map5x6_2-double-dfs_seb.in

python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a dfsl -g s -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-dfsl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a dfsl -g se -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-dfsl_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a dfsl -g seb -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-dfsl_seb.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a dfsl -g reb -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-dfsl_reb.in

python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t double -a dfsl -g s -s none -c file -u ./data/reference/singleHole/map5x6_2-double-dfsl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t double -a dfsl -g se -s none -c file -u ./data/reference/singleHole/map5x6_2-double-dfsl_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t double -a dfsl -g seb -s none -c file -u ./data/reference/singleHole/map5x6_2-double-dfsl_seb.in

python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a sb -g s -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-sb_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f n -a sb -g s -s none -c file -u ./data/reference/singleHole/map5x6_2-single_n-sb_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t double -a sb -g s -s none -c file -u ./data/reference/singleHole/map5x6_2-double-sb_s.in

python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a sbl -g s -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-sbl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f n -a sbl -g s -s none -c file -u ./data/reference/singleHole/map5x6_2-single_n-sbl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t double -a sbl -g s -s none -c file -u ./data/reference/singleHole/map5x6_2-double-sbl_s.in

python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a sb -g r -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-sb_r.in
python ../../tools/padWalking/main.py -p ./testData/maps/singleHole/map5x6_2.txt -x 1 -y 2 -t single -f y -a sb -g rb -s none -c file -u ./data/reference/singleHole/map5x6_2-single_y-sb_rb.in


echo "Many holes"

python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a dfs -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a dfs -g se -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a dfs -g seb -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-dfs_seb.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a dfs -g reb -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-dfs_reb.in

python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f n -a dfs -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_n-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f n -a dfs -g se -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_n-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f n -a dfs -g seb -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_n-dfs_seb.in

python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t double -a dfs -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-dfs_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t double -a dfs -g se -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-dfs_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t double -a dfs -g seb -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-dfs_seb.in

python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a dfsl -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-dfsl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a dfsl -g se -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-dfsl_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a dfsl -g seb -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-dfsl_seb.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a dfsl -g reb -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-dfsl_reb.in

python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t double -a dfsl -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-dfsl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t double -a dfsl -g se -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-dfsl_se.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t double -a dfsl -g seb -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-dfsl_seb.in

python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a sb -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-sb_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f n -a sb -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_n-sb_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t double -a sb -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-sb_s.in

python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a sbl -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-sbl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f n -a sbl -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_n-sbl_s.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t double -a sbl -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-sbl_s.in

python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a sb -g r -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-sb_r.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f y -a sb -g rb -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_y-sb_rb.in


# new ori
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -o E -t double -a sb -g r -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-sb_r-E.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -o E -t double -a sbl -g s -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-sbl_s-E.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -o E -t single -f y -a direct -s none -c file -u ./data/reference/rectangle/map5x6-single_y-direct-E.in
python ../../tools/padWalking/main.py -p ./testData/maps/rectangle/map5x6.txt -x 1 -y 2 -o E -t double -a zigzag -s none -c file -u ./data/reference/rectangle/map5x6-double-zigzag-E.in


python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f n -a sb -g rb -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_n-sb_rb.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t double -a sb -g rb -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-sb_rb.in


python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t single -f n -a sbl -g rb -s none -c file -u ./data/reference/manyHoles/map7x9_3-single_n-sbl_rb.in
python ../../tools/padWalking/main.py -p ./testData/maps/manyHoles/map7x9_3.txt -x 1 -y 2 -t double -a sbl -g rb -s none -c file -u ./data/reference/manyHoles/map7x9_3-double-sbl_rb.in
