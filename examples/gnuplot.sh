# # it even works with command line interfaces!
mkdir -p tmp
cd tmp
gnuplot
plot sin(x)
plot cos(x)
set xlabel "time (s)"
set ylabel "displacement (cm)"
rep
set term png
set output "waves.png"
plot sin(x), cos(x)
set term x11
