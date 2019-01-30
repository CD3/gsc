ls
cat data.txt
head data.txt
sort -k2 -g data.txt | tail -n1
gnuplot
plot 'data.txt'
exit
cat data.txt | gawk '{print $1,$2*$2}' > data2.txt
ls
gnuplot
plot 'data.txt', 'data2.txt'
exit
