cc blinkenLights2.c `pkg-config gtk+-3.0 --cflags --libs` -I ../simh/ -DLOCKLESS -DM_SHARED shm.o -o blinkenLights2


