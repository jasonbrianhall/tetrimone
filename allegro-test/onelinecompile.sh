docker run --rm -v $(pwd):/src:z -u $(id -u):$(id -g) djfdyuruiry/djgpp /bin/sh -c "\
mkdir -p /src/build/dos/obj && \
g++ -c /src/tetrimone.cpp -I/src/build/dos/source-install/include -o /src/build/dos/obj/tetrimone.o -O2 -fpermissive -w && \
g++ /src/build/dos/obj/*.o -lalleg -lm -s -O6 -L/src/build/dos/source-install/lib -o /src/build/dos/tetrimone.exe && \
exe2coff /src/build/dos/tetrimone.exe && \
cat /src/build/dos/csdpmi/bin/CWSDSTUB.EXE /src/build/dos/tetrimone > /src/build/dos/tetrimone.exe"

