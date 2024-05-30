/**
 * 2ine; an OS/2 emulator for Linux.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

/* lib2ine is support code and data that is common between various native
   reimplementations of the OS/2 API, and the OS/2 binary loader. */

#ifndef _INCL_LIB2INE_H_
#define _INCL_LIB2INE_H_ 1

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t sint8;
typedef int16_t sint16;
typedef int32_t sint32;
typedef int64_t sint64;

typedef unsigned int uint;

#ifndef _STUBBED
    #define _STUBBED(x, what) do { \
        static int seen_this = 0; \
        if (!seen_this) { \
            seen_this = 1; \
            fprintf(stderr, "2INE " what ": %s at %s (%s:%d)\n", x, __FUNCTION__, __FILE__, __LINE__); \
        } \
    } while (0)
#endif

#if 0
#ifndef STUBBED
#define STUBBED(x) _STUBBED(x, "STUBBED")
#endif
#ifndef FIXME
#define FIXME(x) _STUBBED(x, "FIXME")
#endif
#endif

#ifndef STUBBED
    #define STUBBED(x) do {} while (0)
#endif
#ifndef FIXME
    #define FIXME(x) do {} while (0)
#endif

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


typedef struct LxMmaps
{
    void *mapped;
    void *addr;
    size_t size;
    int prot;
    uint16 alias;  // 16:16 alias, if one.
} LxMmaps;

typedef struct LxExport
{
    uint32 ordinal;
    const char *name;  // can be NULL
    void *addr;
    const LxMmaps *object;
} LxExport;

struct LxModule;
typedef struct LxModule LxModule;
struct LxModule
{
    uint32 refcount;
    int is_lx;  // 1 if an LX module, 0 if NE.
    union
    {
        LxHeader lx;
        NeHeader ne;
    } header;

    char name[256];  // !!! FIXME: allocate this.

    LxModule **dependencies;
    uint32 num_dependencies;

    LxMmaps *mmaps;
    uint32 num_mmaps;

    const LxExport *exports;
    uint32 num_exports;
    void *nativelib;
    uint32 eip;
    uint32 esp;
    uint16 autodatasize;  // only used for NE binaries.
    int initialized;
    char *os2path;  // absolute path to module, in OS/2 format
    // !!! FIXME: put this elsewhere?
    uint32 signal_exception_focus_count;
    LxModule *prev;  // all loaded modules are in a doubly-linked list.
    LxModule *next;  // all loaded modules are in a doubly-linked list.
};

#pragma pack(push, 1)
typedef struct LxTIB2
{
    uint32 tib2_ultid;
    uint32 tib2_ulpri;
    uint32 tib2_version;
    uint16 tib2_usMCCount;
    uint16 tib2_fMCForceFlag;
} LxTIB2;

typedef struct LxTIB
{
    void *tib_pexchain;
    void *tib_pstack;
    void *tib_pstacklimit;
    LxTIB2 *tib_ptib2;
    uint32 tib_version;
    uint32 tib_ordinal;
} LxTIB;

typedef struct LxPIB
{
    uint32 pib_ulpid;
    uint32 pib_ulppid;
    void *pib_hmte;
    char *pib_pchcmd;
    char *pib_pchenv;
    uint32 pib_flstatus;
    uint32 pib_ultype;
} LxPIB;

#pragma pack(pop)

// We put the 128 bytes of TLS slots (etc) after the TIB structs.
typedef struct LxPostTIB
{
    uint32 tls[32];
    void *anchor_block;
} LxPostTIB;

// There are a few things that need access to the sound hardware:
//  DosBeep(), MMOS/2, DART, basic system sounds, etc. We unify all this
//  into a single SDL audio device open where possible, so that we don't
//  have to deal with multiple devices, but that means it has to move into
//  lib2ine so multiple libraries can access it.
typedef int (*LxAudioGeneratorFn)(void *data, float *stream, int len, int freq);

#define LXTIBSIZE (sizeof (LxTIB) + sizeof (LxTIB2) + sizeof (LxPostTIB))

#define LX_MAX_LDT_SLOTS 8192

typedef struct LxLoaderState
{
    LxModule *loaded_modules;
    LxModule *main_module;
    uint8 main_tibspace[LXTIBSIZE];
    LxPIB pib;
    int using_lx_loader;
    int subprocess;
    int running;
    int trace_native;
    int trace_events;
    char *disks[26];  // mount points, A: through Z: ... NULL if unmounted.
    char *current_dir[26];  // current directory, per-disk, A: through Z: ... NULL if unmounted.
    int current_disk;  // 1==A:\\, 2==B:\\, etc.
    uint32 diskmap;  // 1<<0==drive A mounted, 1<<1==drive B mounted, etc.
    float beep_volume;
    uint8 main_tib_selector;
    uint32 mainstacksize;
    uint16 original_cs;
    uint16 original_ds;
    uint16 original_es;
    uint16 original_ss;
    uint32 *ldt; //[LX_MAX_LDT_SLOTS];
    char *libpath;
    uint32 libpathlen;
    uint32 *tlspage;
    uint32 tlsmask;  // one bit for each TLS slot in use.
    uint8 tlsallocs[32];  // number of TLS slots allocated in one block, indexed by starting block (zero if not the starting block).
    void (*dosExit)(uint32 action, uint32 result);
    void (*initOs2Tib)(uint8 *tibspace, void *_topOfStack, const size_t stacklen, const uint32 tid);
    uint16 (*setOs2Tib)(uint8 *tibspace);
    LxTIB *(*getOs2Tib)(void);
    void (*deinitOs2Tib)(const uint16 selector);
    void *(*allocSegment)(uint16 *selector, const int iscode);
    void (*freeSegment)(const uint16 selector);
    int (*findSelector)(const uint32 addr, uint16 *outselector, uint16 *outoffset, int iscode);
    void (*freeSelector)(const uint16 selector);
    void *(*convert1616to32)(const uint32 addr1616);
    uint32 (*convert32to1616)(void *addr32);
    LxModule *(*loadModule)(const char *modname);
    char *(*makeUnixPath)(const char *os2path, uint32 *err);
    void __attribute__((noreturn)) (*terminate)(const uint32 exitcode);
    int (*registerAudioGenerator)(LxAudioGeneratorFn fn, void *data, const int singleton);
    void (*lib2ine_shutdown)(void);
} LxLoaderState;

typedef const LxExport *(*LxNativeModuleInitEntryPoint)(uint32 *lx_num_exports);
typedef void (*LxNativeModuleDeinitEntryPoint)(void);

// !!! FIXME: need to change this symbol name.
extern __attribute__((visibility("default"))) LxLoaderState GLoaderState;

#define LX_NATIVE_CONSTRUCTOR(modname) void __attribute__((constructor)) lxNativeConstructor_##modname(void)
#define LX_NATIVE_DESTRUCTOR(modname) void __attribute__((destructor)) lxNativeDestructor_##modname(void)

#endif

// end of lib2ine.h ...

