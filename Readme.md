1.just compile the files with the following command clang -Xpreprocessor -fopenmp cmp.c -o cmp  -lomp -I$(brew --prefix libomp)/include -L$(brew --prefix libomp)/lib
2.Execute the file with the following command : ./cmp  largegraph.txt 0  
3.  just check the screenshot I attached in the report for the further execution