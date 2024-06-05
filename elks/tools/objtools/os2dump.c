/* 28 May 24 from https://github.com/icculus/2ine */
/**
 * 2ine; an OS/2 emulator for Linux.
 *
 *  This file written by Ryan C. Gordon.
 *
 * Copyright (c) 2016-2022 Ryan C. Gordon <icculus@icculus.org>.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t sint8;
typedef int16_t sint16;
typedef int32_t sint32;
typedef int64_t sint64;

typedef unsigned int uint;

#pragma pack(push, 1)
typedef struct LxHeader
{
    uint8 magic_l;
    uint8 magic_x;
    uint8 byte_order;
    uint8 word_order;
    uint32 lx_version;
    uint16 cpu_type;
    uint16 os_type;
    uint32 module_version;
    uint32 module_flags;
    uint32 module_num_pages;
    uint32 eip_object;
    uint32 eip;
    uint32 esp_object;
    uint32 esp;
    uint32 page_size;
    uint32 page_offset_shift;
    uint32 fixup_section_size;
    uint32 fixup_section_checksum;
    uint32 loader_section_size;
    uint32 loader_section_checksum;
    uint32 object_table_offset;
    uint32 module_num_objects;
    uint32 object_page_table_offset;
    uint32 object_iter_pages_offset;
    uint32 resource_table_offset;
    uint32 num_resource_table_entries;
    uint32 resident_name_table_offset;
    uint32 entry_table_offset;
    uint32 module_directives_offset;
    uint32 num_module_directives;
    uint32 fixup_page_table_offset;
    uint32 fixup_record_table_offset;
    uint32 import_module_table_offset;
    uint32 num_import_mod_entries;
    uint32 import_proc_table_offset;
    uint32 per_page_checksum_offset;
    uint32 data_pages_offset;
    uint32 num_preload_pages;
    uint32 non_resident_name_table_offset;
    uint32 non_resident_name_table_len;
    uint32 non_resident_name_table_checksum;
    uint32 auto_ds_object_num;
    uint32 debug_info_offset;
    uint32 debug_info_len;
    uint32 num_instance_preload;
    uint32 num_instance_demand;
    uint32 heapsize;
} LxHeader;

typedef struct LxObjectTableEntry
{
    uint32 virtual_size;
    uint32 reloc_base_addr;
    uint32 object_flags;
    uint32 page_table_index;
    uint32 num_page_table_entries;
    uint32 reserved;
} LxObjectTableEntry;

typedef struct LxObjectPageTableEntry
{
    uint32 page_data_offset;
    uint16 data_size;
    uint16 flags;
} LxObjectPageTableEntry;

typedef struct LxResourceTableEntry
{
    uint16 type_id;
    uint16 name_id;
    uint32 resource_size;
    uint16 object;
    uint32 offset;
} LxResourceTableEntry;

typedef struct NeHeader
{
    uint8 magic_n;
    uint8 magic_e;
    uint8 linker_version;
    uint8 linker_revision;
    uint16 entry_table_offset;
    uint16 entry_table_size;
    uint32 crc32;
    uint16 module_flags;
    uint16 auto_data_segment;
    uint16 dynamic_heap_size;
    uint16 stack_size;
    uint16 reg_ip;
    uint16 reg_cs;
    uint16 reg_sp;
    uint16 reg_ss;
    uint16 num_segment_table_entries;
    uint16 num_module_ref_table_entries;
    uint16 non_resident_name_table_size;
    uint16 segment_table_offset;
    uint16 resource_table_offset;
    uint16 resident_name_table_offset;
    uint16 module_reference_table_offset;
    uint16 imported_names_table_offset;
    uint32 non_resident_name_table_offset;
    uint16 num_movable_entries;
    uint16 sector_alignment_shift_count;
    uint16 num_resource_entries;
    uint8 exe_type;
    uint8 os2_exe_flags;
    uint8 reserved[8];
} NeHeader;

typedef struct NeSegmentTableEntry
{
    uint16 offset;
    uint16 size;
    uint16 segment_flags;
    uint16 minimum_allocation;
} NeSegmentTableEntry;

#pragma pack(pop)

// !!! FIXME: some cut-and-paste with lx_loader.c ...

static int sanityCheckLxExe(const uint8 *exe, const uint32 exelen)
{
    if (sizeof (LxHeader) >= exelen) {
        fprintf(stderr, "not an OS/2 EXE\n");
        return 0;
    }

    const LxHeader *lx = (const LxHeader *) exe;
    if ((lx->byte_order != 0) || (lx->word_order != 0)) {
        fprintf(stderr, "Program is not little-endian!\n");
        return 0;
    }

    if (lx->lx_version != 0) {
        fprintf(stderr, "Program is unknown LX EXE version (%u)\n", (uint) lx->lx_version);
        return 0;
    }

    if (lx->cpu_type > 3) { // 1==286, 2==386, 3==486
        fprintf(stderr, "Program needs unknown CPU type (%u)\n", (uint) lx->cpu_type);
        return 0;
    }

    if (lx->os_type != 1) { // 1==OS/2, others: dos4, windows, win386, unknown.
        fprintf(stderr, "Program needs unknown OS type (%u)\n", (uint) lx->os_type);
        return 0;
    }

    if (lx->page_size != 4096) {
        fprintf(stderr, "Program page size isn't 4096 (%u)\n", (uint) lx->page_size);
        return 0;
    }

    return 1;
} // sanityCheckLxExe

static int sanityCheckNeExe(const uint8 *exe, const uint32 exelen)
{
    if (sizeof (NeHeader) >= exelen) {
        fprintf(stderr, "not an OS/2 EXE\n");
        return 0;
    }

    const NeHeader *ne = (const NeHeader *) exe;
    if (ne->exe_type > 1) {
        fprintf(stderr, "Not an OS/2 NE EXE file (exe_type is %d, not 1)\n", (int) ne->exe_type);
        return 0;
    }

    return 1;
} // sanityCheckNeExe

static int sanityCheckExe(uint8 **_exe, uint32 *_exelen, int *_is_lx)
{
    if (*_exelen < 62) {
        fprintf(stderr, "not an OS/2 EXE\n");
        return 0;
    }
    const uint32 header_offset = *((uint32 *) (*_exe + 0x3C));
    //printf("header offset is %u\n", (uint) header_offset);

    *_exe += header_offset;  // skip the DOS stub, etc.
    *_exelen -= header_offset;

    const uint8 *magic = *_exe;

    if ((magic[0] == 'L') && (magic[1] == 'X')) {
        *_is_lx = 1;
        return sanityCheckLxExe(*_exe, *_exelen);
    } else if ((magic[0] == 'N') && (magic[1] == 'E')) {
        *_is_lx = 0;
        return sanityCheckNeExe(*_exe, *_exelen);
    }

    fprintf(stderr, "not an OS/2 EXE\n");
    return 0;
} // sanityCheckExe

static int parseLxExe(const uint8 *origexe, const uint8 *exe)
{
    const LxHeader *lx = (const LxHeader *) exe;
    printf("LX (32-bit) executable.\n");
    printf("module version: %u\n", (uint) lx->module_version);

    printf("module flags:");
    if (lx->module_flags & 0x4) printf(" LIBINIT");
    if (lx->module_flags & 0x10) printf(" INTERNALFIXUPS");
    if (lx->module_flags & 0x20) printf(" EXTERNALFIXUPS");
    if (lx->module_flags & 0x100) printf(" PMINCOMPAT");
    if (lx->module_flags & 0x200) printf(" PMCOMPAT");
    if (lx->module_flags & 0x300) printf(" USESPM");
    if (lx->module_flags & 0x2000) printf(" NOTLOADABLE");
    if (lx->module_flags & 0x8000) printf(" LIBRARYMODULE");
    if (lx->module_flags & 0x18000) printf(" PROTMEMLIBRARYMODULE");
    if (lx->module_flags & 0x20000) printf(" PHYSDRIVERMODULE");
    if (lx->module_flags & 0x28000) printf(" VIRTDRIVERMODULE");
    if (lx->module_flags & 0x40000000) printf(" LIBTERM");
    printf("\n");

    printf("Number of pages in module: %u\n", (uint) lx->module_num_pages);
    printf("EIP Object number: %u\n", (uint) lx->eip_object);
    printf("EIP: 0x%X\n", (uint) lx->eip);
    printf("ESP Object number: %u\n", (uint) lx->esp_object);
    printf("ESP: 0x%X\n", (uint) lx->esp);
    printf("Page size: %u\n", (uint) lx->page_size);
    printf("Page offset shift: %u\n", (uint) lx->page_offset_shift);
    printf("Fixup section size: %u\n", (uint) lx->fixup_section_size);
    printf("Fixup section checksum: 0x%X\n", (uint) lx->fixup_section_checksum);
    printf("Loader section size: %u\n", (uint) lx->loader_section_size);
    printf("Loader section checksum: 0x%X\n", (uint) lx->loader_section_checksum);
    printf("Object table offset: %u\n", (uint) lx->object_table_offset);
    printf("Number of objects in module: %u\n", (uint) lx->module_num_objects);
    printf("Object page table offset: %u\n", (uint) lx->object_page_table_offset);
    printf("Object iterated pages offset: %u\n", (uint) lx->object_iter_pages_offset);
    printf("Resource table offset: %u\n", (uint) lx->resource_table_offset);
    printf("Number of resource table entries: %u\n", (uint) lx->num_resource_table_entries);
    printf("Resident name table offset: %u\n", (uint) lx->resident_name_table_offset);
    printf("Entry table offset: %u\n", (uint) lx->entry_table_offset);
    printf("Module directives offset: %u\n", (uint) lx->module_directives_offset);
    printf("Number of module directives: %u\n", (uint) lx->num_module_directives);
    printf("Fixup page table offset: %u\n", (uint) lx->fixup_page_table_offset);
    printf("Fixup record table offset: %u\n", (uint) lx->fixup_record_table_offset);
    printf("Import module table offset: %u\n", (uint) lx->import_module_table_offset);
    printf("Number of inport module entries: %u\n", (uint) lx->num_import_mod_entries);
    printf("Import procedure name table offset: %u\n", (uint) lx->import_proc_table_offset);
    printf("Per-page checksum offset: %u\n", (uint) lx->per_page_checksum_offset);
    printf("Data pages offset: %u\n", (uint) lx->data_pages_offset);
    printf("Number of preload pages: %u\n", (uint) lx->num_preload_pages);
    printf("Non-resident name table offset: %u\n", (uint) lx->non_resident_name_table_offset);
    printf("Non-resident name table length: %u\n", (uint) lx->non_resident_name_table_len);
    printf("Non-resident name table checksum: 0x%X\n", (uint) lx->non_resident_name_table_checksum);
    printf("Auto data segment object number: %u\n", (uint) lx->auto_ds_object_num);
    printf("Debug info offset: %u\n", (uint) lx->debug_info_offset);
    printf("Debug info length: %u\n", (uint) lx->debug_info_len);
    printf("Number of instance pages in preload section: %u\n", (uint) lx->num_instance_preload);
    printf("Number of instance pages in demand section: %u\n", (uint) lx->num_instance_demand);
    printf("Heap size: %u\n", (uint) lx->heapsize);

    /* This is apparently a requirement as of OS/2 2.0, according to lxexe.txt. */
    if ((lx->object_iter_pages_offset != 0) && (lx->object_iter_pages_offset != lx->data_pages_offset)) {
        fprintf(stderr, "Object iterator pages offset must be 0 or equal to Data pages offset\n");
    }

    // when an LX file says "object" it's probably more like "section" or "segment" ...?
    printf("\n");
    printf("Object table (%u entries):\n", (uint) lx->module_num_objects);
    for (uint32 i = 0; i < lx->module_num_objects; i++) {
        const LxObjectTableEntry *obj = ((const LxObjectTableEntry *) (exe + lx->object_table_offset)) + i;
        printf("Object #%u:\n", (uint) i+1);
        printf("Virtual size: %u\n", (uint) obj->virtual_size);
        printf("Relocation base address: 0x%X\n", (uint) obj->reloc_base_addr);
        printf("Object flags:");
        if (obj->object_flags & 0x1) printf(" READ");
        if (obj->object_flags & 0x2) printf(" WRITE");
        if (obj->object_flags & 0x4) printf(" EXEC");
        if (obj->object_flags & 0x8) printf(" RESOURCE");
        if (obj->object_flags & 0x10) printf(" DISCARD");
        if (obj->object_flags & 0x20) printf(" SHARED");
        if (obj->object_flags & 0x40) printf(" PRELOAD");
        if (obj->object_flags & 0x80) printf(" INVALID");
        if (obj->object_flags & 0x100) printf(" ZEROFILL");
        if (obj->object_flags & 0x200) printf(" RESIDENT");
        if (obj->object_flags & 0x300) printf(" RESIDENT+CONTIG");
        if (obj->object_flags & 0x400) printf(" RESIDENT+LONGLOCK");
        if (obj->object_flags & 0x800) printf(" SYSRESERVED");
        if (obj->object_flags & 0x1000) printf(" 16:16");
        if (obj->object_flags & 0x2000) printf(" BIG");
        if (obj->object_flags & 0x4000) printf(" CONFORM");
        if (obj->object_flags & 0x8000) printf(" IOPL");
        printf("\n");
        printf("Page table index: %u\n", (uint) obj->page_table_index);
        printf("Number of page table entries: %u\n", (uint) obj->num_page_table_entries);
        printf("System-reserved field: %u\n", (uint) obj->reserved);

        printf("Object pages:\n");
        const LxObjectPageTableEntry *objpage = ((const LxObjectPageTableEntry *) (exe + lx->object_page_table_offset)) + (obj->page_table_index - 1);
        const uint32 *fixuppage = (((const uint32 *) (exe + lx->fixup_page_table_offset)) + (obj->page_table_index - 1));
        for (uint32 i = 0; i < obj->num_page_table_entries; i++, objpage++, fixuppage++) {
            printf("Object Page #%u:\n", (uint) (i + obj->page_table_index));
            printf("Page data offset: 0x%X\n", (uint) objpage->page_data_offset);
            printf("Page data size: %u\n", (uint) objpage->data_size);
            printf("Page flags: (%u)", (uint) objpage->flags);
            if (objpage->flags == 0x0) printf(" PHYSICAL");
            else if (objpage->flags == 0x1) printf(" ITERATED");
            else if (objpage->flags == 0x2) printf(" INVALID");
            else if (objpage->flags == 0x3) printf(" ZEROFILL");
            else if (objpage->flags == 0x4) printf(" RANGE");
            else if (objpage->flags == 0x5) printf(" COMPRESSED");
            else printf(" UNKNOWN");
            printf("\n");
            const uint32 fixupoffset = *fixuppage;
            const uint32 fixuplen = fixuppage[1] - fixuppage[0];
            printf("Page's fixup record offset: %u\n", (uint) fixupoffset);
            printf("Page's fixup record size: %u\n", (uint) fixuplen);
            printf("Fixup records:\n");
            const uint8 *fixup = (exe + lx->fixup_record_table_offset) + fixupoffset;
            const uint8 *fixupend = fixup + fixuplen;
            for (uint32 i = 0; fixup < fixupend; i++) {
                printf("Fixup Record #%u:\n", (uint) (i + 1));
                printf("Source type: ");
                const uint8 srctype = *(fixup++);
                if (srctype & 0x10) printf("[FIXUPTOALIAS] ");
                if (srctype & 0x20) printf("[SOURCELIST] ");
                switch (srctype & 0xF) {
                    case 0x00: printf("Byte fixup"); break;
                    case 0x02: printf("16-bit selector fixup"); break;
                    case 0x03: printf("16:16 pointer fixup"); break;
                    case 0x05: printf("16-bit offset fixup"); break;
                    case 0x06: printf("16:32 pointer fixup"); break;
                    case 0x07: printf("32-bit offset fixup"); break;
                    case 0x08: printf("32-bit self-relative offset fixup"); break;
                    default: printf("(undefined fixup)"); break;
                } // switch
                printf("\n");

                const uint8 fixupflags = *(fixup++);
                printf("Target flags:");
                switch (fixupflags & 0x3) {
                    case 0x0: printf(" INTERNAL"); break;
                    case 0x1: printf(" IMPORTBYORDINAL"); break;
                    case 0x2: printf(" IMPORTBYNAME"); break;
                    case 0x3: printf(" INTERNALVIAENTRY"); break;
                } // switch

                if (fixupflags & 0x4) printf(" ADDITIVE");
                if (fixupflags & 0x8) printf(" INTERNALCHAINING");
                if (fixupflags & 0x10) printf(" 32BITTARGETOFFSET");
                if (fixupflags & 0x20) printf(" 32BITADDITIVE");
                if (fixupflags & 0x40) printf(" 16BITORDINAL");
                if (fixupflags & 0x80) printf(" 8BITORDINAL");
                printf("\n");

                uint8 srclist_count = 0;
                if (srctype & 0x20) { // source list
                    srclist_count = *(fixup++);
                    printf("Source offset list count: %u\n", (uint) srclist_count);
                } else {
                    const sint16 srcoffset = *((sint16 *) fixup); fixup += 2;
                    printf("Source offset: %d\n", (int) srcoffset);
                } // else
                printf("\n");

                switch (fixupflags & 0x3) {
                    case 0x0:
                        printf("Internal fixup record:\n");
                        if (fixupflags & 0x40) { // 16 bit value
                            const uint16 val = *((uint16 *) fixup); fixup += 2;
                            printf("Object: %u\n", (uint) val);
                        } else {
                            const uint8 val = *(fixup++);
                            printf("Object: %u\n", (uint) val);
                        } // else

                        printf("Target offset: ");
                        if ((srctype & 0xF) == 0x2) { // 16-bit selector fixup
                            printf("[not used for 16-bit selector fixups]\n");
                        } else if (fixupflags & 0x10) {  // 32-bit target offset
                            const uint32 val = *((uint32 *) fixup); fixup += 4;
                            printf("%u\n", (uint) val);
                        } else {  // 16-bit target offset
                            const uint16 val = *((uint16 *) fixup); fixup += 2;
                            printf("%u\n", (uint) val);
                        } // else
                        break;

                    case 0x1:
                        printf("Import by ordinal fixup record:\n");
                        if (fixupflags & 0x40) { // 16 bit value
                            const uint16 val = *((uint16 *) fixup); fixup += 2;
                            printf("Module ordinal: %u\n", (uint) val);
                        } else {
                            const uint8 val = *(fixup++);
                            printf("Module ordinal: %u\n", (uint) val);
                        } // else

                        if (fixupflags & 0x80) { // 8 bit value
                            const uint8 val = *(fixup++);
                            printf("Import ordinal: %u\n", (uint) val);
                        } else if (fixupflags & 0x10) {  // 32-bit value
                            const uint32 val = *((uint32 *) fixup); fixup += 4;
                            printf("Import ordinal: %u\n", (uint) val);
                        } else {  // 16-bit value
                            const uint16 val = *((uint16 *) fixup); fixup += 2;
                            printf("Import ordinal: %u\n", (uint) val);
                        } // else

                        uint32 additive = 0;
                        if (fixupflags & 0x4) {  // Has additive.
                            if (fixupflags & 0x20) { // 32-bit value
                                additive = *((uint32 *) fixup);
                                fixup += 4;
                            } else {  // 16-bit value
                                additive = *((uint16 *) fixup);
                                fixup += 2;
                            } // else
                        } // if
                        printf("Additive: %u\n", (uint) additive);
                        break;

                    case 0x2: {
                        printf("Import by name fixup record:\n");
                        if (fixupflags & 0x40) { // 16 bit value
                            const uint16 val = *((uint16 *) fixup); fixup += 2;
                            printf("Module ordinal: %u\n", (uint) val);
                        } else {
                            const uint8 val = *(fixup++);
                            printf("Module ordinal: %u\n", (uint) val);
                        } // else

                        uint32 name_offset = 0;
                        if (fixupflags & 0x10) {  // 32-bit value
                            name_offset = *((uint32 *) fixup); fixup += 4;
                        } else {  // 16-bit value
                            name_offset = *((uint16 *) fixup); fixup += 2;
                        } // else

                        const uint8 *import_name = (exe + lx->import_proc_table_offset) + name_offset;
                        char name[128];
                        const uint8 namelen = *(import_name++) & 0x7F;
                        memcpy(name, import_name, namelen);
                        name[namelen] = '\0';
                        printf("Name offset: %u ('%s')\n", (uint) name_offset, name);

                        uint32 additive = 0;
                        if (fixupflags & 0x4) {  // Has additive.
                            if (fixupflags & 0x20) { // 32-bit value
                                additive = *((uint32 *) fixup);
                                fixup += 4;
                            } else {  // 16-bit value
                                additive = *((uint16 *) fixup);
                                fixup += 2;
                            } // else
                        } // if
                        printf("Additive: %u\n", (uint) additive);

                        break;
                    } // case

                    case 0x3:
                        printf("Internal entry table fixup record:\n");
                        printf("WRITE ME\n"); exit(1);
                        break;
                } // switch

                if (srctype & 0x20) { // source list
                    printf("Source offset list:");
                    for (uint8 i = 0; i < srclist_count; i++) {
                        const sint16 val = *((sint16 *) fixup); fixup += 2;
                        printf(" %d", (int) val);
                    } // for
                } // if

                printf("\n\n");
            } // while

            printf("\n");
        } // for
    } // for

    printf("\n");
    printf("Resource table (%u entries):\n", (uint) lx->num_resource_table_entries);
    for (uint32 i = 0; i < lx->num_resource_table_entries; i++) {
        const LxResourceTableEntry *rsrc = ((const LxResourceTableEntry *) (exe + lx->resource_table_offset)) + i;
        printf("%u:\n", (uint) i);
        printf("Type ID: %u\n", (uint) rsrc->type_id);
        printf("Name ID: %u\n", (uint) rsrc->name_id);
        printf("Resource size: %u\n", (uint) rsrc->resource_size);
        printf("Object: %u\n", (uint) rsrc->object);
        printf("Offset: 0x%X\n", (uint) rsrc->offset);
        printf("\n");
    } // for

    printf("\n");

    printf("Entry table:\n");
    int bundleid = 1;
    int ordinal = 1;
    const uint8 *entryptr = exe + lx->entry_table_offset;
    while (*entryptr) {  /* end field has a value of zero. */
        const uint8 numentries = *(entryptr++);  /* number of entries in this bundle */
        const uint8 bundletype = (*entryptr) & ~0x80;
        const uint8 paramtypes = (*(entryptr++) & 0x80) ? 1 : 0;

        printf("Bundle %d (%u entries): ", bundleid, (uint) numentries);
        bundleid++;

        if (paramtypes)
            printf("[PARAMTYPES] ");

        switch (bundletype) {
            case 0x00:
                printf("UNUSED\n");
                printf(" %u unused entries.\n\n", (uint) numentries);
                ordinal += numentries;
                break;

            case 0x01:
                printf("16BIT\n");
                printf(" Object number: %u\n", (uint) *((const uint16 *) entryptr)); entryptr += 2;
                for (uint8 i = 0; i < numentries; i++) {
                    printf(" %d:\n", ordinal++);
                    printf(" Flags:");
                    if (*entryptr & 0x1) printf(" EXPORTED");
                    printf("\n");
                    printf(" Parameter word count: %u\n",  (uint) ((*entryptr & 0xF8) >> 3)); entryptr++;
                    printf(" Offset: %u\n",  (uint) *((const uint16 *) entryptr)); entryptr += 2;
                    printf("\n");
                }
                break;

            case 0x02:
                printf("286CALLGATE\n");
                printf(" Object number: %u\n", (uint) *((const uint16 *) entryptr)); entryptr += 2;
                for (uint8 i = 0; i < numentries; i++) {
                    printf(" %d:\n", ordinal++);
                    printf(" Flags:");
                    if (*entryptr & 0x1) printf(" EXPORTED");
                    printf("\n");
                    printf(" Parameter word count: %u\n",  (uint) ((*entryptr & 0xF8) >> 3)); entryptr++;
                    printf(" Offset: %u\n",  (uint) *((const uint16 *) entryptr)); entryptr += 2;
                    printf(" Callgate selector: %u\n",  (uint) *((const uint16 *) entryptr)); entryptr += 2;
                    printf("\n");
                }
                break;

            case 0x03: printf("32BIT\n");
                printf(" Object number: %u\n", (uint) *((const uint16 *) entryptr)); entryptr += 2;
                for (uint8 i = 0; i < numentries; i++) {
                    printf(" %d:\n", ordinal++);
                    printf(" Flags:");
                    if (*entryptr & 0x1) printf(" EXPORTED");
                    printf("\n");
                    printf(" Parameter word count: %u\n",  (uint) ((*entryptr & 0xF8) >> 3)); entryptr++;
                    printf(" Offset: %u\n",  (uint) *((const uint32 *) entryptr)); entryptr += 4;
                    printf("\n");
                }
                break;

            case 0x04: printf("FORWARDER\n"); break;
                printf(" Reserved field: %u\n", (uint) *((const uint16 *) entryptr)); entryptr += 2;
                for (uint8 i = 0; i < numentries; i++) {
                    printf(" %d:\n", ordinal++);
                    printf(" Flags:");
                    const int isordinal = (*entryptr & 0x1);
                    if (isordinal) printf(" IMPORTBYORDINAL");
                    printf("\n");
                    printf(" Reserved for future use: %u\n",  (uint) ((*entryptr & 0xF8) >> 3)); entryptr++;
                    printf(" Module ordinal number: %u\n", (uint) *((const uint16 *) entryptr)); entryptr += 2;
                    if (isordinal) {
                        printf(" Import ordinal number: %u\n",  (uint) *((const uint32 *) entryptr)); entryptr += 4;
                    } else {
                        printf(" Import name offset: %u\n",  (uint) *((const uint32 *) entryptr)); entryptr += 4;
                    }
                    printf("\n");
                }
                break;

            default:
                printf("UNKNOWN (%u)\n\n", (uint) bundletype);
                break;  // !!! FIXME: what to do?
        } // switch
    } // while
    printf("\n");

    printf("Module directives (%u entries):\n", (uint) lx->num_module_directives);
    const uint8 *dirptr = exe + lx->module_directives_offset;
    for (uint32 i = 0; i < lx->num_module_directives; i++) {
        printf("%u:\n", (uint) i+1);
        printf("Directive ID: %u\n", (uint) *((const uint16 *) dirptr)); dirptr += 2;
        printf("Data size: %u\n", (uint) *((const uint16 *) dirptr)); dirptr += 2;
        printf("Data offset: %u\n", (uint) *((const uint32 *) dirptr)); dirptr += 4;
        printf("\n");
        // !!! FIXME: verify record directive table, etc, based on Directive ID
    }
    printf("\n");

    if (lx->per_page_checksum_offset == 0) {
        printf("No per-page checksums available.\n");
    } else {
        printf("!!! FIXME: look at per-page checksums!\n");
    }
    printf("\n");


    printf("Import modules (%u entries):\n", (uint) lx->num_import_mod_entries);
    const uint8 *import_modules_table = exe + lx->import_module_table_offset;
    for (uint32 i = 0; i < lx->num_import_mod_entries; i++) {
        char name[128];
        const uint8 namelen = *(import_modules_table++);
        // !!! FIXME: name can't be more than 127 chars, according to docs. Check this.
        memcpy(name, import_modules_table, namelen);
        import_modules_table += namelen;
        name[namelen] = '\0';
        printf("%u: %s\n", (uint) i+1, name);
    }

    const uint8 *name_table;

    printf("Resident name table:\n");
    name_table = exe + lx->resident_name_table_offset;
    for (uint32 i = 0; *name_table; i++) {
        const uint8 namelen = *(name_table++);
        char name[256];
        memcpy(name, name_table, namelen);
        name[namelen] = '\0';
        name_table += namelen;
        const uint16 ordinal = *((const uint16 *) name_table); name_table += 2;
        printf("%u: '%s' (ordinal %u)\n", (uint) i, name, (uint) ordinal);
    } // for

    printf("Non-resident name table:\n");
    name_table = origexe + lx->non_resident_name_table_offset;
    const uint8 *end_of_name_table = name_table + lx->non_resident_name_table_len;
    for (uint32 i = 0; (name_table < end_of_name_table) && *name_table; i++) {
        const uint8 namelen = *(name_table++);
        char name[256];
        memcpy(name, name_table, namelen);
        name[namelen] = '\0';
        name_table += namelen;
        const uint16 ordinal = *((const uint16 *) name_table); name_table += 2;
        printf("%u: '%s' (ordinal %u)\n", (uint) i, name, (uint) ordinal);
    } // for

    return 1;
} // parseLxExe

static int parseNeExe(const uint8 *origexe, const uint8 *exe)
{
    const NeHeader *ne = (const NeHeader *) exe;
    printf("NE (16-bit) executable.\n");
    printf("Linker version: %u\n", (uint) ne->linker_version);
    printf("Linker revision: %u\n", (uint) ne->linker_revision);
    printf("Entry table offset: %u\n", (uint) ne->entry_table_offset);
    printf("Entry table size: %u\n", (uint) ne->entry_table_size);
    printf("CRC32: 0x%X\n", (uint) ne->crc32);
    printf("Module flags:");
    if (ne->module_flags == 0) printf(" NOAUTODATA");
    if (ne->module_flags & 0x1) printf(" SINGLEDATA");
    if (ne->module_flags & 0x2) printf(" MULTIPLEDATA");
    if (ne->module_flags & 0x4) printf(" FAMILYAPI");
    if (ne->module_flags & 0x2000) printf(" NOTLOADABLE");
    if (ne->module_flags & 0x4000) printf(" NONCONFORMING");
    if (ne->module_flags & 0x8000) printf(" LIBRARYMODULE");
    printf("\n");
    printf("Application type: ");
    switch ((ne->module_flags >> 8) & 0x3) {
        case 0x0: printf("CONSOLE"); break;
        case 0x1: printf("FULLSCREEN"); break;
        case 0x2: printf("WINPMCOMPAT"); break;
        case 0x3: printf("WINPMUSES"); break;
    } // switch
    printf("\n");
    printf("Automatic data segment: %u\n", (uint) ne->auto_data_segment);
    printf("Dynamic heap size: %u\n", (uint) ne->dynamic_heap_size);
    printf("Stack size: %u\n", (uint) ne->stack_size);
    printf("Initial code address: %X:%X\n", (uint) ne->reg_cs, (uint) ne->reg_ip);
    printf("Initial stack address: %X:%X\n", (uint) ne->reg_ss, (uint) ne->reg_sp);
    printf("Number of segment table entries: %u\n", (uint) ne->num_segment_table_entries);
    printf("Number of module reference table entries: %u\n", (uint) ne->num_module_ref_table_entries);
    printf("Non-resident name table size: %u\n", (uint) ne->non_resident_name_table_size);
    printf("Segment table offset: %u\n", (uint) ne->segment_table_offset);
    printf("Resource table offset: %u\n", (uint) ne->resource_table_offset);
    printf("Resident name table offset: %u\n", (uint) ne->resident_name_table_offset);
    printf("Module reference table offset: %u\n", (uint) ne->module_reference_table_offset);
    printf("Imported names table offset: %u\n", (uint) ne->imported_names_table_offset);
    printf("Non-resident name table offset: %u\n", (uint) ne->non_resident_name_table_offset);
    printf("Number of movable entries: %u\n", (uint) ne->num_movable_entries);
    printf("Sector alignment shift count: %u\n", (uint) ne->sector_alignment_shift_count);
    printf("Number of resource entries: %u\n", (uint) ne->num_resource_entries);
    printf("Executable type: %u\n", (uint) ne->exe_type);

    printf("OS/2 flags:");
    if (ne->os2_exe_flags == 0) {
        printf(" (none)");
    } else {
        if (ne->os2_exe_flags & (1 << 0)) printf(" LONGFILENAMES");
        if (ne->os2_exe_flags & (1 << 1)) printf(" PROTECTEDMODE");
        if (ne->os2_exe_flags & (1 << 2)) printf(" PROPORTIONALFONTS");
        if (ne->os2_exe_flags & (1 << 3)) printf(" GANGLOADAREA");
    }
    printf("\n");
    printf("\n");

    const uint32 sector_shift = ne->sector_alignment_shift_count;

    if (ne->num_segment_table_entries > 0) {
        const uint32 total = (uint32) ne->num_segment_table_entries;
        printf("Segment table (%u entries):\n", (uint) total);
        const NeSegmentTableEntry *seg = (const NeSegmentTableEntry *) (exe + ne->segment_table_offset);
        for (uint32 i = 0; i < total; i++, seg++) {
            printf(" %u:\n", (uint) (i+1));
            printf("  Logical-sector offset: %04x", (uint) seg->offset);
            if (seg->offset == 0) {
                printf(" (no file data)\n");
            } else {
                printf(" (byte position %04x)\n", (uint) (((uint32) seg->offset) << sector_shift));
            }
            printf("  Size: %u\n", (seg->size == 0) ? (uint) 0x10000 : (uint) seg->size);
            printf("  Segment flags:");
            switch (seg->segment_flags & 0x3) {
                case 0: printf(" CODE"); break;
                case 1: printf(" DATA"); break;
                default: printf(" [UNKNOWN SEGMENT TYPE]"); break;
            }
            if (seg->segment_flags & 0x4) printf(" REALMODE");
            if (seg->segment_flags & 0x8) printf(" ITERATED");
            if (seg->segment_flags & 0x10) printf(" MOVABLE");
            if (seg->segment_flags & 0x20) printf(" SHAREABLE");
            if (seg->segment_flags & 0x40) printf(" PRELOAD");
            if (seg->segment_flags & 0x80) printf(" READONLY");
            if (seg->segment_flags & 0x100) printf(" RELOCINFO");
            if (seg->segment_flags & 0x200) printf(" DEBUGINFO");
            if (seg->segment_flags & 0xF000) printf(" DISCARD");
            printf("\n");

            printf("  Discard priority: %u\n", (uint) ((seg->segment_flags >> 13) & 0x7));
            printf("  Minimum allocation: %u\n", (seg->minimum_allocation == 0) ? (uint) 0x10000 : (uint) seg->minimum_allocation);

            if (seg->segment_flags & 0x100) {  // has relocations (fixups)
                const uint8 *segptr = origexe + (((uint32) seg->offset) << sector_shift);
                const uint8 *fixupptr = segptr + (seg->size ? seg->size : 0xFFFF);//FIXME
                const uint32 num_fixups = (uint32) *((const uint16 *) fixupptr); fixupptr += 2;
                printf("HEX:\n");
                for (int i=0; i<seg->size; i++) {
                    if (!(i & 15)) printf("%04x ", i);
                    printf("%02x ", segptr[i]);
                    if ((i & 15) == 15) printf("\n");
                }
                printf("\n");
                printf("  Fixup records (%u entries):\n", (uint) num_fixups);
                for (uint32 j = 0; j < num_fixups; j++) {
                    const uint8 srctype = *(fixupptr++);
                    const uint8 flags = *(fixupptr++);
                    uint32 srcchain_offset = (uint32) *((const uint16 *) fixupptr); fixupptr += 2;
                    const uint8 *targetptr = fixupptr; fixupptr += 4;
                    const int additive = (flags & 0x4) != 0;
                    printf("   %u:\n", (uint) j);
                    printf("    Source type: ");
                    switch (srctype & 0xF) {
                        case 0: printf("LOBYTE"); break;
                        case 2: printf("SEGMENT"); break;
                        case 3: printf("FAR_ADDR"); break;
                        case 5: printf("OFFSET"); break;
                        default: printf("[unknown type]"); break;
                    }
                    printf("\n");
                    printf("    Fixup type:");
                    if (additive) printf(" ADDITIVE");
                    switch (flags & 0x3) {
                        case 0: {
                            const uint8 segment = targetptr[0];
                            //const uint8 reserved = targetptr[1];
                            const uint16 offset = *((const uint16 *) &targetptr[2]);
                            printf(" INTERNALREF (");
                            if (segment == 0xFF)
                                printf("movable segment, ordinal %u)", (uint) offset);
                            else
                                printf("segment %04x, offset %04x)", (uint) segment, (uint) offset);
                        }
                        break;

                        case 1: {
                            const uint16 module = ((const uint16 *) targetptr)[0];
                            const uint16 ordinal = ((const uint16 *) targetptr)[1];
                            printf(" IMPORTORDINAL (module %u, ordinal %u)", (uint) module, (uint) ordinal);
                        }
                        break;

                        case 2: {
                            const uint16 module = ((const uint16 *) targetptr)[0];
                            const uint16 offset = ((const uint16 *) targetptr)[1];
                            printf(" IMPORTNAME (module %u, name offset %u)", (uint) module, (uint) offset);
                        }
                        break;

                        case 3: {
                            const uint16 type = ((const uint16 *) targetptr)[0];
                            printf(" OSFIXUP (type %u)", (uint) type);
                        }
                        break;
                    }
                    printf("\n");

                    if (additive) {
                        printf("    Additive source: %04x", (uint) srcchain_offset);
                    } else {
                        printf("    Source chain:");
                        do {
                            printf(" %04X", (uint) srcchain_offset);
                            srcchain_offset = *((const uint16 *) (segptr + srcchain_offset));
                        } while (srcchain_offset < seg->size);
                    }
                    printf("\n");
                }
            }
        }
        printf("\n");
    }

    if (ne->num_resource_entries > 0) {
        const uint8 *ptr = exe + ne->resource_table_offset;
        const uint32 total = (uint32) ne->num_resource_entries;
        printf("Resources (%u entries):\n", (uint) total);
        int idx = 0;
        while (*((const uint16 *) ptr)) {
            const uint16 typeid = *((const uint16 *) ptr); ptr += 2;
            const uint32 num_resources = (uint32) *((const uint16 *) ptr); ptr += 2;
            ptr += 4;  // reserved.
            for (uint32 j = 0; j < num_resources; j++) {
                idx++;
                printf(" %d:", idx);
                printf("  Type ID: ");
                if (typeid & 0x8000) {
                    printf("%u\n", (uint) (typeid & ~0x8000));
                } else {
                    const uint8 *str = (exe + ne->resource_table_offset) + typeid;
                    const uint32 len = (uint32) *(str++);
                    printf("\"");
                    for (uint32 k = 0; k < len; k++, str++) {
                        printf("%c", (int) *str);
                    }
                    printf("\"\n");
                }

                const uint16 offset = *((const uint16 *) ptr); ptr += 2;
                const uint16 size = *((const uint16 *) ptr); ptr += 2;
                const uint16 flags = *((const uint16 *) ptr); ptr += 2;
                const uint16 resourceid = *((const uint16 *) ptr); ptr += 2;
                ptr += 4;  // reserved.
                printf("  Offset: %u\n", (uint) offset);
                printf("  Size: %u\n", (uint) size);
                printf("  Flags:");
                if (flags & 0x10) printf(" MOVABLE");
                if (flags & 0x20) printf(" PURE");
                if (flags & 0x40) printf(" PRELOAD");
                printf("\n");
                printf("  Resource ID: ");
                if (resourceid & 0x8000) {
                    printf("%u\n", (uint) (resourceid & ~0x8000));
                } else {
                    const uint8 *str = (exe + ne->resource_table_offset) + resourceid;
                    const uint32 len = (uint32) *(str++);
                    printf("\"");
                    for (uint32 k = 0; k < len; k++, str++) {
                        printf("%c", (int) *str);
                    }
                    printf("\"\n");
                }
            }
        }
        printf("\n");
    }

    if (ne->resident_name_table_offset > 0) {
        const uint8 *name_table = exe + ne->resident_name_table_offset;
        if (*name_table) {
            printf("Resident name table:\n");
            for (uint32 i = 0; *name_table; i++) {
                const uint8 namelen = *(name_table++);
                char name[256];
                memcpy(name, name_table, namelen);
                name[namelen] = '\0';
                name_table += namelen;
                const uint16 ordinal = *((const uint16 *) name_table); name_table += 2;
                printf(" %u: '%s' (ordinal %u)\n", (uint) i, name, (uint) ordinal);
            }
            printf("\n");
        }
    }

    if (ne->non_resident_name_table_offset > 0) {
        const uint8 *name_table = origexe + ne->non_resident_name_table_offset;
        const uint8 *end_of_name_table = name_table + ne->non_resident_name_table_size;
        if ((name_table < end_of_name_table) && *name_table) {
            printf("Non-resident name table:\n");
            for (uint32 i = 0; (name_table < end_of_name_table) && *name_table; i++) {
                const uint8 namelen = *(name_table++);
                char name[256];
                memcpy(name, name_table, namelen);
                name[namelen] = '\0';
                name_table += namelen;
                const uint16 ordinal = *((const uint16 *) name_table); name_table += 2;
                printf(" %u: '%s' (ordinal %u)\n", (uint) i, name, (uint) ordinal);
            }
            printf("\n");
        }
    }

    if (ne->num_module_ref_table_entries > 0) {
        const uint32 total = (uint32) ne->num_module_ref_table_entries;
        printf("Module reference table (%u entries):\n", (uint) total);
        const uint16 *ptr = (const uint16 *) (exe + ne->module_reference_table_offset);
        for (uint32 i = 0; i < total; i++, ptr++) {
            const uint8 *name = (exe + ne->imported_names_table_offset) + *ptr;
            const uint32 name_string_len = (uint32) *name;
            name++;
            printf(" %u: ", (uint) i+1);
            for (uint32 j = 0; j < name_string_len; j++, name++) {
                printf("%c", (int) *name);
            }
            printf("\n");
        }
        printf("\n");
    }

    if (ne->entry_table_size > 0) {
        const uint8 *ptr = exe + ne->entry_table_offset;
        if (*ptr) {
            printf("Entry table:\n");
            uint32 ordinal = 1;
            while (1) {
                const uint8 bundled = *(ptr++);
                if (!bundled) {
                    break;
                }

                const uint8 bundletype = *(ptr++);
                switch (bundletype) {
                    case 0x00:  // unused entries, to skip ordinals.
                        ordinal += bundled;
                        break;

                    case 0xFF: // Moveable segment entries.
                        for (uint32 i = 0; i < bundled; i++, ordinal++) {
                            const uint8 flags = *(ptr++);
                            const uint16 int3f = *((const uint16 *) ptr); ptr += 2;
                            const uint8 segment = *(ptr++);
                            const uint16 offset = *((const uint16 *) ptr); ptr += 2;
                            printf(" %u:\n", (uint) ordinal);
                            printf("  Flags: MOVABLE");
                            if (flags & 0x1) printf(" EXPORTED");
                            if (flags & 0x2) printf(" GLOBAL");
                            printf("\n");
                            printf("  Int3f: %X\n", (uint) int3f);
                            printf("  Segment: %u\n", (uint) segment);
                            printf("  Offset: %u\n", (uint) offset);
                        }
                        break;

                    default:
                        for (uint32 i = 0; i < bundled; i++, ordinal++) {
                            const uint8 flags = *(ptr++);
                            const uint16 offset = *((const uint16 *) ptr); ptr += 2;
                            printf(" %u:\n", (uint) ordinal);
                            printf("  Flags: FIXED");
                            if (flags & 0x1) printf(" EXPORTED");
                            if (flags & 0x2) printf(" GLOBAL");
                            printf("\n");
                            printf("  Segment: %u\n", (uint) bundletype);
                            printf("  Offset: %u\n", (uint) offset);
                        }
                        break;
                }
            }
            printf("\n");
        }
    }

    return 1;
} // parseNeExe

static void parseExe(const char *exefname, uint8 *exe, uint32 exelen)
{
    int is_lx = 0;

    printf("os2dump: %s\n", exefname);

    const uint8 *origexe = exe;
    if (!sanityCheckExe(&exe, &exelen, &is_lx))
        return;

    if (is_lx) {
        parseLxExe(origexe, exe);
    } else {
        parseNeExe(origexe, exe);
    }
} // parseExe

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <program.exe>\n", argv[0]);
        return 1;
    }

    const char *exefname = argv[1];
    FILE *io = fopen(exefname, "rb");
    if (!io) {
        fprintf(stderr, "can't open '%s: %s'\n", exefname, strerror(errno));
        return 2;
    }

    if (fseek(io, 0, SEEK_END) < 0) {
        fprintf(stderr, "can't seek in '%s': %s\n", exefname, strerror(errno));
        return 3;
    }

    const uint32 exelen = ftell(io);
    uint8 *exe = (uint8 *) malloc(exelen);
    if (!exe) {
        fprintf(stderr, "Out of memory\n");
        return 4;
    }

    rewind(io);
    if (fread(exe, exelen, 1, io) != 1) {
        fprintf(stderr, "read failure on '%s': %s\n", exefname, strerror(errno));
        return 5;
    }

    fclose(io);

    parseExe(exefname, exe, exelen);

    free(exe);

    return 0;
} // main

// end of lx_dump.c ...
