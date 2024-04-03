xz -k --best *.bin
gzip -k --best *.bin
bzip2 -k --best *.bin
zstd -k -19 *.bin
lzma -k --best *.bin
upx -k --best --ultra-brute *.bin
