v 20110115 2
C 40000 40000 0 0 0 title-B.sym
C 46100 48400 1 270 0 capacitor-2.sym
{
T 46800 48200 5 10 0 0 270 0 1
device=POLARIZED_CAPACITOR
T 46600 48200 5 10 1 1 270 0 1
refdes=100 uF
T 47000 48200 5 10 0 0 270 0 1
symversion=0.1
}
C 48900 46100 1 90 0 resistor-2.sym
{
T 48550 46500 5 10 0 0 90 0 1
device=RESISTOR
T 48600 46300 5 10 1 1 90 0 1
refdes=100k
}
C 50600 48700 1 180 0 terminal-1.sym
{
T 50290 47950 5 10 0 0 180 0 1
device=terminal
T 50290 48100 5 10 0 0 180 0 1
footprint=CONNECTOR 1 1
T 50350 48650 5 10 1 1 180 6 1
refdes=Power (+12V)
}
C 50600 47200 1 180 0 terminal-1.sym
{
T 50290 46450 5 10 0 0 180 0 1
device=terminal
T 50290 46600 5 10 0 0 180 0 1
footprint=CONNECTOR 1 1
T 50350 47150 5 10 1 1 180 6 1
refdes=Signal (out)
}
C 50600 45600 1 180 0 terminal-1.sym
{
T 50290 44850 5 10 0 0 180 0 1
device=terminal
T 50290 45000 5 10 0 0 180 0 1
footprint=CONNECTOR 1 1
T 50350 45550 5 10 1 1 180 6 1
refdes=Common
}
C 42500 48500 1 0 0 terminal-1.sym
{
T 42810 49250 5 10 0 0 0 0 1
device=terminal
T 42810 49100 5 10 0 0 0 0 1
footprint=CONNECTOR 1 1
T 42750 48550 5 10 1 1 0 6 1
refdes=Power
}
C 42500 47000 1 0 0 terminal-1.sym
{
T 42810 47750 5 10 0 0 0 0 1
device=terminal
T 42810 47600 5 10 0 0 0 0 1
footprint=CONNECTOR 1 1
T 42750 47050 5 10 1 1 0 6 1
refdes=Signal (In)
}
C 42500 45400 1 0 0 terminal-1.sym
{
T 42810 46150 5 10 0 0 0 0 1
device=terminal
T 42810 46000 5 10 0 0 0 0 1
footprint=CONNECTOR 1 1
T 42750 45450 5 10 1 1 0 6 1
refdes=Common
}
N 43400 48600 49700 48600 4
N 46300 48400 46300 48600 4
N 46300 47500 46300 45500 4
N 43400 45500 49700 45500 4
C 47100 46900 1 0 0 capacitor-2.sym
{
T 47300 47600 5 10 0 0 0 0 1
device=POLARIZED_CAPACITOR
T 47300 47400 5 10 1 1 0 0 1
refdes=47 uF
T 47300 47800 5 10 0 0 0 0 1
symversion=0.1
}
N 43400 47100 47100 47100 4
N 48000 47100 49700 47100 4
N 48800 47100 48800 47000 4
N 48800 46100 48800 45500 4
T 49400 47200 9 10 1 0 0 0 1
+/- 6V
T 43200 47100 9 10 1 0 0 0 1
0 - 12 V
