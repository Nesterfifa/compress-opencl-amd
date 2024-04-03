g++ -o A.out A.cpp
g++ -o A.out A.cpp
g++ -o B.out B.cpp
g++ -o C.out C.cpp
g++ -o D.out D.cpp
g++ -o E.out E.cpp
g++ -o F.out F.cpp
g++ -o G.out G.cpp
g++ -o H.out H.cpp
g++ -o I.out I.cpp
g++ -o J.out J.cpp
g++ -o K.out K.cpp
g++ -o L.out L.cpp
g++ -o M.out M.cpp
g++ -o N.out N.cpp
g++ -o O.out O.cpp
g++ -o P.out P.cpp

xz -k --best *.out
gzip -k --best *.out
bzip2 -k --best *.out
zstd -k -19 *.out
lzma -k --best *.out
upx -k --best --ultra-brute *.out
