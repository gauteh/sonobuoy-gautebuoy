v 20110115 2
C 40000 40000 0 0 0 title-B.sym
C 40800 48600 1 0 0 connector5-1.sym
{
T 42600 50100 5 10 0 0 0 0 1
device=CONNECTOR_5
T 40900 50300 5 10 1 1 0 0 1
refdes=CONN?
}
C 47000 46200 1 0 0 opamp-2.sym
{
T 47800 47200 5 10 0 0 0 0 1
device=OPAMP
T 47800 47000 5 10 1 1 0 0 1
refdes=LM833N
T 47800 47400 5 10 0 0 0 0 1
symversion=0.1
}
N 48100 46700 51300 46700 4
N 48400 46700 48400 45900 4
N 46500 45900 48400 45900 4
N 46500 45900 46500 46400 4
N 46500 46400 47000 46400 4
C 41700 47700 1 0 0 terminal-1.sym
{
T 42010 48450 5 10 0 0 0 0 1
device=terminal
T 42010 48300 5 10 0 0 0 0 1
footprint=CONNECTOR 1 1
T 41950 47750 5 10 1 1 0 6 1
refdes=Hydrophone (V+)
}
C 41700 44000 1 0 0 terminal-1.sym
{
T 42010 44750 5 10 0 0 0 0 1
device=terminal
T 42010 44600 5 10 0 0 0 0 1
footprint=CONNECTOR 1 1
T 41950 44050 5 10 1 1 0 6 1
refdes=Hydrophone (signal)
}
C 41700 45200 1 0 0 terminal-1.sym
{
T 42010 45950 5 10 0 0 0 0 1
device=terminal
T 42010 45800 5 10 0 0 0 0 1
footprint=CONNECTOR 1 1
T 41950 45250 5 10 1 1 0 6 1
refdes=Hydrophone (Common)
}
N 42500 48800 43400 48800 4
N 43400 48800 43400 47800 4
N 43400 47800 42600 47800 4
N 42500 49400 44200 49400 4
N 44200 49400 44200 45300 4
N 42600 45300 46300 45300 4
C 42800 47800 1 0 0 12V-plus-1.sym
C 43300 45000 1 0 0 gnd-1.sym
N 42500 49100 47500 49100 4
N 47500 49100 47500 47200 4
C 47300 49100 1 0 0 5V-plus-1.sym
N 42500 49700 49100 49700 4
N 49100 49700 49100 45400 4
N 49100 45400 47500 45400 4
N 47500 45400 47500 46200 4
C 48900 49700 1 0 0 5V-minus-1.sym
C 43200 47800 1 270 0 capacitor-2.sym
{
T 43900 47600 5 10 0 0 270 0 1
device=POLARIZED_CAPACITOR
T 43700 47600 5 10 1 1 270 0 1
refdes=100uF
T 44100 47600 5 10 0 0 270 0 1
symversion=0.1
}
N 43400 46900 43400 45300 4
C 43300 43900 1 0 0 capacitor-2.sym
{
T 43500 44600 5 10 0 0 0 0 1
device=POLARIZED_CAPACITOR
T 43500 44400 5 10 1 1 0 0 1
refdes=47uF
T 43500 44800 5 10 0 0 0 0 1
symversion=0.1
}
N 42600 44100 43300 44100 4
C 44200 44000 1 0 0 resistor-2.sym
{
T 44600 44350 5 10 0 0 0 0 1
device=RESISTOR
T 44400 44300 5 10 1 1 0 0 1
refdes=58.33kOhm
}
C 45500 44100 1 270 0 resistor-2.sym
{
T 45850 43700 5 10 0 0 270 0 1
device=RESISTOR
T 45800 43900 5 10 1 1 270 0 1
refdes=41.67kOhm
}
N 45100 44100 45600 44100 4
C 46600 48200 1 0 0 capacitor-1.sym
{
T 46800 48900 5 10 0 0 0 0 1
device=CAPACITOR
T 46800 48700 5 10 1 1 0 0 1
refdes=0.1uF
T 46800 49100 5 10 0 0 0 0 1
symversion=0.1
}
C 46600 47400 1 0 0 capacitor-1.sym
{
T 46800 48100 5 10 0 0 0 0 1
device=CAPACITOR
T 46800 47900 5 10 1 1 0 0 1
refdes=2.2uF
T 46800 48300 5 10 0 0 0 0 1
symversion=0.1
}
C 47700 44500 1 90 0 capacitor-1.sym
{
T 47000 44700 5 10 0 0 90 0 1
device=CAPACITOR
T 47200 44700 5 10 1 1 90 0 1
refdes=0.1uF
T 46800 44700 5 10 0 0 90 0 1
symversion=0.1
}
C 48600 44500 1 90 0 capacitor-1.sym
{
T 47900 44700 5 10 0 0 90 0 1
device=CAPACITOR
T 48100 44700 5 10 1 1 90 0 1
refdes=2.2uF
T 47700 44700 5 10 0 0 90 0 1
symversion=0.1
}
N 45600 44100 45600 47000 4
N 45600 47000 47000 47000 4
N 46600 48400 44200 48400 4
N 46600 47600 44200 47600 4
N 46300 45300 46300 42600 4
N 45600 42600 48400 42600 4
N 45600 42600 45600 43200 4
N 48400 42600 48400 44500 4
N 47500 42600 47500 44500 4
C 46900 42300 1 0 0 gnd-1.sym
C 52200 46800 1 180 0 terminal-1.sym
{
T 51890 46050 5 10 0 0 180 0 1
device=terminal
T 51890 46200 5 10 0 0 180 0 1
footprint=CONNECTOR 1 1
T 51950 46750 5 10 1 1 180 6 1
refdes=Output (preamp)
}
C 45800 46400 1 270 0 capacitor-2.sym
{
T 46500 46200 5 10 0 0 270 0 1
device=POLARIZED_CAPACITOR
T 46300 46200 5 10 1 1 270 0 1
refdes=380 pF
T 46700 46200 5 10 0 0 270 0 1
symversion=0.1
}
N 46000 46400 46000 47000 4
N 46000 45500 46000 45300 4
