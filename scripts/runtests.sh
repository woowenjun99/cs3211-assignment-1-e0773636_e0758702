#!/bin/bash

YELLOW='\033[0;33m'
RESET='\033[0m'

for ((i=1; i<=20; i++)); do
  echo "Iteration: $i"
  echo -e "\n${YELLOW}TEST${RESET} $test_file"
  output=$(../grader ../engine < ./generated.in)
done
