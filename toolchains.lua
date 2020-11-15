toolchain_ncgen = {
	name = "ncgen",
	cowfe = "bin/cowfe-cgen.bootstrap.exe",
	cowbe = "bin/cowbe-cgen.bootstrap.exe",
	linker = "bin/cowlink-cgen.bootstrap.exe",
	assembler = buildcgen,
	runtime = "rt/cgen",
	asmext = ".c",
	binext = ".exe",
	tester = nativetest
}

toolchain_nncgen = {
	name = "nncgen",
	cowbe = "bin/cowbe-cgen.ncgen.exe",
	cowfe = "bin/cowfe-cgen.ncgen.exe",
	linker = "bin/cowlink-cgen.ncgen.exe",
	assembler = buildcgen,
	runtime = "rt/cgen",
	asmext = ".c",
	binext = ".exe",
	tester = nativetest
}

toolchain_ncpm = {
	name = "ncpm",
	cowfe = "bin/cowfe-8080.nncgen.exe",
	cowbe = "bin/cowbe-8080.nncgen.exe",
	linker = "bin/cowlink-8080.nncgen.exe",
	assembler = buildcowasm8080,
	runtime = "rt/cpm",
	asmext = ".asm",
	binext = ".8080.com",
	tester = cpmtest,
}

toolchain_ncpmz = {
	name = "ncpmz",
	cowfe = "bin/cowfe-z80.nncgen.exe",
	cowbe = "bin/cowbe-z80.nncgen.exe",
	linker = "bin/cowlink-8080.nncgen.exe",
	assembler = buildzmac,
	runtime = "rt/cpmz",
	asmext = ".z80",
	binext = ".z80.com",
	tester = cpmtest,
}

toolchain_lxthumb2 = {
	name = "lxthumb2",
	cowfe = "bin/cowfe-thumb2.nncgen.exe",
	cowbe = "bin/cowbe-thumb2.nncgen.exe",
	linker = "bin/cowlink-lxthumb2.nncgen.exe",
	assembler = buildgasarm,
	runtime = "rt/lxthumb2",
	asmext = ".s",
	binext = ".lxthumb2.exe",
	tester = qemuarmtest
}

toolchain_lx386 = {
	name = "lx386",
	cowfe = "bin/cowfe-80386.nncgen.exe",
	cowbe = "bin/cowbe-80386.nncgen.exe",
	linker = "bin/cowlink-lx386.nncgen.exe",
	assembler = buildgas386,
	runtime = "rt/lx386",
	asmext = ".s",
	binext = ".lx386.exe",
	tester = qemu386test
}

toolchain_bbct = {
	name = "bbct",
	cowfe = "bin/cowfe-6502.nncgen.exe",
	cowbe = "bin/cowbe-65c02.nncgen.exe",
	linker = "bin/cowlink-bbct.nncgen.exe",
	assembler = buildtass64,
	runtime = "rt/bbct",
	asmext = ".asm",
	binext = ".bbct",
	tester = tubeemutest,
	archs = { "6502i", "8080" }
}

toolchain_bbctiny = {
	name = "bbctiny",
	cowfe = "bin/cowfe-6502.nncgen.exe",
	cowbe = "bin/cowbe-65c02-tiny.nncgen.exe",
	linker = "bin/cowlink-bbct.nncgen.exe",
	assembler = buildtass64,
	runtime = "rt/bbct",
	asmext = ".asm",
	binext = ".bbctiny",
	tester = tubeemutest,
}

toolchain_bbct6502 = {
	name = "bbct6502",
	cowfe = "bin/cowfe-6502.nncgen.exe",
	cowbe = "bin/cowbe-6502.nncgen.exe",
	linker = "bin/cowlink-bbct.nncgen.exe",
	assembler = buildtass64,
	runtime = "rt/bbct",
	asmext = ".asm",
	binext = ".bbct6502",
	tester = tubeemutest,
	archs = { "6502i", "8080" },
}

toolchain_bbcti = {
	name = "bbcti",
	cowfe = "bin/cowfe-6502i.nncgen.exe",
	cowbe = "bin/cowbe-6502i.nncgen.exe",
	linker = "bin/cowlink-bbcti.nncgen.exe",
	assembler = buildtass64,
	runtime = "rt/bbcti",
	asmext = ".asm",
	binext = ".bbcti",
	tester = tubeemutest,
}

toolchain_unixv7 = {
	name = "unixv7",
	cowfe = "bin/cowfe-pdp11.nncgen.exe",
	cowbe = "bin/cowbe-pdp11.nncgen.exe",
	linker = "bin/cowlink-v7unix.nncgen.exe",
	assembler = buildcowasmpdp11,
	runtime = "rt/unixv7",
	asmext = ".asm",
	binext = ".exe",
	tester = apouttest
}

toolchain_fuzix6303 = {
	name = "fuzix6303",
	cowfe = "bin/cowfe-6303.nncgen.exe",
	cowbe = "bin/cowbe-6303.nncgen.exe",
	linker = "bin/cowlink-fuzix6303.nncgen.exe",
	assembler = buildcowasm6303,
	runtime = "rt/fuzix6303",
	asmext = ".asm",
	binext = ".6303.exe",
	tester = fuzix6303test,
}

toolchain_msdos = {
	name = "msdos",
	cowfe = "bin/cowfe-8086.nncgen.exe",
	cowbe = "bin/cowbe-8086.nncgen.exe",
	linker = "bin/cowlink-msdos.nncgen.exe",
	assembler = buildnasm,
	runtime = "rt/msdos",
	asmext = ".asm",
	binext = ".msdos.com",
	tester = emu2test,
}

toolchain_tlcs90 = {
	name = "tlcs90",
	cowfe = "bin/cowfe-tlcs90.nncgen.exe",
	cowbe = "bin/cowbe-tlcs90.nncgen.exe",
	linker = "bin/cowlink-tlcs90.nncgen.exe",
	assembler = nil,
	runtime = "rt/tlcs90",
	asmext = ".asm",
	binext = ".msdos.com",
	tester = nil,
}

ALL_TOOLCHAINS = {
	toolchain_bbct,
	toolchain_bbct6502,
	toolchain_bbcti,
	toolchain_bbctiny,
	toolchain_fuzix6303,
	toolchain_lxthumb2,
	toolchain_lx386,
	toolchain_msdos,
	toolchain_ncgen,
	toolchain_ncpm,
	toolchain_ncpmz,
	toolchain_nncgen,
	toolchain_unixv7,
}

