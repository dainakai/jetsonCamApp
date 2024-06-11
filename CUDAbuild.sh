mkdir -p ./obj
nvcc -c ./src/cuda_functions.cu -o ./obj/cuda_functions.o --extended-lambda
ar rcs ./obj/libcuda_functions.a ./obj/cuda_functions.o
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib64