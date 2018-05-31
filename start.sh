#自动根据git提交的数量更改版本号
#version=`git log --pretty=oneline | wc -l | awk '{printf("%03d\n",$0 + 100)}'`
#sed -i '91s/.*/#define EYE_VERSION MAIN_VERSION".0'$version'"/' porting/include/jv_common.h

echo "1. ctags -R --fields=+lS"
ctags -R
echo "2. cscope -Rbq"
cscope -Rbq
echo "3. vim ."
vim .
