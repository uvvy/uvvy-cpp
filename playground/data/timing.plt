set term svg fname "Times Roman"

set title "Delay variation"
set ylabel "Packet delay, ms"
set xlabel "Time"
set output "plot-delay.svg"
plot "packet-to-receive-remote.dat" using 1:3 title "RCV remote" with lines, "packet-to-receive-local.dat" using 1:3 title "RCV local" with lines

set output "plot-delay-play.svg"
plot "packet-to-playback-remote.dat" using 1:3 title "PLAY remote" with lines, "packet-to-playback-local.dat" using 1:3 title "PLAY local" with lines
