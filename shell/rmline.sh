##rm 行首是字符串是2的行
sed '/^2/'d test.txt > image.txt
##删除每行的第一个字符串
sed 's/ .//' image.txt > image_1.txt