set term png font "Arial"

set title "Packet sizes"
set ylabel "Packet size, bytes"
set xlabel "Sequence number"
set output "pktsizes.png"
plot "packetsizes.plot" using 1:2 title "Orig size" with dots, "packetsizes.plot" using 1:3 title "Decoded size" with dots
