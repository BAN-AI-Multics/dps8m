cc blinkenLights2.c `pkg-config gtk+-3.0 --cflags --libs` -I../simh/ -I../dps8/ -DLOCKLESS -DM_SHARED ../dps8/shm.o -o blinkenLights2


