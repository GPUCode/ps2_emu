mips64r5900el-ps2-elf-gcc -D_EE -G0 -O2 -Wall  -I$PS2SDK/ee/include -I$PS2SDK/common/include -I.  -c ${1}.c -o ${1}.o
mips64r5900el-ps2-elf-gcc -T$PS2SDK/ee/startup/linkfile -O2 -o ${1}.elf ${1}.o -L$PS2SDK/ee/lib -Wl,-zmax-page-size=128   -ldebug
mips64r5900el-ps2-elf-strip --strip-all ${1}.elf