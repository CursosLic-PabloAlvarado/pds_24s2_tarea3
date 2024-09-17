# Ejemplo base de Jack en C++

Este ejemplo construye una aplicación muy sencilla de "pass-through"
usando Jack, como punto de partida para los proyectos y tareas del
curso.

Esta versión permite recibir una lista de archivos .wav, que se ejecutan
uno tras otro, reemplazando la entrada de micrófono en tanto hayan datos
de los archivos disponibles.  Una vez que todos los archivos terminan de
ejecutarse, regresa al modo "pass-through".

## Dependencias

Requiere C++ en su estándar del 2020 (g++ 12, clang 14).

En derivados de debian (ubuntu, etc):

     sudo apt install jackd2 libjack-jackd2-dev qjackctl build-essential meson ninja-build jack-tools libsndfile1-dev libsndfile1 libboost-all-dev 
     
Jack requiere que su usuario pertenezca al grupo audio, o de otro modo
no tendrá privilegios para el procesamiento demandante en tiempo
real...

     sudo usermod -aG audio <su usuario>

## Construcción

Para construir los ejemplos la primera vez utilice

     meson setup builddir
     cd builddir
     ninja


Si requiere reconstruir todo, utilice

     meson setup --wipe builddir
     cd builddir
     ninja

o si solo requiere reconfigurar por haber agregado otro archivo:

    meson --reconfigure builddir

## Latencia y tamaño de bloque

Para reducir la latencia por medio del tamaño del "periodo" (esto es,
el número de "frames" que cada ciclo de procesamiento recibe, en
QjackCtl, en Settings, se indica en Frames/Period.  Eso es un
parámetro del servidor de Jack y no lo puede controlar la aplicación
como tal.
