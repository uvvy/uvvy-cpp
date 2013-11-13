set term png font "Arial"

set title "Packet sizes"
set ylabel "Packet size, bytes"
set xlabel "Sequence number"
set output "pktsizes.png"
plot "local_packetsizes.plot" using 1:2 title "Orig local size" with dots, "remote_packetsizes.plot" using 1:2 title "Orig remote size" with dots
