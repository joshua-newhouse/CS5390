#!/bin/bash
./node 0 400 9 "Test XEF 0 to 9" 1 &
./node 1 400 9 "Test XEF 1 to 9" 0 2 3 &
./node 2 400 6 "Test XEF 2 to 6" 1 3 5 8 &
./node 3 400 2 "Test XEF 3 to 2" 1 2 4 &
./node 4 400 9 "Test XEF 4 to 9" 3 &
./node 5 400 2 "Test XEF 5 to 2" 2 6 7 &
./node 6 400 2 "Test XEF 6 to 2" 5 7 &
./node 7 400 5 "Test XEF 7 to 5" 5 6 8 9 &
./node 8 400 0 "Test XEF 8 to 0" 2 7 &
./node 9 400 0 "Test XEF 9 to 0" 7 &
