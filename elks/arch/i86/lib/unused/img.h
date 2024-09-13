/* img.h - structure of the Psion .img file
 *
 *  From a post to comp.sys.psion.programmer,
 *  by Alasdair Manson <ali-manson@psion.com>
 *
 *  Probably (C) Psion Ltd.
 */

#define SignatureSize 16
#define MaxAddFiles 4

typedef struct {
    unsigned short offset;
    unsigned short length;
} ADDFILE;

typedef struct {
    unsigned char Signature[SignatureSize];
    unsigned short ImageVersion;
    unsigned short HeaderSizeBytes;
    unsigned short CodeParas;
    unsigned short InitialIP;
    unsigned short StackParas;
    unsigned short DataParas;
    unsigned short HeapParas;
    unsigned short InitialisedData;
    unsigned short CodeCheckSum;
    unsigned short DataCheckSum;
    unsigned short CodeVersion;
    unsigned short Priority;
    ADDFILE Add[MaxAddFiles];
    unsigned short DylCount;
#if 0
    unsigned long DylTableOffset;
#endif
    unsigned short DylTableOffsetLo;
    unsigned short DylTableOffsetHi;
    unsigned short Spare;
} ImgHeader;
