#!/bin/bash
for filename in data/planner-tests/*-init.in; do
  #echo "$filename"
  filename2="${filename%*-init.in}"
  filename3="${filename2##*/}"
  init="$filename"
  goal="data/planner-tests/$filename3-goal.in"
  res="data/planner-results/$filename3-rrt-res.out"
  echo "$filename3"
  time timeout 30 cmake-build-debug/./rofi-reconfig -i "$init" -g "$goal" -a rrt > "$res"
done