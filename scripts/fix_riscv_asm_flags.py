# PlatformIO pre-build script: fix RISC-V assembly ABI flags
#
# LVGL 9.5 ships ARM Helium blend assembly (src/draw/sw/blend/helium/*.S).
# PlatformIO compiles these .S files on ESP32-P4 (RISC-V) with a generic
# -march=rv32imc and no -mabi, producing objects with a different float ABI
# than the ilp32f application ("can't link soft-float modules with
# single-float modules"). The Helium code itself preprocesses to nothing
# because LV_USE_DRAW_SW_ASM is LV_DRAW_SW_ASM_NONE in lv_conf.h, so simply
# making the ABI flags consistent fixes the link.
Import("env")

mcu = env.BoardConfig().get("build.mcu", "")
if mcu == "esp32p4":
    MARCH = "-march=rv32imafc_zicsr_zifencei_xesppie"
    MABI = "-mabi=ilp32f"

    def fix(key):
        flags = env.get(key, [])
        if isinstance(flags, str):
            flags = flags.split()
        flags = [f for f in flags
                 if not f.startswith("-march=") and not f.startswith("-mabi=")]
        flags += [MARCH, MABI]
        env.Replace(**{key: flags})

    for k in ("ASFLAGS", "ASPPFLAGS", "ASFLAGS_END", "ASPPFLAGS_END"):
        try:
            fix(k)
        except Exception:
            pass
    print("fix_riscv_asm_flags: ASFLAGS patched for %s" % mcu)
