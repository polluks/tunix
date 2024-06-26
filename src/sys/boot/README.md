# Roadmap for v3 (TUNIX kernel)

Many UltiMem user have a boot menu
and a proprietary ROM filesystem
already installed on their UltiMem.
These have to remain virtually
unaffected by installing this
bootloader.  That's why the first 64K
of Flash ROM are split into smaller 8K
boot blocks that can be updated quickly,
so existing boot loaders can be moved
away from the first segment which the
KERNAL invokes on system start and
reset.

The idea is to select the formerly
installed boot loader by pressing the
unused UltiMem button on reset.

Settings could be stored in a journal on
the eigth boot block so more functions
could be included conveniently.

# (old plan) Ultimate Ultiboot

[ ] Can run as PRG with any confi-
    guration of the UltiMem.
[ ] Can make and burn images.
[ ] Can install itself as one of the
    boot ROMs.

[ ] Fast memory config selection.
[ ] Alternative boot ROMs.
[ ] Launch from disk or Ultifs.
[ ] Autostart.
[ ] Autostart device preference.
[ ] Memorize launch for next reset or
    hotkey.
