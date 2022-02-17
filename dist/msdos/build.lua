if WITH_MSDOS then
	copy {
		ins = { "examples/mandel.cow" },
		outs = { "$OBJ/dist/msdos/mandel.cow" },
	}

	cpmify {
		ins = { "rt/msdos/cowgol.coh" },
		outs = { "$OBJ/dist/msdos/cowgol.coh" },
	}

	cpmify {
		ins = { "rt/common.coh" },
		outs = { "$OBJ/dist/msdos/common.coh" },
	}

	cpmify {
		ins = { "dist/msdos/demo.bat" },
		outs = { "$OBJ/dist/msdos/demo.bat" },
	}

	copy {
		ins = { "$OBJ/rt/msdos/cowgol.coo" },
		outs = { "$OBJ/dist/msdos/cowgol.coo" }
	}

	copy {
		ins = { "bin/cowfe-16bit.msdos.msdos.exe" },
		outs = { "$OBJ/dist/msdos/cowfe.exe" }
	}

	copy {
		ins = { "bin/cowbe-8086.msdos.msdos.exe" },
		outs = { "$OBJ/dist/msdos/cowbe.exe" }
	}

	copy {
		ins = { "bin/cowlink-msdos.msdos.msdos.exe" },
		outs = { "$OBJ/dist/msdos/cowlink.exe" }
	}
end

