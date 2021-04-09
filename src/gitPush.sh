time=$(date +"%D%T")
message="KernelModule.c Update ${time}"

git add KernelModule.c

git commit -m "${message}"

git push origin master
