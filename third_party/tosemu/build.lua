musashilib {
	m68kconf = "third_party/tosemu/m68kconf.h",
	outs = { "$OBJ/third_party/tosemu/libmusashi.a" }
}

cprogram {
	ins = {
		"third_party/tosemu/bios.c",
		"third_party/tosemu/bios.h",
		"third_party/tosemu/config.h",
		"third_party/tosemu/cpu.c",
		"third_party/tosemu/cpu.h",
		"third_party/tosemu/gemdos.c",
		"third_party/tosemu/gemdoscon.c",
		"third_party/tosemu/gemdoscon_p.h",
		"third_party/tosemu/gemdosfile.c",
		"third_party/tosemu/gemdosfile_p.h",
		"third_party/tosemu/gemdos.h",
		"third_party/tosemu/gemdosmem.c",
		"third_party/tosemu/gemdosmem_p.h",
		"third_party/tosemu/gemdos_p.h",
		"third_party/tosemu/m68kconf.h",
		"third_party/tosemu/main.c",
		"third_party/tosemu/memory.c",
		"third_party/tosemu/memory.h",
		"third_party/tosemu/tossystem.c",
		"third_party/tosemu/tossystem.h",
		"third_party/tosemu/utils.c",
		"third_party/tosemu/utils.h",
		"third_party/tosemu/xbios.c",
		"third_party/tosemu/xbios.h",
		"third_party/musashi/m68k.h",
		"$OBJ/third_party/tosemu/libmusashi.a",
	},
	outs = { "bin/tosemu" },
}

