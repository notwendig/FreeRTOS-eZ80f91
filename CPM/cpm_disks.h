#ifndef CPM_DISKS_H
#define CPM_DISKS_H

#define CPM_DISK_BASE 0x200000UL
#define CPM_FLOPPY_SIZE 256256UL
#define CPM_HARD_SIZE 4194304UL

static unsigned char cpm_disk_present(unsigned char drive)
{
    return drive < 4 || drive == 8 || drive == 9;
}

/* Physical sectors are one-based; BIOS applies floppy skew before this call.
 * Cast before multiplying: the target has 16-bit int and 24-bit size_t. */
static unsigned char cpm_disk_sector(unsigned char drive, unsigned short track,
                                    unsigned short sector, size_t *offset)
{
    size_t base;
    unsigned short tracks, sectors;
    if(drive < 4) {
        base = (size_t)drive * CPM_FLOPPY_SIZE;
        tracks = 77;
        sectors = 26;
    } else if(drive == 8 || drive == 9) {
        base = 4 * CPM_FLOPPY_SIZE + (size_t)(drive - 8) * CPM_HARD_SIZE;
        tracks = 1024;
        sectors = 32;
    } else {
        return 0;
    }
    if(track >= tracks || !sector || sector > sectors)
        return 0;
    *offset = base + ((size_t)track * sectors + sector - 1) * 128;
    return 1;
}

#endif
