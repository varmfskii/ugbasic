#ifndef __UGBASICCOMPILER__
#define __UGBASICCOMPILER__

/*****************************************************************************
 * ugBASIC - an isomorphic BASIC language compiler for retrocomputers        *
 *****************************************************************************
 * Copyright 2021-2024 Marco Spedaletti (asimov@mclink.it)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *----------------------------------------------------------------------------
 * Concesso in licenza secondo i termini della Licenza Apache, versione 2.0
 * (la "Licenza"); è proibito usare questo file se non in conformità alla
 * Licenza. Una copia della Licenza è disponibile all'indirizzo:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Se non richiesto dalla legislazione vigente o concordato per iscritto,
 * il software distribuito nei termini della Licenza è distribuito
 * "COSÌ COM'È", SENZA GARANZIE O CONDIZIONI DI ALCUN TIPO, esplicite o
 * implicite. Consultare la Licenza per il testo specifico che regola le
 * autorizzazioni e le limitazioni previste dalla medesima.
 ****************************************************************************/

/*! @mainpage ugBASIC - REFERENCE MANUAL
 *
 * @section intro_sec Introduction
 *
 * This documentation is intended for anyone wishing to extend the compiler 
 * to include new instructions, new build targets, and support other hardware.
 * However, it is not suitable for those who simply want to use the ugBASIC 
 * language, for which you should refer to the [USER MANUAL](https://retroprogramming.iwashere.eu/ugbasic:user).
 */

/****************************************************************************
 * INCLUDE SECTION 
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include "libs/tsx.h"
#include "libs/tmx.h"

/****************************************************************************
 * DECLARATIONS AND DEFINITIONS SECTION 
 ****************************************************************************/

#ifdef _WIN32
    #define PATH_SEPARATOR          '\\'
#else
    #define PATH_SEPARATOR          '/'
#endif

#ifdef _DEBUG
    #define TRACE0( s )         (void)printf( "trace: %s\n", s );
    #define TRACE1( s, p1 )     { \
                                    char temporary[MAX_TEMPORARY_STORAGE]; \
                                    (void)sprintf( temporary, s, p1 ); \
                                    TRACE0( temporary ); \
                                }
    #define TRACE2( s, p1, p2 ) { \
                                    char temporary[MAX_TEMPORARY_STORAGE]; \
                                    (void)sprintf( temporary, s, p1, p2 ); \
                                    TRACE0( temporary ); \
                                }
#else
    #define TRACE0( s )             (void) s;
    #define TRACE1( s, p1 )         (void) s; (void) p1;
    #define TRACE2( s, p1, p2 )     (void) s; (void) p1; (void) p2;
#endif

/**
 * @brief Type of memory banks
 */
typedef enum _BankType {

    /** Executable code */
    BT_CODE = 0,

    /** Variables */
    BT_VARIABLES = 1,

    /** Temporary variables */
    BT_TEMPORARY = 2,

    /** Generic (unknonw) data */
    BT_DATA = 3,

    /** Strings (static and dynamic) */
    BT_STRINGS = 4

} BankType;

/**
 * @brief Maximum number of bank types
 */
#define BANK_TYPE_COUNT   5

/**
 * @brief Structure of a single bank
 */
typedef struct _Bank {

    /** ID of the bank */
    int id;

    /** Name of the bank */
    char * name;

    /** Starting address */
    int address;

    /** Bank type */
    BankType type;

    /** (optional) file name that will be loaded into the bank */
    char *filename;

    /** Bank max size (in bytes) */
    int space;

    /** Bank remaining size (in bytes) */
    int remains;

    /** Data contained in the block */
    char * data;

    /** Link to the next bank (NULL if this is the last one) */
    struct _Bank * next;

} Bank;

/**
 * @brief Structure of a single file inside a storage
 */
typedef struct _FileStorage {

    /** ID of the file */
    int id;

    /** Source name of the file */
    char * sourceName;

    /** Target name of the file */
    char * targetName;

    /** Size of the file */
    int size;

    /** Variable (eventually) linked for automatic loading */
    struct _Variable * variable;

    /** Content of the file */
    char * content;

    /** Link to the next file (NULL if this is the last one) */
    struct _FileStorage * next;

} FileStorage;

/**
 * @brief Structure of a single storage
 */
typedef struct _Storage {

    /** ID of the storage */
    int id;

    /** Name of the storage */
    char * name;

    /** Filename of the storage */
    char * fileName;

    /** List of files */
    FileStorage * files;

    /** Link to the next storage (NULL if this is the last one) */
    struct _Storage * next;

} Storage;

typedef enum _OutputFileType {

    OUTPUT_FILE_TYPE_BIN = 0,
    OUTPUT_FILE_TYPE_PRG = 1,
    OUTPUT_FILE_TYPE_XEX = 2,
    OUTPUT_FILE_TYPE_K7_ORIGINAL = 3,
    OUTPUT_FILE_TYPE_K7_NEW = 4,
    OUTPUT_FILE_TYPE_TAP = 5,
    OUTPUT_FILE_TYPE_CAS = 6,
    OUTPUT_FILE_TYPE_ROM = 7,
    OUTPUT_FILE_TYPE_D64 = 8,
    OUTPUT_FILE_TYPE_DSK = 9,
    OUTPUT_FILE_TYPE_ATR = 10

} OutputFileType;

typedef enum _PeepHoleOptimizationKind {
    PEEPHOLE = 1, 
    DEADVARS = 2, 
    RELOCATION1 = 3, 
    RELOCATION2 = 4
} PeepHoleOptimizationKind;

/* expanable string */
struct _POBuffer {
    char *str; /* actual string */
    int   len; /* string length (not counting null char) */
    int   cap; /* capacity of buffer */
};

typedef struct _POBuffer *POBuffer;

/**
 * @brief Gamma correction type (for some palettes)
 * 
 * This enumeration represent the type of color correction
 * to be used.
 */
typedef enum _GammaCorrection {

    /* No gamma correction applied. */
    GAMMA_CORRECTION_NONE = 0,

    /* Gamma correction by Samuel Devulder */
    GAMMA_CORRECTION_TYPE1 = 1,

    /* Gamma correction by Dino Florenzi */
    GAMMA_CORRECTION_TYPE2 = 2

} GammaCorrection;

/**
 * @brief Structure of a single (static) string
 */
typedef struct _StaticString {

    /** unique id */
    int id;

    /** String */
    char * value;

    int size;

    /** Link to the next string (NULL if this is the last one) */
    struct _StaticString * next;

} StaticString;

/**
 * @brief Structure to store color components (red, green and blue)
 * 
 * This structure stores the color components (red, blue and green) 
 * of a pixel, 8 bits wide. This structure is used both to represent the 
 * retrocomputer palette and to process input data from image files.
 */
typedef struct _RGBi {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    unsigned char index;
    char description[64];
    unsigned char hardwareIndex;
    unsigned char used;
    int count;
} RGBi;

/**
 * @brief Type of variables
 * 
 * @todo support for data type VT_STRING
 * @todo support for signed data type
 * @todo support for arrays
 */
typedef enum _VariableType {

    /** Single unsigned byte (8 bit) */
    VT_BYTE = 1,
    /** Single signed byte (7 bit w / 2 complement) */
    VT_SBYTE = 2,

    /** Single unsigned word (16 bit) */
    VT_WORD = 3,
    /** Single signed word (15 bit w / 2 complement) */
    VT_SWORD = 4,

    /** Single unsigned double word (32 bit) */
    VT_DWORD = 5,
    /** Single signed double word (31 bit w / 2 complement) */
    VT_SDWORD = 6,

    /** Memory address (16 bit) */
    VT_ADDRESS = 7,

    /** Ordinate or abscissa (8 bit or 16 bit) */
    VT_POSITION = 8,

    /** Color index (8 bit) */
    VT_COLOR = 9,

    /** Strings (static) */
    VT_STRING = 10,

    /** Static buffer of a specific size */
    VT_BUFFER = 11,

    /** Array of any kind */
    VT_ARRAY = 12,

    /** Strings (dynamic) */
    VT_DSTRING = 13,

    /** MOBs (Movable OBjects) -- deprecated! */
    // VT_MOB = 14,

    /** IMAGE (static picture) */
    VT_IMAGE = 15,

    /** THREAD ID */
    VT_THREAD = 16,

    /** IMAGES (static pictures) + optional tile set attached */
    VT_IMAGES = 17,

    /** CHAR (printable character) */
    VT_CHAR = 18,

    /** SPRITE (basic hardware movable objects) */
    VT_SPRITE = 19,

    /** TILE (basic character sized graphic) */
    VT_TILE = 20,

    /** TILES (rectagled group of character sized graphics) */
    VT_TILES = 21,

    /** TILESET (a set of tiles) */
    VT_TILESET = 22,

    /** SEQUENCE (a set of images) */
    VT_SEQUENCE = 23,

    /** MUSIC (a [piece of] music) */
    VT_MUSIC = 24,

    /** BLIT (blit definition) */
    VT_BLIT = 25,

    /** FLOAT (floating point) */
    VT_FLOAT = 26,

    /** TILEMAP */
    VT_TILEMAP = 27,

    /** BIT */
    VT_BIT = 28

} VariableType;

typedef struct _Resource {

    char        *   realName;
    int             isAddress;
    VariableType    type;

} Resource;

#define MAX_TEMPORARY_STORAGE           1024

#define MAX_ARRAY_DIMENSIONS            256
#define MAX_PARAMETERS                  256
#define MAX_PALETTE                     256
#define MAX_TILESETS                    256
#define MAX_NESTED_ARRAYS               16
#define MAX_PROCEDURES                  4096
#define MAX_RESIDENT_SHAREDS            128
#define PROTOTHREAD_DEFAULT_COUNT       16
#define DSTRING_DEFAULT_COUNT           255
#define DSTRING_DEFAULT_SPACE           1024

#define FONT_SCHEMA_EMBEDDED            0
#define FONT_SCHEMA_STANDARD            1
#define FONT_SCHEMA_SEMIGRAPHIC         2
#define FONT_SCHEMA_COMPLETE            3
#define FONT_SCHEMA_ALPHA               4
#define FONT_SCHEMA_ASCII               5
#define FONT_DEFAULT_SCHEMA             FONT_SCHEMA_EMBEDDED

#define VT_BW_1BIT( t, v )              ( ( (t) == (v) ) ? 1 : 0 )
#define VT_BW_8BIT( t, v )              ( ( (t) == (v) ) ? 8 : 0 )
#define VT_BW_16BIT( t, v )             ( ( (t) == (v) ) ? 16 : 0 )
#define VT_BW_24BIT( t, v )             ( ( (t) == (v) ) ? 24 : 0 )
#define VT_BW_32BIT( t, v )             ( ( (t) == (v) ) ? 32 : 0 )
#define VT_BW_40BIT( t, v )             ( ( (t) == (v) ) ? 40 : 0 )
#define VT_BW_64BIT( t, v )             ( ( (t) == (v) ) ? 64 : 0 )
#define VT_BW_80BIT( t, v )             ( ( (t) == (v) ) ? 80 : 0 )
#define VT_BW_128BIT( t, v )             ( ( (t) == (v) ) ? 128 : 0 )

#define VT_BITWIDTH( t ) \
        ( VT_BW_1BIT( t, VT_BIT ) + VT_BW_8BIT( t, VT_CHAR ) + VT_BW_8BIT( t, VT_BYTE ) + VT_BW_8BIT( t, VT_SBYTE ) + VT_BW_8BIT( t, VT_COLOR ) + VT_BW_8BIT( t, VT_THREAD ) + \
        VT_BW_16BIT( t, VT_WORD ) + VT_BW_16BIT( t, VT_SWORD ) + VT_BW_16BIT( t, VT_ADDRESS ) + VT_BW_16BIT( t, VT_POSITION ) + \
        VT_BW_32BIT( t, VT_DWORD ) + VT_BW_32BIT( t, VT_SDWORD ) )

#define VT_POW2_2( t, v )             ( ( (t) == (v) ) ? 2 : 0 )
#define VT_POW2_3( t, v )             ( ( (t) == (v) ) ? 3 : 0 )
#define VT_POW2_4( t, v )             ( ( (t) == (v) ) ? 4 : 0 )

#define VT_MAX_BITWIDTH_TYPE( a, b ) \
        ( ( ( a == VT_FLOAT ) || ( b == VT_FLOAT ) ) ? ( VT_FLOAT ) : \
            ( VT_BITWIDTH( a ) > VT_BITWIDTH( b ) ) ? ( a ) : ( b ) )

#define VT_MAX_FLOAT_BITWIDTH_TYPE( a, b ) \
        ( ( VT_FLOAT_BITWIDTH( a ) > VT_FLOAT_BITWIDTH( b ) ) ? ( a ) : ( b ) )

#define VT_SIGNED( t ) \
        ( ( (t) == VT_SBYTE ) || ( (t) == VT_SWORD ) || ( (t) == VT_SDWORD ) || ( (t) == VT_POSITION ) || ( (t) == VT_FLOAT ) )

#define VT_UNSIGN( t ) \
            ( VT_SIGNED( t ) ? \
                ( \
                    ( ( (t) == (VT_SBYTE) ) ? VT_BYTE : 0 ) + \
                    ( ( (t) == (VT_SWORD) ) ? VT_WORD : 0 ) + \
                    ( ( (t) == (VT_SDWORD) ) ? VT_DWORD : 0 ) + \
                    ( ( (t) == (VT_POSITION) ) ? VT_WORD : 0 ) + \
                    ( ( (t) == (VT_FLOAT) ) ? VT_FLOAT : 0 ) \
                ) \
            : t )

#define VT_SIGN( t ) \
            ( ( ! VT_SIGNED( t ) ) ? \
                ( \
                    ( ( (t) == (VT_BIT) ) ? VT_BIT : 0 ) + \
                    ( ( (t) == (VT_BYTE) ) ? VT_SBYTE : 0 ) + \
                    ( ( (t) == (VT_WORD) ) ? VT_SWORD : 0 ) + \
                    ( ( (t) == (VT_DWORD) ) ? VT_SDWORD : 0 ) + \
                    ( ( (t) == (VT_FLOAT) ) ? VT_FLOAT : 0 ) + \
                    ( ( (t) == (VT_POSITION) ) ? VT_POSITION : 0 ) + \
                    ( ( (t) == (VT_ADDRESS) ) ? VT_ADDRESS : 0 ) + \
                    ( ( (t) == (VT_COLOR) ) ? VT_COLOR : 0 ) \
                ) \
            : t )

#define VT_SIGN_8BIT( v ) ( v < 0 ? ( ((~(unsigned char)(abs(v)))+1 ) ) : (v) )
#define VT_SIGN_16BIT( v ) ( v < 0 ? ( ((~(unsigned short)(abs(v)))+1 ) ) : (v) )
#define VT_SIGN_32BIT( v ) ( v < 0 ? ( (~((unsigned int) (abs(v)))+1 ) ) : (v) )

#define VT_ESIGN_8BIT( t, v ) ( VT_SIGNED(t) ? VT_SIGN_8BIT(v) : (v) )
#define VT_ESIGN_16BIT( t, v ) ( VT_SIGNED(t) ? VT_SIGN_16BIT(v) : (v) ) 
#define VT_ESIGN_32BIT( t, v ) ( VT_SIGNED(t) ? VT_SIGN_32BIT(v) : (v) ) 

#define VT_USIGN_8BIT( v ) (char)( ( v & 0x80 ) ? -( ((~((unsigned char)v)))+1 ) : (v) )
#define VT_USIGN_16BIT( v ) (short)( ( v & 0x8000 ) ? -( ((~((unsigned short)v)))+1 ) : (v) )
#define VT_USIGN_32BIT( v ) (int)( ( v & 0x80000000 ) ? -( ((~((unsigned int)v)))+1 ) : (v) )

#define VT_UNSIGN_8BIT( t, v ) ( VT_SIGNED(t) ? VT_USIGN_8BIT(v) : (v) )
#define VT_UNSIGN_16BIT( t, v ) ( VT_SIGNED(t) ? VT_USIGN_16BIT(v) : (v) ) 
#define VT_UNSIGN_32BIT( t, v ) ( VT_SIGNED(t) ? VT_USIGN_32BIT(v) : (v) ) 

/**
 * @brief Maximum number of variable types
 */
#define VARIABLE_TYPE_COUNT   26

/**
 * @brief Enum for memory area type
 * 
 * This enum will describe the kind of memory area, if it is
 * accessible directly (so: it can be used for code and variable,
 * without limits) or it is "gated" through a specific prologue
 * and epilogue code.
 */
typedef enum _MemoryAreaType {

    /**
     * This memory area can be accessed directly,
     * and can be initialized by compiler.
     */
    MAT_DIRECT = 1,

    /**
     * This memory area can be accessed only after calling a specific
     * prologue and epilogue code. 
     */
    MAT_GATED = 2,

    /**
     * This memory area can be accessed directly but it is not
     * initialized directly -- only at runtime. 
     */
    MAT_RAM = 3

} MemoryAreaType;

typedef struct _MemoryArea {

    int id;

    /**
     * Initial starting address
     */
    int start;

    /**
     * Starting address
     */
    int current;

    /**
     * Ending address
     */
    int end;

    /**
     * Current available size
     */
    int size;

    /**
     * Type
     */
    MemoryAreaType type;

    /** Link to the next memory area (NULL if this is the last one) */
    struct _MemoryArea * next;

} MemoryArea;

#define MEMORY_AREA_DEFINE( _type, _start, _end ) \
    { \
        MemoryArea * memoryArea = malloc( sizeof( MemoryArea ) ); \
        memset( memoryArea, 0, sizeof( MemoryArea ) ) ; \
        memoryArea->id = UNIQUE_ID; \
        memoryArea->start = _start; \
        memoryArea->current = _start; \
        memoryArea->end = _end; \
        memoryArea->size = (_end-_start); \
        memoryArea->type = _type; \
        memoryArea->next = NULL; \
        MemoryArea * last = _environment->memoryAreas; \
        if ( last ) { \
            while( last->next ) { \
                last = last->next; \
            } \
            last->next = memoryArea; \
        } else { \
            _environment->memoryAreas = memoryArea; \
        } \
    }

typedef enum _ConstantType {

    CT_INTEGER = 0,         // integer
    CT_STRING = 1,          // string
    CT_FLOAT = 2            // float

} ConstantType;

/**
 * @brief Structure of a single constant
 */
typedef struct _Constant {

    /** Name of the constant (in the program) */
    char * name;

    /** Real name (used for source generation) */
    char * realName;

    ConstantType    type;

    /** 
     * This flag mark if this variable is imported by external ASM
     */
    int imported;

    /** 
     * The initial (numeric) value of the variable, as given by last (re)definition.
     */
    int value;

    /** 
     * The pointer to the constant string.
     */
    StaticString * valueString;

    /** 
     * The initial (floating) value of the variable, as given by last (re)definition.
     */
    double valueFloating;

    /** Link to the next constant (NULL if this is the last one) */
    struct _Constant * next;

} Constant;

/**
 * @brief Structure of a single label
 */
typedef struct _Label {

    /** Name of the label (if not numeri) */
    char * name;

    /** Line numberf (if numeric) */
    int number;

    /** Link to the next constant (NULL if this is the last one) */
    struct _Label * next;

} Label;

typedef enum _FloatTypePrecision {

    FT_FAST = 0,        // fast = 24 bit
    FT_SINGLE = 1       // single = 32 bit

} FloatTypePrecision;

typedef enum _FloatTypeAngle {

    FT_RADIAN = 0,        // radiants
    FT_DEGREE = 1          // degrees

} FloatTypeAngle;

typedef struct _FloatType {

    FloatTypePrecision precision;
    FloatTypeAngle angle;

} FloatType;

typedef struct _OffsettingVariable {

    int sequence;

    /**
     * Back referenced variable
     */
    struct _Variable * variable;
    
    /** Link to the next offsetting */
    struct _OffsettingVariable * next;

} OffsettingVariable;

typedef struct _Offsetting {

    /**
     * Size of an element
     */
    int size;

    /**
     * Count of elements
     */
    int count;

    /**
     * Back referenced variable
     */
    OffsettingVariable * variables;

    /** Link to the next offsetting */
    struct _Offsetting * next;

} Offsetting;

/**
 * @brief Structure of a single variable
 */
typedef struct _Variable {

    /** Name of the variable (in the program) */
    char * name;

    /** Real name (used for source generation) */
    char * realName;

    /** Only for temporary vars: the meaning for this variable */
    char * meaningName;

    /** Variable type */
    VariableType type;

    /** Precision type (if float) */
    FloatTypePrecision precision;

    /** 
     * This flag mark if this variable is temporary or not 
     */
    int temporary;

    /** 
     * This flag mark if this variable is used (1) or not (0); 
     * it is valid only for temporary one. 
     */
    int used;

    /** 
     * This flag mark if this variable is locked (1) or not (0); 
     * it is valid only for temporary one and avoid to free
     * variables that are still used. 
     */
    int locked;

    /** 
     * This flag mark if this variable is imported by external ASM
     */
    int imported;

    /** 
     * This flag mark if this variable has been assigned.
     * If it is temporary MUST not be outputed as value.
     */
    int assigned;

    /** 
     * The initial value of the variable, as given by last (re)definition.
     */
    int value;

    /** 
     * If this variable has been initialized by a numeric value,
     * this flag will be setted.
     */
    int initializedByConstant;

    /** 
     * The pointer to the constant string.
     */
    StaticString * valueString;

    /** 
     * The static floating's value, as given by last (re)definition.
     */
    double valueFloating;

    /**
     * Position of the bit inside the byte.
     */
    int bitPosition;

    /**
     * Position of the byte inside the (generic) bitstream.
     */
    int bitByte;

    /** 
     * The static buffer's value, as given by last (re)definition.
     */
    unsigned char * valueBuffer;

    /** 
     * Reflected variable.
     */
    unsigned char * reflected;

    /** 
     * The size of the (naive/compressed) static buffer (in bytes).
     */
    int size;

    /** 
     * The size of the (uncompressed) static buffer (in bytes).
     */
    int uncompressedSize;

    /** 
     * The absolute address of this variable (if any).
     */
    int absoluteAddress;

    /** 
     * Is a printable buffer?
     */
    int printable;

    /** 
     * Pointer to the bank where this variable belongs to.
     */
    Bank * bank;

    /** 
     * Pointer to the memory area where this variable belongs to.
     */
    MemoryArea * memoryArea;

    /**
     * Number of dimensions of this array
     */
    int arrayDimensions;

    /**
     * Size of each dimension
     */
    int arrayDimensionsEach[MAX_ARRAY_DIMENSIONS];

    /**
     * Initialization values
     */
    Constant * arrayInitialization;

    /** Variable type */
    VariableType arrayType;

    /** Float precision */
    FloatTypePrecision arrayPrecision;

    /** Is threaded? */
    int threaded;

    /** size of single frame (if IMAGES) */
    int frameSize;

    /** count of frames (if IMAGES) */
    int frameCount;

    int staticalInit;

    /** Original bitmap data (if IMAGE/IMAGES) */
    char * originalBitmap;

    /** Original bitmap width (if IMAGE/IMAGES) */
    int originalWidth;

    /** Original bitmap height (if IMAGE/IMAGES) */
    int originalHeight;

    /** Original bitmap depth (if IMAGE/IMAGES) */
    int originalDepth;

    /** Original bitmap nr. colors (if IMAGE/IMAGES) */
    int originalColors;

    int mapWidth;

    int mapHeight;

    int mapLayers;

    int frameWidth;

    int frameHeight;

    int firstGid;
    
    /** Original bitmap palette (if IMAGE/IMAGES) */
    RGBi originalPalette[MAX_PALETTE];

    /** Bank to be used to store the content of this variable */
    int bankAssigned;

    /** Resident shared assigned to this */
    int residentAssigned;

    /** Unique ID assigned to this variable (is banked) */
    int variableUniqueId;

    /** If VT_IMAGES, this is the original tsx' tileset attached (if used) */
    TsxTileset * originalTileset;

    struct _Variable * tileset;

    /** If VT_TILEMAP, this is the original tmx' tileset attached (if used) */
    TmxMap * originalTilemap;

    /** 
     * This flag mark if this variable is read only (1) or not (0); 
     * read only variables could be stored into non writable memory.
     */
    int readonly;

    /**
     * This flag mark if a variable must be seen as a "memory placeholder"
     * for a data stored into the mass storage.
     */
    int onStorage;

    /**
     * Linked offsetting for each frame
     */
    Offsetting * offsettingFrames;

    /**
     * Linked offsetting for each sequence
     */
    Offsetting * offsettingSequences;

    /** Link to the next variable (NULL if this is the last one) */
    struct _Variable * next;

} Variable;

typedef struct _Procedure {

    /** Name of the procedure (in the program) */
    char * name;

    /**
     * Parameters definition
     */
    int parameters;

    /**
     * Parameters definition
     */
    char * parametersEach[MAX_PARAMETERS];

    /**
     * Parameters definition
     */
    int parametersAsmioEach[MAX_PARAMETERS];

    /**
     * Parameters definition
     */
    int parametersValueEach[MAX_PARAMETERS];

    /**
     * Parameters definition
     */
    VariableType parametersTypeEach[MAX_PARAMETERS];

    /**
     * Is a protothread?
     */
    int protothread;

    /**
     * Is declared?
     */
    int declared;

    /**
     * Is system?
     */
    int system;

    /**
     * Address
     */
    int address;

    /**
     * Temporary storage for (cpu) return definition
     */
    int returns;
    
    /**
     * Temporary storage for (cpu) return
     */
    char * returnsEach[MAX_PARAMETERS];

    /**
     * Temporary storage for (cpu) asmio return
     */
    int returnsAsmioEach[MAX_PARAMETERS];

    /**
     * Temporary storage for (cpu) asmio definition
     */
    VariableType returnsTypeEach[MAX_PARAMETERS];

    /** Link to the next procedure (NULL if this is the last one) */
    struct _Procedure * next;

} Procedure;

/**
 * @brief Types of conditional jumps supported.
 */
typedef enum _ConditionalType {
    /** IF ... THEN ... ENDIF */
    CT_IF = 0,

    /** ON ... GOTO ... */
    CT_ON_GOTO = 1,

    /** ON ... GOSUB ... */
    CT_ON_GOSUB = 2,

    /** ON ... PROC ... */
    CT_ON_PROC = 3,

    /** SELECT ... CASE ... */
    CT_SELECT_CASE = 4

} ConditionalType;

/**
 * @brief Maximum number of conditional types
 */
#define CONDITIONAL_TYPE_COUNT   5

/**
 * @brief Structure of a single conditional jump.
 */
typedef struct _Conditional {

    /** Type of conditional */
    ConditionalType type;

    /** Label to jump. */
    char *label;

    /** Expression to evaluate. */
    Variable *expression;

    /** Incremental index for forced jumps. */
    int index;

    /** In case of CT_SELECT_CASE, case else has been emitted?. */
    int caseElse;

    /** Next conditional */
    struct _Conditional * next;

} Conditional;

/**
 * @brief Types of loops supported.
 */
typedef enum _LoopType {
    /** DO ... LOOP */
    LT_DO = 0,

    /** WHILE ... WEND */
    LT_WHILE = 1,

    /** REPEAT ... UNTIL */
    LT_REPEAT = 2,

    /** FOR ... NEXT */
    LT_FOR = 3,

    /** FOR ... NEXT (multithread) */
    LT_FOR_MT = 4,

    /** BEGIN...END GAMELOOP */
    LT_GAMELOOP = 5

} LoopType;

/**
 * @brief Maximum number of loop types
 */
#define LOOP_TYPE_COUNT   1

/**
 * @brief Structure of a single loop.
 */
typedef struct _Loop {

    /** Type of conditional */
    LoopType type;

    /** Label to jump. */
    char *label;

    /** Variable with index. */
    Variable *index;

    /** Variable with from. */
    Variable *from;

    /** Variable with from (resident). */
    Variable *fromResident;

    /** Variable with to. */
    Variable *to;

    /** Variable with to (resident). */
    Variable *toResident;

    /** Variable with step. */
    Variable *step;

    /** Variable with step (resident). */
    Variable *stepResident;

    /** Variable with zero (0). */
    Variable *zero;

    int statical;
    
    /** Next conditional */
    struct _Loop * next;

} Loop;

typedef struct _Pattern {

    char * pattern;

    /** Next pattern */
    struct _Pattern * next;

} Pattern;

typedef enum _Writing {

    WRITING_REPLACE = 0,
    WRITING_OR = 1,
    WRITING_XOR = 2,
    WRITING_AND = 3,
    WRITING_IGNORE = 4,

    WRITING_PAPER = 1,
    WRITING_PEN = 2,
    WRITING_NORMAL = 3

} Writing;

typedef struct _LoadedFile {

    char * fileName;

    Variable * variable;

    /** Next loaded file */
    struct _LoadedFile * next;

} LoadedFile;

typedef struct _ScreenMode {

    int         id;

    char *      description;

    int         bitmap;

    int         width;

    int         height;

    int         colors;

    int         tileWidth;

    int         tileHeight;

    int         score;

    struct _ScreenMode  * next;

} ScreenMode;

#define SCREEN_MODE_DEFINE( _id, _bitmap, _width, _height, _colors, _tile_width, _tile_height, _description ) \
    { \
        ScreenMode * screenMode = malloc( sizeof( ScreenMode ) ); \
        memset( screenMode, 0, sizeof( screenMode ) ) ; \
        screenMode->bitmap = _bitmap; \
        screenMode->id = _id; \
        screenMode->width = _width; \
        screenMode->height = _height; \
        screenMode->colors = _colors; \
        screenMode->tileWidth = _tile_width; \
        screenMode->tileHeight = _tile_height; \
        screenMode->score = 0; \
        screenMode->description = strdup( _description ); \
        screenMode->next = NULL; \
        ScreenMode * last = _environment->screenModes; \
        if ( last ) { \
            while( last->next ) { \
                last = last->next; \
            } \
            last->next = screenMode; \
        } else { \
            _environment->screenModes = screenMode; \
        } \
    }

typedef struct _Embedded {

    int cpu_beq;
    int cpu_bneq;
    int cpu_busy_wait;
    int cpu_bveq;
    int cpu_bvneq;
    int cpu_combine_nibbles;
    int cpu_compare_16bit;
    int cpu_compare_32bit;
    int cpu_compare_8bit;
    int cpu_compare_and_branch_8bit;
    int cpu_compare_and_branch_16bit;
    int cpu_compare_and_branch_16bit_const;
    int cpu_compare_and_branch_32bit_const;
    int cpu_compare_and_branch_8bit_const;
    int cpu_compare_and_branch_char_const;
    int cpu_di;
    int cpu_ei;
    int cpu_inc;
    int cpu_inc_16bit;
    int cpu_inc_32bit;
    int cpu_dec;
    int cpu_dec_16bit;
    int cpu_less_than_16bit_const;
    int cpu_less_than_32bit_const;
    int cpu_less_than_8bit_const;
    int cpu_less_than_16bit;
    int cpu_less_than_32bit;
    int cpu_less_than_8bit;
    int cpu_greater_than_16bit;
    int cpu_greater_than_32bit;
    int cpu_greater_than_8bit;
    int cpu_fill;
    int cpu_fill_blocks;
    int cpu_halt;
    int cpu_end;
    int cpu_jump;
    int cpu_call;
    int cpu_call_indirect;
    int cpu_return;
    int cpu_pop;
    int cpu_label;
    int cpu_limit_16bit;
    int cpu_logical_not_8bit;
    int cpu_logical_and_8bit;
    int cpu_logical_or_8bit;
    int cpu_not_8bit;
    int cpu_and_8bit;
    int cpu_or_8bit;
    int cpu_xor_8bit;
    int cpu_swap_8bit;
    int cpu_not_16bit;
    int cpu_and_16bit;
    int cpu_or_16bit;
    int cpu_xor_16bit;
    int cpu_swap_16bit;
    int cpu_not_32bit;
    int cpu_and_32bit;
    int cpu_or_32bit;
    int cpu_xor_32bit;
    int cpu_swap_32bit;
    int cpu_math_add_16bit;
    int cpu_math_add_16bit_const;
    int cpu_math_add_16bit_with_16bit;
    int cpu_math_add_16bit_with_8bit;
    int cpu_math_add_32bit;
    int cpu_math_add_32bit_const;
    int cpu_math_add_8bit;
    int cpu_math_add_8bit_const;
    int cpu_math_and_const_16bit;
    int cpu_math_and_const_32bit;
    int cpu_math_and_const_8bit;
    int cpu_math_complement_const_16bit;
    int cpu_math_complement_const_32bit;
    int cpu_math_complement_const_8bit;
    int cpu_math_div2_8bit;
    int cpu_math_div2_const_16bit;
    int cpu_math_div2_const_32bit;
    int cpu_math_div2_const_8bit;
    int cpu_math_double_16bit;
    int cpu_math_double_32bit;
    int cpu_math_double_8bit;
    int cpu_math_mul_16bit_to_32bit;
    int cpu_math_mul_8bit_to_16bit;
    int cpu_math_div_32bit_to_16bit;
    int cpu_math_div_16bit_to_16bit;
    int cpu_math_div_8bit_to_8bit;
    int cpu_math_mul2_const_16bit;
    int cpu_math_mul2_const_32bit;
    int cpu_math_mul2_const_8bit;
    int cpu_math_sub_16bit;
    int cpu_math_sub_32bit;
    int cpu_math_sub_8bit;
    int cpu_math_sub_16bit_with_8bit;
    int cpu_move_16bit;
    int cpu_addressof_16bit;
    int cpu_move_32bit;
    int cpu_move_8bit;
    int cpu_peek;
    int cpu_poke;
    int cpu_random;
    int cpu_random_16bit;
    int cpu_random_32bit;
    int cpu_random_8bit;
    int cpu_store_16bit;
    int cpu_store_32bit;
    int cpu_store_8bit;
    int cpu_store_char;
    int cpu_mem_move;
    int cpu_mem_move_direct;
    int cpu_mem_move_size;
    int cpu_mem_move_direct_size;
    int cpu_mem_move_direct_indirect_size;
    int cpu_compare_memory;
    int cpu_compare_memory_size;
    int cpu_less_than_memory;
    int cpu_less_than_memory_size;
    int cpu_greater_than_memory;
    int cpu_greater_than_memory_size;
    int cpu_uppercase;
    int cpu_lowercase;
    int cpu_convert_string_into_16bit;
    int cpu_fill_indirect;
    int cpu_flip;
    int cpu_move_8bit_indirect;
    int cpu_move_8bit_indirect2;
    int cpu_move_8bit_indirect2_8bit;
    int cpu_move_8bit_indirect2_16bit;
    int cpu_move_16bit_indirect;
    int cpu_move_16bit_indirect2;
    int cpu_move_16bit_indirect2_8bit;
    int cpu_move_32bit_indirect;
    int cpu_move_32bit_indirect2;
    int cpu_bit_check;
    int cpu_bit_inplace;
    int cpu_number_to_string;
    int cpu_move_8bit_indirect_with_offset;
    int cpu_bits_to_string;
    int cpu_hex_to_string;
    int cpu_bit_check_extended;
    int cpu_move_8bit_indirect_with_offset2;
    int cpu_dsdefine;
    int cpu_dsalloc;
    int cpu_dsfree;
    int cpu_dswrite;
    int cpu_dsresize;
    int cpu_dsresize_size;
    int cpu_dsgc;
    int cpu_dsdescriptor;
    int cpu_move_8bit_with_offset;
    int cpu_move_8bit_with_offset2;
    int cpu_store_8bit_with_offset;
    int cpu_dsalloc_size;
    int cpu_complement2_8bit;
    int cpu_complement2_16bit;
    int cpu_complement2_32bit;
    int cpu_sqroot;
    int cpu_is_negative;
    int cpu_msc1_uncompress;
    int cpu_string_sub;

} Embedded;

typedef struct _Deployed {

    int vbl;
    int joystick;
    int keyboard;
    int sprite;
    int sqr;
    int back;
    int vars;
    int startup;
    int c6847startup;
    int c6847vars;
    int bitmap;
    int vic1vars;
    int vic1startup;
    int vic2vars;
    int vic2varsGraphic;
    int vic2startup;
    int vic2zvars;
    int vic2zvarsGraphic;
    int vic2zstartup;
    int vdcvars;
    int vdcvarsGraphic;
    int vdcstartup;
    int vdczvars;
    int vdczvarsGraphic;
    int vdczstartup;
    int tedvars;
    int tedvarsGraphic;
    int tedstartup;
    int anticstartup;
    int anticvars;
    int gtiastartup;
    int gtiavars;
    int gtiavarsGraphic;
    int gtiapreproc;
    int gimevars;
    int gimestartup;
    int zxvars;
    int msx1vars;
    int sc3000vars;
    int sg1000vars;
    int vg5000vars;
    int colecovars;
    int cpcvars;
    int cpcvarsGraphic;
    int cpcstartup;
    int c128zvars;
    int c128zvarsGraphic;
    int c128zstartup;
    int tms9918vars;
    int tms9918varsGraphic;
    int tms9918startup;
    int ef936xvars;
    int ef936xstartup;
    int ef9345vars;
    int ef9345startup;
    int plot;
    int dstring;
    int scancode;
    int textEncodedAt;
    int textEncodedAtText;
    int textEncodedAtGraphic;
    int numberToString;
    int bitsToString;
    int vScroll;
    int vScrollText;
    int vScrollTextUp;
    int vScrollTextDown;
    int textVScrollScreen;
    int cls;
    int clsText;
    int clsGraphic;
    int textCline;
    int textHScroll;
    int textHScrollLine;
    int textHScrollScreen;
    int scroll;
    int raster;
    int putimage;
    int getimage;
    int puttilemap;
    int blitimage;
    int sliceimagecopy;
    int sliceimageextract;
    int protothread;
    int tiles;
    int font;
    int sidvars;
    int sidstartup;
    int pc128audio;
    int doubleBuffer;
    int pokeyvars;
    int pokeystartup;
    int ay8910vars;
    int ay8910startup;
    int sn76489vars;
    int sn76489startup;

    int draw;
    int bar;

    Embedded embedded;

    int fp_vars;

    int fp_mul4;
    int fp_mul24;
    int fp_mul16;
    int fp_mul24_stack_based;
    int fp_pushpop;
    int fp_c_times_bde;
    int fp_mov4;
    int fp_common_str;
    int fp_div24_24;
    int fp_sqrt24_mant;
    int fp_sqrt32;
    int fp_div32_16;
    
    int fp_fast_mul;
    int fp_fast_to_string;
    int fp_fast_pow10_lut;
    int fp_format_str;
    int fp_fast_from_16;
    int fp_fast_from_8;
    int fp_fast_to_16;
    int fp_fast_to_8;
    int fp_fast_add;
    int fp_fast_sub;
    int fp_fast_div;
    int fp_fast_cmp;
    int fp_fast_sin;
    int fp_fast_cos;
    int fp_fast_sqr;
    int fp_fast_mod1;
    int fp_fast_neg;
    int fp_fast_abs;
    int fp_fast_tan;
    int fp_fast_bg;
    int fp_fast_amean;
    int fp_fast_geomean;
    int fp_fast_div_pow2;

    int fp_single_vars;
    int fp_single_mul;
    int fp_single_pow10_lut;
    int fp_single_mul24;
    int fp_single_to_string;
    int fp_single_from_16;
    int fp_single_from_8;
    int fp_single_to_16;
    int fp_single_to_8;
    int fp_single_add;
    int fp_single_sub;
    int fp_single_div;
    int fp_single_cmp;
    int fp_single_sin;
    int fp_single_cos;
    int fp_single_mod1;
    int fp_single_neg;
    int fp_single_horner_step;
    int fp_single_abs;
    int fp_single_tan;
    int fp_single_bgi;
    int fp_single_sqrt;
    int fp_single_mulpow2;
    int fp_single_amean;
    int fp_single_mulu8_divpow2;
    int fp_single_geomean;
    
    int duff;

    int read_data_unsafe;
    int irq;
    int draw_string;
    int paint;
    int play_string;
    int put_tilemap;

    int timer;

    int dcommon;
    int dload;
    int dsave;

} Deployed;

typedef struct _DString {

    int count;
    int space;

} DString;

typedef struct _ProtothreadConfig {

    int count;

} ProtothreadConfig;

typedef struct _InputConfig {

    char separator;
    int size;
    char cursor;
    char rate;
    char delay;

} InputConfig;

typedef struct _VestigialConfig {

    char screenModeUnique;
    char doubleBufferSelected;
    char doubleBuffer;
    char palettePreserve;

} VestigialConfig;

typedef struct _FontConfig {

    int schema;

} FontConfig;

typedef struct _Macro {

    char * name;
    char * parameters[MAX_TEMPORARY_STORAGE];
    int parameterCount;
    char * lines[MAX_TEMPORARY_STORAGE];
    int lineCount;
    struct _Macro * next;
    
} Macro;

typedef struct _EmbedResult {

    char * line;
    int current;
    int excluded[MAX_NESTED_ARRAYS];
    int conditional;
    Macro * macro;
    Macro * currentMacro;
    char * values[MAX_TEMPORARY_STORAGE];
    int valueCount;
    char * lines[MAX_TEMPORARY_STORAGE];
    int lineCount;

} EmbedResult;

typedef struct _TileDescriptor {

    int whiteArea;
    int horizontalEdges[8];
    int verticalEdges[8];

} TileDescriptor;

typedef struct _TileData {

    char data[8];

} TileData;

typedef struct _TileDescriptors {

    int                 first;
    int                 firstFree;
    int                 lastFree;
    int                 count;

    TileDescriptor *    descriptor[256];
    TileData            data[256];

} TileDescriptors;

typedef int (*RgbConverterFunction)(int, int, int);

extern int yycolno;
extern int yyposno;

typedef struct _Blit {

    char * name;
    char * realName;
    int freeRegisters;
    int usedMemory;
    int sourceCount;
    char * sources[MAX_TEMPORARY_STORAGE];

} Blit;

typedef struct _DataDataSegment {
    
    VariableType type;
    FloatTypePrecision precision;
    char * data;
    int size;

    struct _DataDataSegment * next;

} DataDataSegment;

typedef struct _DataSegment {
    
    VariableType type;

    int isNumeric;

    char * name;
    char * realName;
    int lineNumber;

    DataDataSegment * data;

    struct _DataSegment * next;

} DataSegment;

/**
 * @brief Structure of compilation environment
 * 
 * @todo implement DEF FN
 * @todo implement PROCEDURE
 */
typedef struct _Environment {

    // TODO: implement DEF FN

    /* --------------------------------------------------------------------- */
    /* INPUT PARAMETERS                                                      */
    /* --------------------------------------------------------------------- */

    /**
     * Filename of BAS source (*.bas) 
     */
    char * sourceFileName;

    /**
     * Filename of ASM source (*.asm) 
     */
    char * asmFileName;

    /**
     * Filename of EXE source (extension depends on target and output format) 
     */
    char * exeFileName;

    /**
     * Filename of linker's configuration file (*.cfg) 
     */
    char * configurationFileName;

    /**
     * Filename of debugger's labels file (*.lb2) 
     */
    char * debuggerLabelsFileName;

    /**
     * Filename of assembly listing file (*.lst) 
     */
    char * listingFileName;

    /**
     * Filename of profiled listing file (*.profile) 
     */
    char * profileFileName;

    /**
     * Filename of executer
     */
    char * executerFileName;

    /**
     * Filename of compiler 
     */
    char * compilerFileName;

    /**
     * Filename of app maker 
     */
    char * appMakerFileName;

    /**
     * Filename of decb
     */
    char * decbFileName;

    /**
     * Filename of dir2atr
     */
    char * dir2atrFileName;

    /**
     * Filename of dsktools
     */
    char * dsktoolsFileName;

    /**
     * Filename of additional information file
     */
    char * additionalInfoFileName;

    /**
     * TemporaryPath 
     */
    char * temporaryPath;

    /**
     * 
     */
    int analysis;

    /**
     * Maximum number of cycles for peep hole optimizations (0 = disable)
     */
    int peepholeOptimizationLimit;

    /**
     * Maximum number of cycles for profiling.
     */
    int profileCycles;

    /**
     * Enable the visualization of warnings during compilation.
     */
    int warningsEnabled;

    /**
     * Enable the installation of chain tool.
     */
    int installChainTool;

    /**
     * List of embedded methods
     */
    Embedded embedded;

    /**
     * Stats about usage of embedded methods
     */
    Embedded embeddedStats;

    /**
     * 
     */
    FloatType floatType;

    /**
     * 
     */
    DString dstring;

    /**
     * 
     */
    FontConfig fontConfig;

    /**
     * 
     */
    ProtothreadConfig protothreadConfig;

    /**
     * 
     */
    InputConfig inputConfig;

    /**
     * 
     */
    VestigialConfig vestigialConfig;

    /**
     * 
     */
    EmbedResult embedResult;

    /**
     * Type of output. 
     */
    OutputFileType outputFileType;

    /*
     * Variables must be explicitly defined?
     */
    int optionExplicit;

    /*
     * Graphical operation has to be clipped?
     */
    int optionClip;

    /*
     * Read operation has to be safe?
     */
    int optionReadSafe;

    Blit blit;

    /**
     * Precalculated blits
     */

    char * blitAND;
    char * blitOR;
    char * blitNOT;
     
    /* --------------------------------------------------------------------- */
    /* INTERNAL STRUCTURES                                                   */
    /* --------------------------------------------------------------------- */

    /**
     * Current line parsed.
     */
    char * parsedLine;

    /**
     * Current line number in the BAS file.
     */
    int yylineno;

    /**
     * Last unique identification number 
     * (used for temporary variables and labels)
     */
    int uniqueId;

    /**
     * Last unique identification number 
     * (used for image and file resources)
     */
    int uniqueResourceId;

    /**
     * Set of banks defined during compilation. 
     * It contains all the banks, divided by type.
     */
    Bank * banks[BANK_TYPE_COUNT];

    ScreenMode * screenModes;

    // /**
    //  * Largest variable used as transient memory area.
    //  */
    // Variable * storageTransientMemoryArea;

    /**
     * Set of storages
     */
    Storage * storage;

    /**
     * Current storage
     */
    Storage * currentStorage;

    /**
     * Current file storage
     */
    FileStorage * currentFileStorage;

    /**
     * List of labels.
     */
    Label * labels;

    /**
     * List of dataSegments.
     */
    DataSegment * dataSegment;

    /**
     * If any dynamic RESTORE is used
     */
    int restoreDynamic;

    /**
     * If any read was used
     */
    int readDataUsed;

    /**
     * Last label defined
     */
    char * lastDefinedLabel;

    int lastDefinedLabelNumeric;

    /**
     * Last label defined is numeric
     */
    int lastDefinedLabelIsNumeric;

    /**
     * List of temporary (but not reusable) variables.
     */
    Variable * tempResidentVariables;

    /**
     * Current procedure (for temporary variables),
     * 0 = main program
     */
    int currentProcedure;
    
    /**
     * List of temporary (and reusable) variables (per procedure).
     */
    Variable * tempVariables[MAX_PROCEDURES];

    /**
     * List of constants defined in the program.
     */
    Constant * constants;

    /**
     * List of variables defined in the program.
     */
    Variable * variables;

    /**
     * List of procedures defined in the program.
     */
    Procedure * procedures;

    /**
     * List of variables defined in the procedure.
     */
    Variable * procedureVariables;

    /**
     * List of patterns for GLOBAL / SHARED variables
     */
    Pattern * globalVariablePatterns;

    /**
     * List of loaded files
     */
    LoadedFile * loadedFiles;

    /**
     * List of strings
     */
    StaticString * strings;

    /**
     * This flag marks if the program has opened a "game loop".
     */
    int hasGameLoop;

    /**
     * This flag marks if the program has called "run parallel"
     * at least once.
     */
    int runParallel;

    /**
     * This flag marks if the program needs a BITMASK/BITMASKN temporary variable.
     */
    int bitmaskNeeded;

    /**
     * List of (currently opened) conditionals.
     */
    Conditional * conditionals;

    /**
     * List of (currently opened) loops.
     */
    Loop * loops;

    // /**
    //  * "Every" status
    //  */
    // Variable * everyStatus;

    // /**
    //  * "Every" counter
    //  */
    // Variable * everyCounter;

    // /**
    //  * "Every" timing
    //  */
    // Variable * everyTiming;

    /**
     * Memory areas available for the specific platform
     */
    MemoryArea * memoryAreas;

    /**
     * Current graphical mode
     */
    int currentMode;

    /**
     * Current bitwidth for current mode
     */
    int currentModeBW;

    /**
     * Current tile / bitmap mode
     */
    int currentTileMode;

    /**
     * Current RGB converter
     */
    RgbConverterFunction currentRgbConverterFunction;

    /**
     * Temporary storage for array definition
     */
    int arrayDimensions;

    /**
     * Temporary storage for array definition
     */
    int arrayDimensionsEach[MAX_ARRAY_DIMENSIONS];

    /**
     * Actual index of nested array access
     */
    int arrayNestedIndex;

    /**
     * Temporary storage for array access
     */
    int arrayIndexes[MAX_NESTED_ARRAYS];

    /**
     * Temporary storage for array access
     */
    char * arrayIndexesEach[MAX_NESTED_ARRAYS][MAX_ARRAY_DIMENSIONS];

    /**
     * Temporary storage for array access (using constants)
     */
    int arrayIndexesDirectEach[MAX_NESTED_ARRAYS][MAX_ARRAY_DIMENSIONS];

    /**
     * Temporary storage for current array
     */
    Variable * currentArray;

    /**
     * Temporary storage for current sprite
     */
    char * currentSprite;

    /**
     * Temporary storage for current sprite number
     */
    int currentSpriteNumber;

    /**
     * Temporary storage for lower limit
     */
    char * lowerLimit;

    /**
     * Temporary storage for upper limit
     */
    char * upperLimit;

    /**
     * Current procedure
     */
    char * procedureName;

    /**
     * Temporary storage for address
     */
    int address;

    /**
     * Temporary storage for parameters definition
     */
    int parameters;

    /**
     * Temporary storage for parameters definition
     */
    char * parametersEach[MAX_PARAMETERS];

    /**
     * Temporary storage for (cpu) asmio
     */
    int parametersAsmioEach[MAX_PARAMETERS];

    /**
     * Temporary storage for parameters definition
     */
    VariableType parametersTypeEach[MAX_PARAMETERS];

    /**
     * Temporary storage for (cpu) direct value
     */
    int parametersValueEach[MAX_PARAMETERS];

    /**
     * Temporary storage for (cpu) return definition
     */
    int returns;

    /**
     * Temporary storage for (cpu) return
     */
    char * returnsEach[MAX_PARAMETERS];

    /**
     * Temporary storage for (cpu) asmio return
     */
    int returnsAsmioEach[MAX_PARAMETERS];

    /**
     * Temporary storage for (cpu) asmio definition
     */
    VariableType returnsTypeEach[MAX_PARAMETERS];

    /**
     * Temporary storage for protothread definition
     */
    int protothread;

    /**
     * Has at least one parallel procedure?
     */
    int anyProtothread;

    /**
     * Step when resuming the protothread
     */
    int protothreadStep;
    
    /**
     * Is protothread forbidden?
     */
    int protothreadForbid;

    /**
     * 
     */
    VariableType dataDataType;

    /**
     * Screen width in pixels (statically determined)
     */
    int screenWidth;

    /**
     * Screen height in pixels (statically determined)
     */
    int screenHeight;

    /**
     * Screen shades (statically determined)
     */
    int screenShades;

    /**
     * Screen colors (statically determined)
     */
    int screenColors;

    /**
     * Number of tiles accessible.
     */
    int screenTiles;

    /**
     * Screen width in characters (statically determined)
     */
    int screenTilesWidth;

    /**
     * Screen height in characters (statically determined)
     */
    int screenTilesHeight;

    /**
     * Font width in pixels (statically determined)
     */
    int fontWidth;

    /**
     * Font height in pixels (statically determined)
     */
    int fontHeight;

    /**
     * Deployed modules.
     */
    Deployed deployed;

    /**
     * Enable stats of embedded modules
     */

    int embeddedStatsEnabled;

    /**
     * If true, the body of procedure will not be produced
     */
    int emptyProcedure;

    /**
     * List of offset table to generate.
     */
    Offsetting * offsetting;

    TileDescriptors * descriptors;

    /**
     * Actual number of tilesets defined
     */
    int tilesetCount;

    /**
     * Data of each tileset
     */
    TileDescriptors * tilesets[MAX_TILESETS];

    /**
     * Debug during LOAD IMAGE.
     */
    int debugImageLoad;
    
    /**
     * Default type for variables.
     */
    VariableType defaultVariableType;

    /**
     * Current palette selection.
     */
    int paletteSelected;

    /**
     * Current palette index.
     */
    int paletteIndex;

    /**
     * Is original source included?
     */
    int tenLinerRulesEnforced;

    /**
     * Is running in sandbox?
     */
    int sandbox;

    /**
     * Is double buffering enabled?
     */
    int doubleBufferEnabled;

    /*
     * Gamma correction to be used.
     */
    GammaCorrection gammaCorrection;

    /*
     * List of available banks
     */
    Bank * expansionBanks;

    /*
     * Max size of a single block allocated
     */
    int maxExpansionBankSize[MAX_RESIDENT_SHAREDS];

    /*
     * Number of assembly lines produced until now.
     */
    int producedAssemblyLines;

    /*
     * Number of assembly lines produced until the previous step.
     */
    int previousProducedAssemblyLines;

    /*
     * Current source line analyzed by peephole optimizer.
     */
    int currentSourceLineAnalyzed;

    /*
     * Number of assembly lines removed for currentSourceLine.
     */
    int removedAssemblyLines;

    /*
     * Number of bytes produced for currentSourceLine
     */
    int bytesProduced;

    /*
     * Origin is used
     */
    int originUsed;

    /*
     * Origin's abscissa
     */
    int originX;

    /*
     * Origin's ordinate
     */
    int originY;

    /*
     * Origin axis ordinate
     */
    int originYDirection;

    /*
     * Resolution's used
     */
    int resolutionUsed;

    /*
     * Resolution's abscissa
     */
    int resolutionX;

    /*
     * Resolution's ordinate
     */
    int resolutionY;

    /*
     * slice image starting X
     */
    char * sliceImageX;

    /*
     * slice image starting y
     */
    char * sliceImageY;

    int defaultUnsignedType;

    int defaultNarrowType;

    /*
     * Used for deferred writing of assembly file.
     */
    char *deferredEmbedded[MAX_TEMPORARY_STORAGE];

    int deferredEmbeddedSize[MAX_TEMPORARY_STORAGE];

    char * threadIdentifier[MAX_TEMPORARY_STORAGE];

    int lastThreadIdentifierUsed;

    char * soundNote[MAX_TEMPORARY_STORAGE];
    int soundNoteValue[MAX_TEMPORARY_STORAGE];
    char * soundDuration[MAX_TEMPORARY_STORAGE];
    int soundDurationValue[MAX_TEMPORARY_STORAGE];
    int lastSoundNoteDuration;
    int atLeastOneSoundNoteDurationSymbolic;

    /*
     * Starting address of frame buffer
     */
    int frameBufferStart;

    /*
     * Starting address of auxiliary frame buffer
     */
    int frameBufferStart2;


    /**
     * 
     */
    int frameWidth;

    /**
     * 
     */
    int frameHeight;

    /**
     * Position of the next bit allocable.
     */
    int bitPosition;

    /**
     * Position of the next byte allocable.
     */
    int bitByte;

    /**
     * Size of the paint bucket
     */
    int paintBucketSize;

    /* --------------------------------------------------------------------- */
    /* OUTPUT PARAMETERS                                                     */
    /* --------------------------------------------------------------------- */

    /**
     * Handle to the file opened to write the ASM source (*.asm).
     */
    FILE * asmFile;

    /**
     * Handle to the file opened to write the linker configuration file (*.cfg).
     */
    FILE * configurationFile;

    /**
     * Handle to the file opened to write the list of labels (*.lb2).
     */
    FILE * debuggerLabelsFile;

    /**
     * Handle to the file opened to write the assembly listing.
     */
    FILE * listingFile;

    /**
     * Handle to the file opened to write the number of assembly lines 
     * for each ugBASIC line.
     */
    FILE * additionalInfoFile;


} Environment;

#define UNIQUE_ID            ((struct _Environment *)_environment)->uniqueId++
#define UNIQUE_RESOURCE_ID   ((struct _Environment *)_environment)->uniqueResourceId++
#define MAKE_LABEL  char label[32]; sprintf( label, "_label%d", UNIQUE_ID);

#define CRITICAL( s ) fprintf(stderr, "CRITICAL ERROR during compilation of %s:\n\t%s at %d column %d (%d)\n", ((struct _Environment *)_environment)->sourceFileName, s, ((struct _Environment *)_environment)->yylineno, (yycolno+1), (yyposno+1) ); target_cleanup( ((struct _Environment *)_environment) ); exit( EXIT_FAILURE );
#define CRITICAL2( s, v ) fprintf(stderr, "CRITICAL ERROR during compilation of %s:\n\t%s (%s) at %d column %d (%d)\n",  ((struct _Environment *)_environment)->sourceFileName, s, v, ((struct _Environment *)_environment)->yylineno, (yycolno+1), (yyposno+1) ); target_cleanup( ((struct _Environment *)_environment) ); exit( EXIT_FAILURE );
#define CRITICAL2i( s, v ) fprintf(stderr, "CRITICAL ERROR during compilation of %s:\n\t%s (%d) at %d column %d (%d)\n", ((struct _Environment *)_environment)->sourceFileName, s, v, ((struct _Environment *)_environment)->yylineno, (yycolno+1), (yyposno+1) ); target_cleanup( ((struct _Environment *)_environment) ); exit( EXIT_FAILURE );
#define CRITICAL3( s, v1, v2 ) fprintf(stderr, "CRITICAL ERROR during compilation of %s:\n\t%s (%s, %s) at %d column %d (%d)\n", ((struct _Environment *)_environment)->sourceFileName, s, v1, v2, ((struct _Environment *)_environment)->yylineno, (yycolno+1), (yyposno+1) ); target_cleanup( ((struct _Environment *)_environment) ); exit( EXIT_FAILURE );
#define CRITICAL3i( s, v1, v2 ) fprintf(stderr, "CRITICAL ERROR during compilation of %s:\n\t%s (%s, %d) at %d column %d (%d)\n", ((struct _Environment *)_environment)->sourceFileName, s, v1, v2, ((struct _Environment *)_environment)->yylineno, (yycolno+1), (yyposno+1) ); target_cleanup( ((struct _Environment *)_environment) ); exit( EXIT_FAILURE );
#define CRITICAL4si( s, v, d1, d2 ) fprintf(stderr, "CRITICAL ERROR during compilation of %s:\n\t%s (%s, %d, %d) at %d column %d (%d)\n", ((struct _Environment *)_environment)->sourceFileName, s, v, d1, d2, ((struct _Environment *)_environment)->yylineno, (yycolno+1), (yyposno+1) ); target_cleanup( ((struct _Environment *)_environment) ); exit( EXIT_FAILURE );
#define CRITICAL_UNIMPLEMENTED( v ) CRITICAL2("E000 - Internal method not implemented:", v );
#define CRITICAL_TEMPORARY2( v ) CRITICAL2("E001 - Unable to create space for temporary variable", v );
#define CRITICAL_VARIABLE( v ) CRITICAL2("E002 - Using of an undefined variable", v );
#define CRITICAL_DATATYPE_UNSUPPORTED( k, v ) CRITICAL3("E003 - Datatype not supported for keyword", k, v );
#define CRITICAL_CANNOT_CAST( t1, t2 ) CRITICAL3("E004 - Cannot cast types", t1, t2 );
#define CRITICAL_STORE_UNSUPPORTED( t ) CRITICAL2("E005 - Datatype cannot be stored directly", t );
#define CRITICAL_RESIZE_UNSUPPORTED( t ) CRITICAL2("E006 - Datatype cannot be resized directly", t );
#define CRITICAL_MOVE_NAKED_UNSUPPORTED( t ) CRITICAL2("E007 - Datatype cannot be copied directly", t );
#define CRITICAL_BUFFER_SIZE_MISMATCH( v1, v2 ) CRITICAL3("E008 - Buffer sizes mismatch -- cannot be copied", v1, v2 );
#define CRITICAL_DATATYPE_MISMATCH( v1, v2 ) CRITICAL3("E009 - Datatype mismatch", v1, v2 );
#define CRITICAL_ADD_UNSUPPORTED( v, t ) CRITICAL3("E010 - Add unsupported for variable of given datatype", v, t );
#define CRITICAL_SUB_UNSUPPORTED( v, t ) CRITICAL3("E011 - Sub unsupported for variable of given datatype", v, t );
#define CRITICAL_COMPLEMENT_UNSUPPORTED( v, t ) CRITICAL3("E012 - Complement unsupported for variable of given datatype", v, t );
#define CRITICAL_MUL_UNSUPPORTED( v, t ) CRITICAL3("E013 - Multiplication unsupported for variable of given datatype", v, t );
#define CRITICAL_DIV_UNSUPPORTED( v, t ) CRITICAL3("E014 - Division unsupported for variable of given datatype", v, t );
#define CRITICAL_CANNOT_COMPARE( t1, t2 ) CRITICAL3("E015 - Cannot compare types", t1, t2 );
#define CRITICAL_MUL2_UNSUPPORTED( v, t ) CRITICAL3("E016 - Double unsupported for variable of given datatype", v, t );
#define CRITICAL_DIV2_UNSUPPORTED( v, t ) CRITICAL3("E017 - Division by 2 unsupported for variable of given datatype", v, t );
#define CRITICAL_AND_UNSUPPORTED( v, t ) CRITICAL3("E018 - Bitwise AND unsupported for variable of given datatype", v, t );
#define CRITICAL_LEFT_UNSUPPORTED( v, t ) CRITICAL3("E019 - LEFT unsupported for variable of given datatype", v, t );
#define CRITICAL_RIGHT_UNSUPPORTED( v, t ) CRITICAL3("E020 - RIGHT unsupported for variable of given datatype", v, t );
#define CRITICAL_MID_UNSUPPORTED( v, t ) CRITICAL3("E021 - MID unsupported for variable of given datatype", v, t );
#define CRITICAL_INSTR_UNSUPPORTED( v, t ) CRITICAL3("E022 - INSTR unsupported for variable of given datatype", v, t );
#define CRITICAL_STRING_UNSUPPORTED( v, t ) CRITICAL3("E023 - STRING unsupported for variable of given datatype", v, t );
#define CRITICAL_UPPER_UNSUPPORTED( v, t ) CRITICAL3("E024 - UPPER unsupported for variable of given datatype", v, t );
#define CRITICAL_LOWER_UNSUPPORTED( v, t ) CRITICAL3("E025 - LOWER unsupported for variable of given datatype", v, t );
#define CRITICAL_STR_UNSUPPORTED( v, t ) CRITICAL3("E026 - STR unsupported for variable of given datatype", v, t );
#define CRITICAL_VAL_UNSUPPORTED( v, t ) CRITICAL3("E027 - VAL unsupported for variable of given datatype", v, t );
#define CRITICAL_CHR_UNSUPPORTED( v, t ) CRITICAL3("E028 - CHR unsupported for variable of given datatype", v, t );
#define CRITICAL_ASC_UNSUPPORTED( v, t ) CRITICAL3("E029 - ASC unsupported for variable of given datatype", v, t );
#define CRITICAL_LEN_UNSUPPORTED( v, t ) CRITICAL3("E030 - LEN unsupported for variable of given datatype", v, t );
#define CRITICAL_POW_UNSUPPORTED( v, t ) CRITICAL3("E031 - ^ unsupported for variable of given datatype", v, t );
#define CRITICAL_SGN_UNSUPPORTED( v, t ) CRITICAL3("E032 - SGN unsupported for variable of given datatype", v, t );
#define CRITICAL_ABS_UNSUPPORTED( v, t ) CRITICAL3("E033 - ABS unsupported for variable of given datatype", v, t );
#define CRITICAL_DEBUG_UNSUPPORTED( v, t ) CRITICAL3("E034 - DEBUG unsupported for variable of given datatype", v, t );
#define CRITICAL_ARRAY_SIZE_MISMATCH( v, d1, d2 ) CRITICAL4si("E035 - number of indexes different from array dimensions", v, d1, d2 );
#define CRITICAL_NOT_ARRAY( v ) CRITICAL2("E036 - accessing with indexes on a non array variable", v );
#define CRITICAL_PROCEDURE_NESTED_UNSUPPORTED( n ) CRITICAL2("E037 - cannot define a nested procedure (a procedure inside a procedure)", n ); 
#define CRITICAL_PROCEDURE_NOT_OPENED() CRITICAL("E038 - END PROC outside a procedure" ); 
#define CRITICAL_PROCEDURE_MISSING( n ) CRITICAL2("E039 - call to an undefined procedure", n ); 
#define CRITICAL_PROCEDURE_PARAMETERS_MISMATCH( n, d1, d2 ) CRITICAL4si("E040 - wrong number of parameters on procedure call", n, d1, d2 ); 
#define CRITICAL_SHARED_ONLY_IN_PROCEDURES() CRITICAL("E041 - SHARED can be used only inside a PROCEDURE");
#define CRITICAL_GLOBAL_ONLY_OUTSIDE_PROCEDURES() CRITICAL("E042 - GLOBAL can be used only outside a PROCEDURE");
#define CRITICAL_PRINT_UNSUPPORTED(v, t) CRITICAL3("E043 - PRINT unsupported for variable of given datatype", v, t );
#define CRITICAL_NOT_SUPPORTED( v ) CRITICAL2("E044 - Command / Keyword not supported:", v );
#define CRITICAL_BIT_UNSUPPORTED( v, t ) CRITICAL3("E045 - BIT unsupported for variable of given datatype", v, t );
#define CRITICAL_INPUT_UNSUPPORTED( v, t ) CRITICAL3("E046 - INPUT unsupported for variable of given datatype", v, t );
#define CRITICAL_MOVE_UNSUPPORTED( t ) CRITICAL2("E047 - Datatype cannot be copied", t );
#define CRITICAL_SCREEN_MODE_BITMAP_UNSUPPORTED( t ) CRITICAL2("E048 - Screen mode unsupported for BITMAP mode", t );
#define CRITICAL_SCREEN_MODE_TILEMAP_UNSUPPORTED( t ) CRITICAL2("E049 - Screen mode unsupported for TILEMAP mode", t );
#define CRITICAL_RANDOM_UNSUPPORTED(v, t) CRITICAL3("E050 - RANDOM unsupported for variable of given datatype", v, t );
#define CRITICAL_MOD_UNSUPPORTED(v, t) CRITICAL3("E051 - MOD unsupported for variable of given datatype", v, t );
#define CRITICAL_SCREEN_UNSUPPORTED(v) CRITICAL2i("E052 - SCREEN mode unsupported", v );
#define CRITICAL_LOAD_MISSING_FILE(f) CRITICAL2("E053 - LOAD missing file", f );
#define CRITICAL_LOAD_FILE_TOO_LONG(f) CRITICAL2("E054 - LOAD file too long (>255 bytes)", f );
#define CRITICAL_CANNOT_CAST_BUFFER_STRING_SIZE(a,b) CRITICAL3("E055 - Cannot cast BUFFER to STRING: buffer too long (>255 bytes)", a, b );
#define CRITICAL_IMAGE_LOAD_MISSING_FILE(f) CRITICAL2("E056 - LOAD IMAGE missing file", f );
#define CRITICAL_IMAGE_LOAD_UNKNOWN_FORMAT(f) CRITICAL2("E057 - LOAD IMAGE file format unknown", f );
#define CRITICAL_IMAGE_CONVERTER_TOO_COLORS(f) CRITICAL2i("E059 - IMAGE converter unsupported -- too much colors", f );
#define CRITICAL_SQR_UNSUPPORTED( v, t ) CRITICAL3("E060 - SQR unsupported for variable of given datatype", v, t );
#define CRITICAL_UNDEFINED_CONSTANT( c ) CRITICAL2("E061 - use of an undefined constant", c );
#define CRITICAL_TYPE_MISMATCH_CONSTANT_NUMERIC( c ) CRITICAL2("E062 - use of an wrong type constant (numeric expected, string used)", c );
#define CRITICAL_IMAGE_CONVERTER_INVALID_WIDTH( w ) CRITICAL2i("E063 - invalid width for image, must be multiple of 8 pixels", w );
#define CRITICAL_IMAGE_CONVERTER_INVALID_HEIGHT( h ) CRITICAL2i("E064 - invalid height for image, must be multiple of 8 pixels", h );
#define CRITICAL_BIN_UNSUPPORTED( v, t ) CRITICAL3("E065 - BIN unsupported for variable of given datatype", v, t );
#define CRITICAL_MUL2_INVALID_STEPS( v ) CRITICAL2("E066 - invalid steps for multiplication by 2", v );
#define CRITICAL_UNABLE_TO_EMBED( v ) CRITICAL2("E067 - unable to embed library, only inline available", v );
#define CRITICAL_UNABLE_TO_INLINE( v ) CRITICAL2("E068 - unable to inline call, only library available", v );
#define CRITICAL_NOT_IMAGE( v ) CRITICAL2("E069 - variable is not an image", v );
#define CRITICAL_NOT_ASSIGNED_IMAGE( v ) CRITICAL2("E070 - variable is not an loaded image, please use assign operator", v );
#define CRITICAL_NEGATIVE_CONSTANT( v ) CRITICAL2("E071 - negative constant is not allowed", v);
#define CRITICAL_TOO_LITTLE_CONSTANT( v ) CRITICAL2("E072 - constant value under the minimum limit", v);
#define CRITICAL_TOO_BIG_CONSTANT( v ) CRITICAL2("E073 - constant value over the maximum limit", v);
#define CRITICAL_INVALID_STRING_COUNT( d ) CRITICAL2i("E074 - invalid maximum number of strings", d);
#define CRITICAL_INVALID_STRING_SPACE( d ) CRITICAL2i("E075 - invalid maximum space occupied by strings", d);
#define CRITICAL_TYPE_MISMATCH_CONSTANT_STRING( c ) CRITICAL2("E076 - use of an wrong type constant (string expected, numeric used)", c );
#define CRITICAL_CANNOT_OPEN_EXECUTABLE_FILE( c )  CRITICAL2("E077 - cannot open executable file for post elaboration", c );
#define CRITICAL_PARALLEL_PROCEDURE_CANNOT_BE_CALLED( c ) CRITICAL2("E078 - cannot CALL a PARALLEL PROCEDURE: use SPAWN instead", c );
#define CRITICAL_PROCEDURE_CANNOT_BE_INVOKED( c ) CRITICAL2("E078 - cannot SPAWN a PROCEDURE: use CALL instead", c );
#define CRITICAL_LOCAL_VARIABLE_OUTSIDE_PROCEDURE( c )  CRITICAL2("E079 - cannot define LOCAL vars outside PARALLEL PROCEDURE", c );
#define CRITICAL_OR_UNSUPPORTED( v, t ) CRITICAL3("E080 - Bitwise OR unsupported for variable of given datatype", v, t );
#define CRITICAL_NOT_UNSUPPORTED( v, t ) CRITICAL3("E081 - Bitwise NOT unsupported for variable of given datatype", v, t );
#define CRITICAL_IMAGE_CONVERTER_INVALID_FRAME_WIDTH( w ) CRITICAL2i("E082 - invalid width for framed image, must be multiple of 8 pixels", w );
#define CRITICAL_IMAGE_CONVERTER_INVALID_FRAME_HEIGHT( h ) CRITICAL2i("E083 - invalid height for framed image, must be multiple of 8 pixels", h );
#define CRITICAL_IMAGE_CONVERTER_INVALID_OFFSET_X( x ) CRITICAL2i("E084 - invalid offset x for image, must be >= 0 and < width", x );
#define CRITICAL_IMAGE_CONVERTER_INVALID_OFFSET_Y( y ) CRITICAL2i("E085 - invalid offset y for image, must be >= 0 and < height", y );
#define CRITICAL_IMAGES_LOAD_INVALID_FRAME_WIDTH( w ) CRITICAL2i("E086 - invalid frame width, not multiple of width", w );
#define CRITICAL_IMAGES_LOAD_INVALID_FRAME_HEIGHT( h ) CRITICAL2i("E087 - invalid frame height, not multiple of height", h );
#define CRITICAL_PUT_IMAGE_UNSUPPORTED( v, t ) CRITICAL3("E088 - PUT IMAGE unsupported for given datatype", v, t );
#define CRITICAL_NOT_IMAGES( v ) CRITICAL2("E089 - variable is not an collection of images", v );
#define CRITICAL_NOT_ASSIGNED_IMAGES( v ) CRITICAL2("E090 - variable is not a loaded collection of images, please use assign operator", v );
#define CRITICAL_NOT_STRING_ARRAY( v ) CRITICAL2("E091 - accessing as a string array on a non string array", v );
#define CRITICAL_SIZE_UNSUPPORTED( v, t ) CRITICAL3("E092 - SIZE unsupported for variable of given datatype", v, t );
#define CRITICAL_UNSUPPORTED_OUTPUT_FILE_TYPE( t ) CRITICAL2("E093 - output file type format unsupported for this type of executable", t );
#define CRITICAL_BUFFER_SIZE_MISMATCH_ARRAY_SIZE( v, d1, d2 ) CRITICAL4si("E094 - size of buffer mismatch the size of array", v, d1, d2 );
#define CRITICAL_CANNOT_ASSIGN_TO_ARRAY( v, t ) CRITICAL3("E095 - cannot assign this type to array", v, t );
#define CRITICAL_NEW_IMAGE_UNSUPPORTED_MODE(f) CRITICAL2i("E096 - NEW IMAGE unsupported for the given screen mode", f );
#define CRITICAL_GET_IMAGE_UNSUPPORTED( v, t ) CRITICAL3("E097 - GET IMAGE unsupported for given datatype", v, t );
#define CRITICAL_INVALID_DIVISOR2( d ) CRITICAL2i("E098 - invalid divisor for DIVISION2, must be power of two", d );
#define CRITICAL_INVALID_MULTIPLICATOR2( d ) CRITICAL2i("E099 - invalid multiplicator for MULTIPLICATOR2, must be power of two", d );
#define CRITICAL_INVALID_TASK_COUNT( d ) CRITICAL2i("E100 - invalid number of tasks for multitasking", d);
#define CRITICAL_CANNOT_COMPARE_WITH_CASE( d ) CRITICAL2("E101 - cannot compare with case", d);
#define CRITICAL_ADD_INPLACE_UNSUPPORTED( v, t ) CRITICAL3("E102 - Add in place unsupported for variable of given datatype", v, t );
#define CRITICAL_SUB_INPLACE_UNSUPPORTED( v, t ) CRITICAL3("E103 - Sub in place unsupported for variable of given datatype", v, t );
#define CRITICAL_HEX_UNSUPPORTED( v, t ) CRITICAL3("E104 - HEX unsupported for variable of given datatype", v, t );
#define CRITICAL_PRINT_BUFFER_ON_A_NOT_BUFFER( v ) CRITICAL2("E105 - PRINT BUFFER not allowed for non buffer variables", v );
#define CRITICAL_10_LINE_RULES_ENFORCED( v ) CRITICAL2("E106 - this command is not allowed on sources for 10 liner contest", v );
#define CRITICAL_INVALID_INPUT_SEPARATOR( d ) CRITICAL2i("E107 - invalid character used for INPUT separator", d);
#define CRITICAL_INVALID_INPUT_SIZE( d ) CRITICAL2i("E108 - invalid size for INPUT temporary buffer", d);
#define CRITICAL_INVALID_INPUT_CURSOR( d ) CRITICAL2i("E109 - invalid cursor character for INPUT", d);
#define CRITICAL_CANNOT_CALCULATE_SPRITE_WIDTH( ) CRITICAL("E110 - cannot calculate SPRITE WIDTH statically" );
#define CRITICAL_CANNOT_CALCULATE_SPRITE_HEIGHT( ) CRITICAL("E111 - cannot calculate SPRITE HEIGHT statically" );
#define CRITICAL_CANNOT_ALLOCATE_MORE_TILE( ) CRITICAL("E112 - cannot allocate one more tile on tileset" );
#define CRITICAL_TILE_LOAD_MISSING_FILE(f) CRITICAL2("E113 - LOAD TILE missing file", f );
#define CRITICAL_TILE_LOAD_UNKNOWN_FORMAT(f) CRITICAL2("E114 - LOAD TILE file format unknown", f );
#define CRITICAL_TILE_INVALID_WIDTH( w ) CRITICAL2i("E115 - invalid width for tile, must be 8 pixels", w );
#define CRITICAL_TILE_INVALID_HEIGHT( h ) CRITICAL2i("E116 - invalid height for tile, must be 8 pixels", h );
#define CRITICAL_TILE_LOAD_ON_NON_TILESET( t ) CRITICAL2("E117 - loading tile(s) on non tileset", t );
#define CRITICAL_USE_TILESET_ON_NON_TILESET( t ) CRITICAL2("E118 - using a non tileset", t );
#define CRITICAL_CANNOT_MOVE_UNROLLED_TILE( t ) CRITICAL2("E119 - cannot move an unrolled tile", t );
#define CRITICAL_NOT_TILE( v ) CRITICAL2("E120 - variable is not a (set of) tile(s)", v );
#define CRITICAL_CANNOT_RESPAWN_NOT_THREADID( v ) CRITICAL2("E121 - cannot respawn something that is not a thread id", v );
#define CRITICAL_CANNOT_MMOVE_INVALID_SIZE( v ) CRITICAL2("E122 - invalid data type for SIZE on MMOVE", v );
#define CRITICAL_CANNOT_MMOVE_UNSUPPORTED( ) CRITICAL("E123 - MMOVE VIDEO to VIDEO unsupported" );
#define CRITICAL_EXPANSION_OUT_OF_MEMORY_LOADING( v ) CRITICAL2("E124 - out of memory when loading BANKED resource", v );
#define CRITICAL_NOT_ASSIGNED_SEQUENCE( v ) CRITICAL2("E125 - variable is not a set of loaded collection of images, please use assign operator", v );
#define CRITICAL_SEQUENCE_LOAD_INVALID_FRAME_WIDTH( w ) CRITICAL2i("E126 - invalid frame width, not multiple of width", w );
#define CRITICAL_SEQUENCE_LOAD_INVALID_FRAME_HEIGHT( h ) CRITICAL2i("E127 - invalid frame height, not multiple of height", h );
#define CRITICAL_CANNOT_MUSIC( v ) CRITICAL2("E128 - variable is not MUSIC, so cannot music it", v );
#define CRITICAL_FILENAME_INVALID_COLON( v ) CRITICAL2("E129 - invalid filename, colon character not allowed", v );
#define CRITICAL_FILENAME_INVALID_BACKSLASH( v ) CRITICAL2("E130 - invalid filename, backslash character not allowed", v );
#define CRITICAL_CANNOT_KILL_NOT_THREADID( v ) CRITICAL2("E131 - cannot KILL something that is not a thread id", v );
#define CRITICAL_STORAGE_NESTED_UNSUPPORTED( n ) CRITICAL2("E132 - cannot define a nested storage (a storage inside a storage)", n ); 
#define CRITICAL_STORAGE_NOT_OPENED() CRITICAL("E133 - ENDSTORAGE outside a storage definition" ); 
#define CRITICAL_DLOAD_MISSING_FILE(f) CRITICAL2("E134 - DLOAD missing file", f );
#define CRITICAL_INCLUDE_FILE_NOT_FOUND(f) CRITICAL2("E135 - INCLUDE missing file", f );
#define CRITICAL_XOR_UNSUPPORTED( v, t ) CRITICAL3("E136 - Bitwise XOR unsupported for variable of given datatype", v, t );
#define CRITICAL_SWAP_DIFFERENT_BITWIDTH( v )  CRITICAL2("E137 - Bitwise SWAP supported only for variable of same bitwidth", v );
#define CRITICAL_CANNOT_REMOVE_FILE(f,n) CRITICAL3("E138 - cannot remove file", f, n );
#define CRITICAL_CONSTANT_ALREADY_DEFINED_AS_VARIABLE(f) CRITICAL2("E139 - cannot define a variable with the same name of a constant", f );
#define CRITICAL_VARIABLE_ALREADY_DEFINED_AS_CONSTANT(f) CRITICAL2("E140 - cannot define a constant with the same name of a variable", f );
#define CRITICAL_END_GAMELOOP_WITHOUT_GAMELOOP() CRITICAL("E141 - END GAMELOOP without BEGIN GAMELOOP" );
#define CRITICAL_EVERY_OFF_WITHOUT_EVERY() CRITICAL("E142 - EVERY OFF without EVERY definition" );
#define CRITICAL_WEND_WITHOUT_WHILE() CRITICAL("E143 - WEND without WHILE" );
#define CRITICAL_EXIT_WITHOUT_LOOP() CRITICAL("E144 - EXIT without LOOP" );
#define CRITICAL_EXIT_WITHOUT_ENOUGH_LOOP() CRITICAL("E145 - EXIT without enough LOOPs" );
#define CRITICAL_ENDSELECT_WITHOUT_SELECT() CRITICAL("E146 - ENDSELECT without SELECT" );
#define CRITICAL_UNTIL_WITHOUT_REPEAT() CRITICAL("E147 - UNTIL without REPEAT" );
#define CRITICAL_LOOP_WITHOUT_DO() CRITICAL("E148 - LOOP without DO" );
#define CRITICAL_ENDIF_WITHOUT_IF() CRITICAL("E149 - ENDIF without IF" );
#define CRITICAL_NEXT_WITHOUT_FOR() CRITICAL("E150 - NEXT without FOR" );
#define CRITICAL_ELSE_WITHOUT_IF() CRITICAL("E151 - ELSE without IF" );
#define CRITICAL_CASE_WITHOUT_SELECT_CASE() CRITICAL("E152 - CASE without SELECT CASE" );
#define CRITICAL_CASE_ELSE_WITHOUT_SELECT_CASE() CRITICAL("E153 - CASE ELSE without SELECT CASE" );
#define CRITICAL_VARIABLE_REDEFINED_DIFFERENT_TYPE( f ) CRITICAL2("E154 - variable redefined with a different type", f );
#define CRITICAL_VARIABLE_IMPORTED_DIFFERENT_TYPE( f ) CRITICAL2("E155 - variable imported with a different type", f );
#define CRITICAL_CONSTANT_REDEFINED_DIFFERENT_TYPE( f ) CRITICAL2("E156 - constant redefined with a different type", f );
#define CRITICAL_CONSTANT_REDEFINED_DIFFERENT_VALUE( f ) CRITICAL2("E157 - constant redefined with a different value", f );
#define CRITICAL_VARIABLE_UNDEFINED(f) CRITICAL2("E158 - undefined variable (OPTION EXPLICIT ON)", f );
#define CRITICAL_VARIABLE_ALREADY_DEFINED(f) CRITICAL2("E159 - variable already defined", f );
#define CRITICAL_MACRO_TOO_MUCH_PARAMETERS(m, p) CRITICAL3("E160 - too much parameters in macro", m, p );
#define CRITICAL_MACRO_TOO_MUCH_LINES(m) CRITICAL2("E160 - too much lines in macro", m );
#define CRITICAL_MACRO_TOO_MUCH_VALUES(m,v) CRITICAL3("E161 - too much values in macro", m, v );
#define CRITICAL_MACRO_MISMATCH_PARAMETER_VALUES(m) CRITICAL2("E162 - mismatch number of values and parameters", m );
#define CRITICAL_MACRO_UNDEFINED(m) CRITICAL2("E163 - macro undefined", m );
#define CRITICAL_BLIT_IMAGE_UNSUPPORTED( v, t ) CRITICAL3("E164 - BLIT IMAGE unsupported for given datatype", v, t );
#define CRITICAL_BLIT_ALLOC_REGISTER_EXHAUSTED( ) CRITICAL("E165 - CPU registers exhausted in BLIT definition" );
#define CRITICAL_BLIT_ALLOC_MEMORY_EXHAUSTED( ) CRITICAL("E166 - CPU memory exhausted in BLIT definition" );
#define CRITICAL_BLIT_INVALID_FREE_REGISTER( s, r ) CRITICAL3i("E167 - invalid free CPU register free in BLIT definition", s, r );
#define CRITICAL_BLIT_TOO_MUCH_SOURCES( ) CRITICAL("E168 - too much sources on BLIT IMAGE for this target" );
#define CRITICAL_BLIT_CANNOT_MIX_IMAGE_TYPES( n ) CRITICAL2("E169 - cannot mix image types with BLIT IMAGE", n );
#define CRITICAL_BLIT_ALREADY_DEFINED( n ) CRITICAL2("E170 - BLIT with same name already defined", n );
#define CRITICAL_BLIT_CANNOT_BLIT( n ) CRITICAL2("E171 - BLIT IMAGE with something that is not a blit", n );
#define CRITICAL_ARRAY_DEFINITION_FILE_NOT_FOUND( n ) CRITICAL2("E172 - file not found for initialize array", n );
#define CRITICAL_ARRAY_MISSING_SIZE( n ) CRITICAL2("E173 - missing size on one or more dimensions", n );
#define CRITICAL_ARRAY_MULTIDIMENSIONAL( n ) CRITICAL2("E174 - simple UBOUND/LBOUND cannot be used on multidimensional array", n );
#define CRITICAL_ARRAY_DATATYPE_NOT_SUPPORTED( n ) CRITICAL2("E175 - datatype not supported for array loading from binary file", n );
#define CRITICAL_CANNOT_CAST_FLOAT_PRECISION( v1, v2 ) CRITICAL3("E0176 - Cannot cast types since float precision mismatch", v1, v2 );
#define CRITICAL_SWAP_UNSUPPORTED( v, t ) CRITICAL3("E177 - Swap unsupported for variable of given datatype", v, t );
#define CRITICAL_CANNOT_EMIT_FLOAT_CONST( v ) CRITICAL2("E178 - cannot emit floating point constants", v );
#define CRITICAL_UNKNOWN_CPU_REGISTER( ) CRITICAL("E179 - unknown register");
#define CRITICAL_UNSETTABLE_CPU_REGISTER( v ) CRITICAL2("E180 - CPU register cannot be used", v );
#define CRITICAL_UNKNOWN_CPU_STACK( v ) CRITICAL2("E181 - unknown stack size", v );
#define CRITICAL_DECLARE_PROC_NESTED_UNSUPPORTED( v ) CRITICAL2("E182 - cannot nest DECLARE PROC/FUNCTION inside a PROC", v );
#define CRITICAL_INVALID_INPUT_RATE( v ) CRITICAL2i("E183 - invalid value for INPUT/KEYBOARD RATE", v );
#define CRITICAL_INVALID_INPUT_DELAY( v ) CRITICAL2i("E184 - invalid value for INPUT/KEYBOARD DELAY", v );
#define CRITICAL_ARRAY_ASSIGN_DATATYPE_NOT_SUPPORTED( v ) CRITICAL2("E185 - cannot instantiate an array of this kind with direct assignment", v );
#define CRITICAL_TILESET_LOAD_UNKNOWN_FORMAT( v ) CRITICAL2("E186 - unknown tileset format", v );
#define CRITICAL_TILESET_LOAD_MISSING_IMAGE( v ) CRITICAL2("E187 - missing image from tileset", v );
#define CRITICAL_RESOURCE_LOAD_MISSING_FILE(f) CRITICAL2("E188 - missing file in loading resource", f );
#define CRITICAL_PUT_IMAGE_NAMED_TILE_MISSING_TILESET( v ) CRITICAL2("E189 - missing tileset from images", v );
#define CRITICAL_PUT_IMAGE_NAMED_TILE_MISSING_TILES_FROM_TILESET( v ) CRITICAL2("E190 - missing tiles' definition on tileset", v );
#define CRITICAL_PUT_IMAGE_NAMED_TILE_NOT_FOUND( v ) CRITICAL2("E191 - tile not found in tileset", v );
#define CRITICAL_PUT_IMAGE_NAMED_TILE_INVALID_PROBABILITY( v ) CRITICAL2("E192 - invalid probability for tile selection", v );
#define CRITICAL_TILEMAP_LOAD_UNKNOWN_FORMAT( v ) CRITICAL2("E193 - unknown tilemap format", v );
#define CRITICAL_TILEMAP_LOAD_MISSING_LAYER( v ) CRITICAL2("E194 - missing layer from tilemap", v );
#define CRITICAL_TILEMAP_LOAD_MISSING_TILESET( v ) CRITICAL2("E195 - missing tileset from tilemap", v );
#define CRITICAL_TILEMAP_LOAD_ONLY_ONE_TILESET( v ) CRITICAL2("E196 - only one tileset is supported for each tilemap", v );
#define CRITICAL_CANNOT_PUT_TILEMAP_FOR_NON_TILEMAP( v ) CRITICAL2("E197 - cannot PUT TILEMAP without a tile map", v );
#define CRITICAL_CANNOT_CAST_TILEMAP_SIZE( v ) CRITICAL2("E198 - cannot cast TILEMAP since sizes are different", v );
#define CRITICAL_TILEMAP_LOAD_ONLY_ONE_LAYER( v ) CRITICAL2("E199 - only one layer is supported for each tilemap", v );
#define CRITICAL_TILE_CLASS_NO_TILESET( v ) CRITICAL2("E200 - cannot call TILE CLASS on something that is not a TILESET", v );
#define CRITICAL_TILE_CLASS_INVALID_ID( v ) CRITICAL2i("E201 - invalid tile id on TILE CLASS", v );
#define CRITICAL_TILE_WIDTH_NO_TILESET( v ) CRITICAL2("E202 - cannot call TILE WIDTH on something that is not a TILESET", v );
#define CRITICAL_TILE_HEIGHT_NO_TILESET( v ) CRITICAL2("E203 - cannot call TILE HEIGHT on something that is not a TILESET", v );
#define CRITICAL_TILE_PROBABILITY_NO_TILESET( v ) CRITICAL2("E204 - cannot call TILE PROBABILITY on something that is not a TILESET", v );
#define CRITICAL_TILE_PROBABILITY_INVALID_ID( v ) CRITICAL2i("E205 - invalid tile id on TILE PROBABILITY", v );
#define CRITICAL_TILEMAP_WIDTH_NO_TILEMAP( v ) CRITICAL2("E206 - cannot call TILEMAP WIDTH on something that is not a TILEMAP", v );
#define CRITICAL_TILEMAP_HEIGHT_NO_TILEMAP( v ) CRITICAL2("E207 - cannot call TILEMAP HEIGHT on something that is not a TILEMAP", v );
#define CRITICAL_TILEMAP_LOAD_ONLY_SAME_SIZE_LAYER( v ) CRITICAL2("E208 - cannot use tile maps with layers of different size", v );
#define CRITICAL_TILESET_OF_INVALID_TILEMAP( v ) CRITICAL2("E209 - cannot use TILESET OF on something that is not a TILEMAP", v );
#define CRITICAL_TILEMAP_INDEX_INVALID_TILEMAP( v ) CRITICAL2("E210 - cannot use TILEMAP INDEX on something that is not a TILEMAP", v );
#define CRITICAL_SLICE_IMAGE_UNSUPPORTED( v, t ) CRITICAL3("E211 - SLICE IMAGE unsupported for given datatype", v, t );
#define CRITICAL_SLICE_IMAGE_UNSUPPORTED_COMBINATION( ) CRITICAL("E212 - SLICE IMAGE cannot optimize the call in this combination" );
#define CRITICAL_IMAGE_EXTRACT_ON_NOT_IMAGES( v ) CRITICAL2("E213 - calling IMAGE on something that is not IMAGES / SEQUENCE", v );
#define CRITICAL_TILE_ID_ON_NOT_TILESET( v ) CRITICAL2("E214 - using TILE ID on something that is not a TILESET", v ); 
#define CRITICAL_TILE_ID_MISSING_ORIGINAL_TILESET( v ) CRITICAL2("E215 - missing Tiled informations", v ); 
#define CRITICAL_IMAGES_LOAD_INVALID_AUTO_WITHOUT_GIF( v ) CRITICAL2("E216 - cannot use implicit FRAME SIZE without an animated GIF", v );
#define CRITICAL_IMAGES_LOAD_IMAGE_TOO_BIG( v ) CRITICAL2("E217 - IMAGES cannot be loaded since is too big", v );
#define CRITICAL_SEQUENCE_LOAD_IMAGE_TOO_BIG( v ) CRITICAL2("E218 - SEQUENCE cannot be loaded since is too big", v );
#define CRITICAL_TILES_LOAD_IMAGE_TOO_BIG( v ) CRITICAL2("E218 - TILES cannot be loaded since is too big", v );
#define CRITICAL_TILESET_LOAD_IMAGE_TOO_BIG( v ) CRITICAL2("E219 - TILESET cannot be loaded since is too big", v );
#define CRITICAL_IF_WITHOUT_ENDIF( ) CRITICAL("E220 - IF without ENDIF" );
#define CRITICAL_SELECT_CASE_WITHOUT_ENDSELECT( ) CRITICAL("E221 - SELECT CASE without ENDSELECT" );
#define CRITICAL_DO_WITHOUT_LOOP( ) CRITICAL("E222 - DO without LOOP" );
#define CRITICAL_WHILE_WITHOUT_WEND( ) CRITICAL("E223 - WHILE without WEND" );
#define CRITICAL_REPEAT_WITHOUT_UNTIL( ) CRITICAL("E224 - REPEAT without UNTIL" );
#define CRITICAL_FOR_WITHOUT_NEXT( ) CRITICAL("E225 - FOR without NEXT" );
#define CRITICAL_BEGIN_GAMELOOP_WITHOUT_END_GAMELOOP( ) CRITICAL("E226 - BEGIN GAMELOOP without END GAMELOOP" );
#define CRITICAL_CANNOT_GENERATE_RANDOM( ) CRITICAL("E227 - cannot generate random number with this parameter" );
#define CRITICAL_LINE_NUMBER_ALREADY_DEFINED( n ) CRITICAL2i("E228 - line number already defined", n );
#define CRITICAL_LABEL_ALREADY_DEFINED( n ) CRITICAL2("E229 - label already defined", n );
#define CRITICAL_TILE_ID_NO_TILESET( v ) CRITICAL2("E230 - cannot call TILE ID on something that is not a TILESET", v );
#define CRITICAL_READ_WITHOUT_DATA( v ) CRITICAL("E231 - READ without DATA" );
#define CRITICAL_READ_DATA_TYPE_NOT_SUPPORTED( v, t ) CRITICAL3("E232 - READ not supported for this kind of variable", v, t );
#define CRITICAL_RESTORE_WITHOUT_DATA( v ) CRITICAL2("E233 - RESTORE without DATA", v );
#define CRITICAL_READ_END_WITHOUT_DATA( ) CRITICAL("E234 - READ END without DATA" );
#define CRITICAL_DATA_LOAD_TEXT_NOT_FOUND( v ) CRITICAL2("E235 - cannot find file to load DATA in", v );
#define CRITICAL_CANNOT_COMPARE_CONST( t ) CRITICAL2("E236 - Cannot compare type with a constant", t );
#define CRITICAL_CANNOT_USE_DRAW_WITHOUT_STRING( t ) CRITICAL2("E237 - DRAW need a string with drawing commands", t );
#define CRITICAL_PUT_NOT_NOT_SUPPORTED( t ) CRITICAL2("E238 - PUT with NOT operator is not supported", t );
#define CRITICAL_CANNOT_USE_PLAY_WITHOUT_STRING( t ) CRITICAL2("E239 - PLAY need a string with playing commands", t );
#define CRITICAL_RESTORE_WITH_UNSUPPORTED_DATA_TYPE( t ) CRITICAL2("E240 - RESTORE with unsupported data type", t ); 
#define CRITICAL_SANDBOX_ENFORCED( v ) CRITICAL2("E241 - this command is not allowed on sources for sandbox execution", v );
#define CRITICAL_NEW_IMAGES_UNSUPPORTED_MODE(f) CRITICAL2i("E242 - NEW IMAGES unsupported for the given screen mode", f );
#define CRITICAL_MIDI_OUT_OF_MEMORY(f) CRITICAL2("E243 - out of memory on MIDI conversion using LOAD MUSIC", f );
#define CRITICAL_STRPTR_NOT_STRING(v, t) CRITICAL3("E244 - cannot use STRPTR on something that is not a string", v, t );
#define CRITICAL_DLOAD_MISSING_ADDRESS(v) CRITICAL2("E245 - destination address for DLOAD is missing", v );
#define CRITICAL_DLOAD_MISSING_SIZE(v) CRITICAL2("E246 - size for DLOAD is missing", v );
#define CRITICAL_STORAGE_NOT_AVAILABLE() CRITICAL("E247 - the BEGIN STORAGE keyword is not available with this z88dk release" );
#define CRITICAL_CANNOT_STORE_FILE_ON_VARIABLE_OF_DIFFERENT_TYPE(v) CRITICAL("E248 - cannot store file on different type variable" );
#define CRITICAL_MISSING_FILE_STORAGE(v) CRITICAL("E249 - missing file storage" );
#define CRITICAL_DSAVE_MISSING_ADDRESS(v) CRITICAL2("E250 - source address for DSAVE is missing", v );
#define CRITICAL_DSAVE_MISSING_SIZE(v) CRITICAL2("E251 - size for DSAVE is missing", v );
#define CRITICAL_VARIABLE_CANNOT_DIRECT_ASSIGN_DIFFERENT_TYPE( t1, t2 ) CRITICAL3("E252 - cannot direct assign between different types", t1, t2 );
#define CRITICAL_WRONG_NEXT_INDEX(v) CRITICAL2("E253 - NEXT with a wrong FOR index", v );
#define CRITICAL_PUT_IMAGE_UNINITIALIZED(v) CRITICAL2("E254 - PUT IMAGE with uninitialized image variable", v );
#define CRITICAL_MULTITASKING_ALREADY_FORBIDDEN() CRITICAL("E255 - multitasking is already forbidden");
#define CRITICAL_MULTITASKING_NOT_FORBIDDEN() CRITICAL("E256 - multitasking is already allowed");
#define CRITICAL_MULTITASKING_FORBIDDEN() CRITICAL("E257 - multitasking is actually forbidden");
#define CRITICAL_INVALID_PAINT_BUFFER(v) CRITICAL2i("E258 - invalid PAINT BUFFER size", v );
#define CRITICAL_TILEMAP_SOURCE_MISSING(v) CRITICAL2("E259 - invalid tilemap, missing source", v );
#define CRITICAL_IMAGES_LOAD_IMAGE_BUFFER_TOO_BIG() CRITICAL("E260 - image too big from buffer" );
#define CRITICAL_PROCEDURE_DUPLICATE_PARAMETER(p,v) CRITICAL3("E261 - duplicate parameter on procedure", p, v );
#define CRITICAL_CANNOT_KILL_NOT_ARRAY_THREADS(v) CRITICAL2("E262 - cannot KILL elements of something that is not an array of threads", p, v );

#define WARNING( s ) if ( ((struct _Environment *)_environment)->warningsEnabled) { fprintf(stderr, "WARNING during compilation of %s:\n\t%s at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, ((struct _Environment *)_environment)->yylineno ); }
#define WARNING2( s, v ) if ( ((struct _Environment *)_environment)->warningsEnabled) { fprintf(stderr, "WARNING during compilation of %s:\n\t%s (%s) at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, v, _environment->yylineno ); }
#define WARNING2i( s, v ) if ( ((struct _Environment *)_environment)->warningsEnabled) { fprintf(stderr, "WARNING during compilation of %s:\n\t%s (%i) at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, v, _environment->yylineno ); }
#define WARNING3( s, v1, v2 ) if ( ((struct _Environment *)_environment)->warningsEnabled) { fprintf(stderr, "WARNING during compilation of %s:\n\t%s (%s, %s) at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, v1, v2, _environment->yylineno ); }
#define WARNING_BITWIDTH( v1, v2 ) WARNING3("W001 - Multiplication could loose precision", v1, v2 );
#define WARNING_DOWNCAST( v1, v2 ) WARNING3("W002 - Implicit downcasting to less bitwidth (precision loss)", v1, v2 );
#define WARNING_SCREEN_MODE( v1 ) WARNING2i("W003 - Screen mode unsupported", v1 );
#define WARNING_USE_OF_UNDEFINED_ARRAY( v1 ) WARNING2("W004 - use of undefined array", v1 );
#define WARNING_IMAGE_CONVERTER_UNSUPPORTED_MODE(f) WARNING2i("W005 - IMAGE converter unsupported for the given screen mode", f );
#define WARNING_IMAGE_LOAD_EXACT_IGNORED( ) WARNING("W006 - Loading of the image will ignore EXACT flag" );
#define WARNING_DLOAD_IGNORED_SIZE( f ) WARNING2("W007 - size for DLOAD is ignored", f );
#define WARNING_DLOAD_IGNORED_OFFSET( f ) WARNING2("W008 - offset for DLOAD is ignored", f );

int assemblyLineIsAComment( char * _buffer );

int buffered_fputs(const char * _string, FILE * _stream);
void buffered_fprintf(FILE * _stream, const char * _format, ...);
size_t buffered_fwrite( void * _data, size_t _size, size_t _count, FILE * _stream);
void buffered_push_output( );
void buffered_output( FILE * _stream );
void buffered_prepend_output( );
void buffered_pop_output( );

#define outline0n(n,s,r)     \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            buffered_fputs("\t", ((Environment *)_environment)->asmFile); \
        if ( ((Environment *)_environment)->emptyProcedure ) { \
            buffered_fputs("\t; (excluded by ON target) : ", ((Environment *)_environment)->asmFile); \
        } \
        buffered_fputs(s,((Environment *)_environment)->asmFile); \
        if ( r ) { \
            buffered_fputs("\n", ((Environment *)_environment)->asmFile); \
            if ( ! ((Environment *)_environment)->emptyProcedure ) { \
                ((Environment *)_environment)->producedAssemblyLines += assemblyLineIsAComment( s ) ? 0 : 1; \
            } \
        } \
    }

#define outline1n(n,s,a,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            buffered_fputs("\t", ((Environment *)_environment)->asmFile); \
        if ( ((Environment *)_environment)->emptyProcedure ) { \
            buffered_fputs("\t; (excluded by ON target) : ", ((Environment *)_environment)->asmFile); \
        } \
        buffered_fprintf(((Environment *)_environment)->asmFile, s, a); \
        if ( r ) { \
            buffered_fputs("\n", ((Environment *)_environment)->asmFile); \
            if ( ! ((Environment *)_environment)->emptyProcedure ) { \
                ((Environment *)_environment)->producedAssemblyLines += assemblyLineIsAComment( s ) ? 0 : 1; \
            } \
        } \
    }

#define outline2n(n,s,a,b,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            buffered_fputs("\t", ((Environment *)_environment)->asmFile); \
        if ( ((Environment *)_environment)->emptyProcedure ) { \
            buffered_fputs("\t; (excluded by ON target) : ", ((Environment *)_environment)->asmFile); \
        } \
        buffered_fprintf(((Environment *)_environment)->asmFile, s, a, b); \
        if ( r ) { \
            buffered_fputs("\n", ((Environment *)_environment)->asmFile); \
            if ( ! ((Environment *)_environment)->emptyProcedure ) { \
                ((Environment *)_environment)->producedAssemblyLines += assemblyLineIsAComment( s ) ? 0 : 1; \
            } \
        } \
    }

#define outline3n(n,s,a,b,c,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            buffered_fputs("\t", ((Environment *)_environment)->asmFile); \
        if ( ((Environment *)_environment)->emptyProcedure ) { \
            buffered_fputs("\t; (excluded by ON target) : ", ((Environment *)_environment)->asmFile); \
        } \
        buffered_fprintf(((Environment *)_environment)->asmFile, s, a, b, c); \
        if ( r ) { \
            buffered_fputs("\n", ((Environment *)_environment)->asmFile); \
            if ( ! ((Environment *)_environment)->emptyProcedure ) { \
                ((Environment *)_environment)->producedAssemblyLines += assemblyLineIsAComment( s ) ? 0 : 1; \
            } \
        } \
    }

#define outline4n(n,s,a,b,c,d,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            buffered_fputs("\t", ((Environment *)_environment)->asmFile); \
        if ( ((Environment *)_environment)->emptyProcedure ) { \
            buffered_fputs("\t; (excluded by ON target) : ", ((Environment *)_environment)->asmFile); \
        } \
        buffered_fprintf(((Environment *)_environment)->asmFile, s, a, b, c, d); \
        if ( r ) { \
            buffered_fputs("\n", ((Environment *)_environment)->asmFile); \
            if ( ! ((Environment *)_environment)->emptyProcedure ) { \
                ((Environment *)_environment)->producedAssemblyLines += assemblyLineIsAComment( s ) ? 0 : 1; \
            } \
        } \
    }

#define outline5n(n,s,a,b,c,d,e,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            buffered_fputs("\t", ((Environment *)_environment)->asmFile); \
        if ( ((Environment *)_environment)->emptyProcedure ) { \
            buffered_fputs("\t; (excluded by ON target) : ", ((Environment *)_environment)->asmFile); \
        } \
        buffered_fprintf(((Environment *)_environment)->asmFile, s, a, b, c, d, e); \
        if ( r ) { \
            buffered_fputs("\n", ((Environment *)_environment)->asmFile); \
            if ( ! ((Environment *)_environment)->emptyProcedure ) { \
                ((Environment *)_environment)->producedAssemblyLines += assemblyLineIsAComment( s ) ? 0 : 1; \
            } \
        } \
    }

#define cfgline0n(n,s,r)     \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->configurationFile); \
        fputs(s,((Environment *)_environment)->configurationFile); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->configurationFile); \
    }

#define cfgline1n(n,s,a,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->configurationFile); \
        fprintf(((Environment *)_environment)->configurationFile, s, a); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->configurationFile); \
    }

#define cfgline2n(n,s,a,b,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->configurationFile); \
        fprintf(((Environment *)_environment)->configurationFile, s, a, b); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->configurationFile); \
    }

#define cfgline3n(n,s,a,b,c,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->configurationFile); \
        fprintf(((Environment *)_environment)->configurationFile, s, a, b, c); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->configurationFile); \
    }

#define cfgline4n(n,s,a,b,c,d,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->configurationFile); \
        fprintf(((Environment *)_environment)->configurationFile, s, a, b, c, d); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->configurationFile); \
    }

#define cfgline5n(n,s,a,b,c,d,e,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->configurationFile); \
        fprintf(((Environment *)_environment)->configurationFile, s, a, b, c, d, e); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->configurationFile); \
    }

#define outfile0(f)     \
    { \
        FILE * fh = fopen( f, "rt" ); \
        if ( fh ) { \
            char line[MAX_TEMPORARY_STORAGE]; \
            while( ! feof( fh ) ) { \
                if ( fgets( line, MAX_TEMPORARY_STORAGE, fh ) ) { \
                    buffered_fputs( line, ((Environment *)_environment)->asmFile); \
                    ((Environment *)_environment)->producedAssemblyLines += assemblyLineIsAComment( line ) ? 0 : 1; \
                } \
            } \
            fclose( fh ); \
            buffered_fputs("\n", ((Environment *)_environment)->asmFile); \
        } else { \
            CRITICAL2("Unable to include ugbasic system file", f ); \
        } \
    } 

int embedparse (void *);
int embed_scan_string (const char *);

#define outembedded0(e)     \
     { \
        char * parsed = malloc( (8*e##_len) + 1 ); \
        memset( parsed, 0, (8*e##_len) + 1 ); \
        char * tmp = malloc( e##_len + 1 ); \
        memset( tmp, 0, e##_len + 1 ); \
        memcpy( tmp, e, e##_len ); \
        char * line = strtok( tmp, "\x0a" ); \
        while( line ) { \
            _environment->embedResult.line = line; \
            _environment->embedResult.conditional = 0; \
            _environment->embedResult.lineCount = 0; \
            embed_scan_string( line ); \
            embedparse(_environment); \
            if ( ! _environment->embedResult.conditional ) { \
                int i; \
                for( i=0; i<_environment->embedResult.current; ++i ) { \
                    if ( _environment->embedResult.excluded[i] ) \
                        break; \
                } \
                if ( i>= _environment->embedResult.current ) { \
                    if ( _environment->embedResult.lineCount ) { \
                        int j=0; \
                        for( j=0; j<_environment->embedResult.lineCount; ++j ) { \
                            strcat( parsed, _environment->embedResult.lines[j] ); \
                            strcat( parsed, "\x0a" ); \
                            ((Environment *)_environment)->producedAssemblyLines += assemblyLineIsAComment( _environment->embedResult.lines[j] ) ? 0 : 1; \
                        } \
                    } else { \
                        strcat( parsed, line ); \
                        strcat( parsed, "\x0a" ); \
                        ((Environment *)_environment)->producedAssemblyLines += assemblyLineIsAComment( line ) ? 0 : 1; \
                    } \
                } \
            } \
            line = strtok( NULL, "\x0a" ); \
        } \
        free( tmp ); \
        buffered_fwrite( parsed, strlen( parsed )-1, 1, ((Environment *)_environment)->asmFile ); \
        free( parsed ); \
        buffered_fputs( "\n", ((Environment *)_environment)->asmFile ); \
    }

#define outembeddeddef0(e) \
    { \
        int deferredIndex = 0; \
        \
        for( deferredIndex = 0; deferredIndex < MAX_TEMPORARY_STORAGE; ++deferredIndex ) { \
            if ( !_environment->deferredEmbedded[deferredIndex] ) { \
                break; \
            } \
        } \
        \
        char * tmp = malloc( e##_len + 1 ); \
        memset( tmp, 0, e##_len + 1 ); \
        memcpy( tmp, e, e##_len ); \
        \
        _environment->deferredEmbedded[deferredIndex] = tmp; \
        _environment->deferredEmbeddedSize[deferredIndex] = e##_len; \
        \
    }


#define adiline0(s) \
    if ( ((Environment *)_environment)->additionalInfoFile ) { \
            fprintf( ((Environment *)_environment)->additionalInfoFile, s ); \
            fprintf( ((Environment *)_environment)->additionalInfoFile, "\n" ); \
    }
#define adiline1(s,a) \
    if ( ((Environment *)_environment)->additionalInfoFile ) { \
            fprintf( ((Environment *)_environment)->additionalInfoFile, s, a ); \
            fprintf( ((Environment *)_environment)->additionalInfoFile, "\n" ); \
    }
#define adiline2(s,a,b) \
    if ( ((Environment *)_environment)->additionalInfoFile ) { \
            fprintf( ((Environment *)_environment)->additionalInfoFile, s, a, b ); \
            fprintf( ((Environment *)_environment)->additionalInfoFile, "\n" ); \
    }
#define adiline3(s,a,b,c) \
    if ( ((Environment *)_environment)->additionalInfoFile ) { \
            fprintf( ((Environment *)_environment)->additionalInfoFile, s, a, b, c ); \
            fprintf( ((Environment *)_environment)->additionalInfoFile, "\n" ); \
    }
#define adiline4(s,a,b,c,d) \
    if ( ((Environment *)_environment)->additionalInfoFile ) { \
            fprintf( ((Environment *)_environment)->additionalInfoFile, s, a, b, c, d ); \
            fprintf( ((Environment *)_environment)->additionalInfoFile, "\n" ); \
    }
#define adiline5(s,a,b,c,d,e) \
    if ( ((Environment *)_environment)->additionalInfoFile ) { \
            fprintf( ((Environment *)_environment)->additionalInfoFile, s, a, b, c, d, e ); \
            fprintf( ((Environment *)_environment)->additionalInfoFile, "\n" ); \
    }

#define adiline6(s,a,b,c,d,e,f) \
    if ( ((Environment *)_environment)->additionalInfoFile ) { \
            fprintf( ((Environment *)_environment)->additionalInfoFile, s, a, b, c, d, e, f ); \
            fprintf( ((Environment *)_environment)->additionalInfoFile, "\n" ); \
    }

#define adilinepalette(s,c,p) \
    if ( ((Environment *)_environment)->additionalInfoFile ) { \
            fprintf( ((Environment *)_environment)->additionalInfoFile, s, c ); \
            fprintf( ((Environment *)_environment)->additionalInfoFile, ":" ); \
            int i = 0; \
            for( i=0; i<c; ++i ) { \
                fprintf( ((Environment *)_environment)->additionalInfoFile, "%2.2x%2.2x%2.2x%2.2x:%2.2x:%s :%2.2x:%2.2x:%2.2x:", \
                    p[i].alpha, p[i].red, p[i].green, p[i].blue, p[i].index, (p[i].description)?p[i].description:"", p[i].hardwareIndex, p[i].used, p[i].count ); \
            } \
            fprintf( ((Environment *)_environment)->additionalInfoFile, "\n" ); \
    }

#define adilinebeginbitmap(s) \
    if ( ((Environment *)_environment)->additionalInfoFile ) { \
            fprintf( ((Environment *)_environment)->additionalInfoFile, "%s:", s ); \
    }

#define adilinepixel(p) \
    if ( ((Environment *)_environment)->additionalInfoFile ) { \
            fprintf( ((Environment *)_environment)->additionalInfoFile, "%2.2x:", p ); \
    }

#define adilineendbitmap() \
    if ( ((Environment *)_environment)->additionalInfoFile ) { \
            fprintf( ((Environment *)_environment)->additionalInfoFile, "\n" ); \
    }

#define outhead0(s)             outline0n(0, s, 1)
#define outhead1(s,a)           outline1n(0, s, a, 1)
#define outhead2(s,a,b)         outline2n(0, s, a, b, 1)
#define outhead3(s,a,b,c)       outline3n(0, s, a, b, c, 1)
#define outhead4(s,a,b,c,d)     outline4n(0, s, a, b, c, d, 1)
#define outhead5(s,a,b,c,d,e)   outline5n(0, s, a, b, c, d, e, 1)
#define outline0(s)             outline0n(1, s, 1)
#define outline1(s,a)           outline1n(1, s, a, 1)
#define outline2(s,a,b)         outline2n(1, s, a, b, 1)
#define outline3(s,a,b,c)       outline3n(1, s, a, b, c, 1)
#define outline4(s,a,b,c,d)     outline4n(1, s, a, b, c, d, 1)
#define outline5(s,a,b,c,d,e)   outline5n(1, s, a, b, c, d, e, 1)
#define out0(s)                 outline0n(0, s, 0)
#define out1(s,a)               outline1n(0, s, a, 0)
#define out2(s,a,b)             outline2n(0, s, a, b, 0)
#define out3(s,a,b,c)           outline3n(0, s, a, b, c, 0)
#define out4(s,a,b,c,d)         outline4n(0, s, a, b, c, d, 0)
#define out5(s,a,b,c,d,e)       outline5n(0, s, a, b, c, d, e, 0)

#define cfghead0(s)             cfgline0n(0, s, 1)
#define cfghead1(s,a)           cfgline1n(0, s, a, 1)
#define cfghead2(s,a,b)         cfgline2n(0, s, a, b, 1)
#define cfghead3(s,a,b,c)       cfgline3n(0, s, a, b, c, 1)
#define cfghead4(s,a,b,c,d)     cfgline4n(0, s, a, b, c, d, 1)
#define cfghead5(s,a,b,c,d,e)   cfgline5n(0, s, a, b, c, d, e, 1)
#define cfgline0(s)             cfgline0n(1, s, 1)
#define cfgline1(s,a)           cfgline1n(1, s, a, 1)
#define cfgline2(s,a,b)         cfgline2n(1, s, a, b, 1)
#define cfgline3(s,a,b,c)       cfgline3n(1, s, a, b, c, 1)
#define cfgline4(s,a,b,c,d)     cfgline4n(1, s, a, b, c, d, 1)
#define cfgline5(s,a,b,c,d,e)   cfgline5n(1, s, a, b, c, d, e 1)
#define cfg0(s)                 cfgline0n(0, s, 0)
#define cfg1(s,a)               cfgline1n(0, s, a, 0)
#define cfg2(s,a,b)             cfgline2n(0, s, a, b, 0)
#define cfg3(s,a,b,c)           cfgline3n(0, s, a, b, c, 0)
#define cfg4(s,a,b,c,d)         cfgline4n(0, s, a, b, c, d, 0)
#define cfg5(s,a,b,c,d,e)       cfgline5n(0, s, a, b, c, d, e, 0)

#define deploy(s,e)  \
        if ( ! _environment->deployed.s ) { \
            int ignoreEmptyProcedure = _environment->emptyProcedure; \
            _environment->emptyProcedure = 0; \
            cpu_jump( _environment, #s "_after" ); \
            outembedded0(e); \
            cpu_label( _environment, #s "_after" ); \
            _environment->emptyProcedure = ignoreEmptyProcedure; \
            _environment->deployed.s = 1; \
        }

#define deploy_preferred(s,e)  \
        _environment->deployed.s = 1; \

#define deploy_deferred(s,e)  \
        if ( ! _environment->deployed.s ) { \
            outembeddeddef0(e); \
            _environment->deployed.s = 1; \
        }

#define deploy_inplace(s,e)  \
        if ( ! _environment->deployed.s ) { \
            outembedded0(e); \
        }

#define deploy_inplace_preferred(s,e)  \
        if ( _environment->deployed.s ) { \
            _environment->deployed.s = 0; \
            deploy_inplace(s,e); \
            _environment->deployed.s = 1; \
        }

#define deploy_with_vars(s,e,v)  \
        if ( ! _environment->deployed.s ) { \
            int ignoreEmptyProcedure = _environment->emptyProcedure; \
            _environment->emptyProcedure = 0; \
            cpu_jump( _environment, #s "_after" ); \
            outembedded0(e); \
            v(_environment);\
            cpu_label( _environment, #s "_after" ); \
            _environment->emptyProcedure = ignoreEmptyProcedure; \
            _environment->deployed.s = 1; \
        }

#define deploy_deferred_with_vars(s,e,v)  \
        if ( ! _environment->deployed.s ) { \
            outembeddeddef0(e); \
            v(_environment);\
            _environment->deployed.s = 1; \
        }

#define deploy_embedded(s,e)  \
        if ( ! _environment->deployed.embedded.s ) { \
            int ignoreEmptyProcedure = _environment->emptyProcedure; \
            _environment->emptyProcedure = 0; \
            cpu_jump( _environment, #s "_after" ); \
            outembedded0(e); \
            cpu_label( _environment, #s "_after" ); \
            _environment->emptyProcedure = ignoreEmptyProcedure; \
            _environment->deployed.embedded.s = 1; \
        }

#define deploy_deferred_embedded(s,e)  \
        if ( ! _environment->deployed.embedded.s ) { \
            outembeddeddef0(e); \
            _environment->deployed.embedded.s = 1; \
        }

#define deploy_begin(s)  \
        if ( ! _environment->deployed.s ) { \
            int ignoreProtothread = _environment->protothread; \
            int ignoreEmptyProcedure = _environment->emptyProcedure; \
            _environment->protothread = 0; \
            _environment->emptyProcedure = 0; \
            cpu_jump( _environment, #s "_after" ); \
            cpu_label( _environment, "lib_" #s ); \

#define deploy_end(s)  \
            cpu_label( _environment, #s "_after" ); \
            _environment->protothread = ignoreProtothread; \
            _environment->emptyProcedure = ignoreEmptyProcedure; \
            _environment->deployed.s = 1; \
        }

#define inline(s) \
        _environment->embeddedStats.s++; \
        if ( !_environment->embedded.s ) {

#define no_inline(s) \
        if ( !_environment->embedded.s ) { \
            CRITICAL_UNABLE_TO_INLINE(#s); \

#define no_embedded(s) \
        } else { \
            CRITICAL_UNABLE_TO_EMBED(#s); \
        }

#define embedded(s,e) \
        } else { \
            deploy_embedded(s,e) \

#define done() \
        }

#define parse_embedded(p, s) \
    if ( strcmp( p, #s ) == 0 ) { \
        _environment->embedded.s = 1; \
    }

#define stats_embedded(s) \
    printf("%s:\t%d\t%s\t\n", #s, _environment->embeddedStats.s, _environment->embedded.s ? "embedded" : "inline" );

#define WW_PEN              1
#define WW_PAPER            2

#define SHIFT_LEFT          1
#define SHIFT_RIGHT         2
#define SHIFT_CAPSLOCK      4
#define SHIFT_CONTROL       8
#define SHIFT_LEFT_ALT      16
#define SHIFT_RIGHT_ALT     32

#define TILEMAP_NATIVE      0
#define BITMAP_NATIVE       1

#define PROTOTHREAD_STATUS_WAITING		0
#define PROTOTHREAD_STATUS_RUNNING		1
#define PROTOTHREAD_STATUS_YIELDED		2
#define PROTOTHREAD_STATUS_EXITED		3
#define PROTOTHREAD_STATUS_ENDED		4

#define FLAG_FLIP_X         1
#define FLAG_FLIP_Y         2
#define FLAG_ROLL_X         4
#define FLAG_OVERLAYED      8
#define FLAG_ROLL_Y         16

#define FLAG_TRANSPARENCY   32
#define FLAG_DOUBLE_Y       64
#define FLAG_EXACT          128
#define FLAG_COMPRESSED     256
#define FLAG_WITH_PALETTE   512

#define IMF_INSTRUMENT_EXPLOSION        			0x00
#define IMF_INSTRUMENT_ACOUSTIC_GRAND_PIANO			0x01
#define IMF_INSTRUMENT_BRIGHT_ACOUSTIC_PIANO		0x02
#define IMF_INSTRUMENT_ELECTRIC_GRAND_PIANO			0x03
#define IMF_INSTRUMENT_HONKY_TONK_PIANO				0x04
#define IMF_INSTRUMENT_ELECTRIC_PIANO1				0x05
#define IMF_INSTRUMENT_ELECTRIC_PIANO2				0x06
#define IMF_INSTRUMENT_HARPSICHORD					0x07
#define IMF_INSTRUMENT_CLAVI						0x08
#define IMF_INSTRUMENT_CELESTA						0x09
#define IMF_INSTRUMENT_GLOCKENSPIEL					0x0A
#define IMF_INSTRUMENT_MUSIC_BOX					0x0B
#define IMF_INSTRUMENT_VIBRAPHONE					0x0C
#define IMF_INSTRUMENT_MARIMBA						0x0D
#define IMF_INSTRUMENT_XYLOPHONE					0x0E
#define IMF_INSTRUMENT_TUBULAR_BELLS				0x0F
#define IMF_INSTRUMENT_DULCIMER						0x10
#define IMF_INSTRUMENT_DRAWBAR_ORGAN				0x11
#define IMF_INSTRUMENT_PERCUSSIVE_ORGAN				0x12
#define IMF_INSTRUMENT_ROCK_ORGAN					0x13
#define IMF_INSTRUMENT_CHURCH_ORGAN					0x14
#define IMF_INSTRUMENT_REED_ORGAN					0x15
#define IMF_INSTRUMENT_ACCORDION					0x16
#define IMF_INSTRUMENT_HARMONICA					0x17
#define IMF_INSTRUMENT_TANGO_ACCORDION				0x18
#define IMF_INSTRUMENT_ACOUSTIC_GUITAR_NYLON		0x19
#define IMF_INSTRUMENT_ACOUSTIC_GUITAR_STEEL		0x1A
#define IMF_INSTRUMENT_ELECTRIC_GUITAR_JAZZ			0x1B
#define IMF_INSTRUMENT_ELECTRIC_GUITAR_CLEAN		0x1C
#define IMF_INSTRUMENT_ELECTRIC_GUITAR_MUTED		0x1D
#define IMF_INSTRUMENT_OVERDRIVEN_GUITAR			0x1E
#define IMF_INSTRUMENT_DISTORTION_GUITAR			0x1F
#define IMF_INSTRUMENT_GUITAR_HARMONICS				0x20
#define IMF_INSTRUMENT_ACOUSTIC_BASS				0x21
#define IMF_INSTRUMENT_ELECTRIC_BASS_FINGER			0x22
#define IMF_INSTRUMENT_ELECTRIC_BASS_PICK			0x23
#define IMF_INSTRUMENT_FRETLESS_BASS				0x24
#define IMF_INSTRUMENT_SLAP_BASS_1					0x25
#define IMF_INSTRUMENT_SLAP_BASS_2					0x26
#define IMF_INSTRUMENT_SYNTH_BASS_1					0x27
#define IMF_INSTRUMENT_SYNTH_BASS_2					0x28
#define IMF_INSTRUMENT_VIOLIN						0x29
#define IMF_INSTRUMENT_VIOLA						0x2A
#define IMF_INSTRUMENT_CELLO						0x2B
#define IMF_INSTRUMENT_CONTRABASS					0x2C
#define IMF_INSTRUMENT_TREMOLO_STRINGS				0x2D
#define IMF_INSTRUMENT_PIZZICATO_STRINGS			0x2E
#define IMF_INSTRUMENT_ORCHESTRAL_HARP				0x2F
#define IMF_INSTRUMENT_TIMPANI						0x30
#define IMF_INSTRUMENT_STRING_ENSEMBLE_1			0x31
#define IMF_INSTRUMENT_STRING_ENSEMBLE_2			0x32
#define IMF_INSTRUMENT_SYNTHSTRINGS_1				0x33
#define IMF_INSTRUMENT_SYNTHSTRINGS_2				0x34
#define IMF_INSTRUMENT_CHOIR_AAHS					0x35
#define IMF_INSTRUMENT_VOICE_OOHS					0x36
#define IMF_INSTRUMENT_SYNTH_VOICE					0x37
#define IMF_INSTRUMENT_ORCHESTRA_HIT				0x38
#define IMF_INSTRUMENT_TRUMPET						0x39
#define IMF_INSTRUMENT_TROMBONE						0x3A
#define IMF_INSTRUMENT_TUBA							0x3B
#define IMF_INSTRUMENT_MUTED_TRUMPET				0x3C
#define IMF_INSTRUMENT_FRENCH_HORN					0x3D
#define IMF_INSTRUMENT_BRASS_SECTION				0x3E
#define IMF_INSTRUMENT_SYNTHBRASS_1					0x3F
#define IMF_INSTRUMENT_SYNTHBRASS_2					0x40
#define IMF_INSTRUMENT_SOPRANO_SAX					0x41
#define IMF_INSTRUMENT_ALTO_SAX						0x42
#define IMF_INSTRUMENT_TENOR_SAX					0x43
#define IMF_INSTRUMENT_BARITONE_SAX					0x44
#define IMF_INSTRUMENT_OBOE							0x45
#define IMF_INSTRUMENT_ENGLISH_HORN					0x46
#define IMF_INSTRUMENT_BASSOON						0x47
#define IMF_INSTRUMENT_CLARINET						0x48
#define IMF_INSTRUMENT_PICCOLO						0x49
#define IMF_INSTRUMENT_FLUTE						0x4A
#define IMF_INSTRUMENT_RECORDER						0x4B
#define IMF_INSTRUMENT_PAN_FLUTE					0x4C
#define IMF_INSTRUMENT_BLOWN_BOTTLE					0x4D
#define IMF_INSTRUMENT_SHAKUHACHI					0x4E
#define IMF_INSTRUMENT_WHISTLE						0x4F
#define IMF_INSTRUMENT_OCARINA						0x50
#define IMF_INSTRUMENT_LEAD_1_SQUARE				0x51
#define IMF_INSTRUMENT_LEAD_2_SAWTOOTH				0x52
#define IMF_INSTRUMENT_LEAD_3_CALLIOPE				0x53
#define IMF_INSTRUMENT_LEAD_4_CHIFF					0x54
#define IMF_INSTRUMENT_LEAD_5_CHARANG				0x55
#define IMF_INSTRUMENT_LEAD_6_VOICE					0x56
#define IMF_INSTRUMENT_LEAD_7_FIFTHS				0x57
#define IMF_INSTRUMENT_LEAD_8_BASS_LEAD				0x58
#define IMF_INSTRUMENT_PAD_1_NEW_AGE				0x59
#define IMF_INSTRUMENT_PAD_2_WARM					0x5A
#define IMF_INSTRUMENT_PAD_3_POLYSYNTH				0x5B
#define IMF_INSTRUMENT_PAD_4_CHOIR					0x5C
#define IMF_INSTRUMENT_PAD_5_BOWED					0x5D
#define IMF_INSTRUMENT_PAD_6_METALLIC				0x5E
#define IMF_INSTRUMENT_PAD_7_HALO					0x5F
#define IMF_INSTRUMENT_PAD_8_SWEEP					0x60
#define IMF_INSTRUMENT_FX_1_RAIN					0x61
#define IMF_INSTRUMENT_FX_2_SOUNDTRACK				0x62
#define IMF_INSTRUMENT_FX_3_CRYSTAL					0x63
#define IMF_INSTRUMENT_FX_4_ATMOSPHERE				0x64
#define IMF_INSTRUMENT_FX_5_BRIGHTNESS				0x65
#define IMF_INSTRUMENT_FX_6_GOBLINS					0x66
#define IMF_INSTRUMENT_FX_7_ECHOES					0x67
#define IMF_INSTRUMENT_FX_8_SCI_FI					0x68
#define IMF_INSTRUMENT_SITAR						0x69
#define IMF_INSTRUMENT_BANJO						0x6A
#define IMF_INSTRUMENT_SHAMISEN						0x6B
#define IMF_INSTRUMENT_KOTO							0x6C
#define IMF_INSTRUMENT_KALIMBA						0x6D
#define IMF_INSTRUMENT_BAG_PIPE						0x6E
#define IMF_INSTRUMENT_FIDDLE						0x6F
#define IMF_INSTRUMENT_SHANAI						0x70
#define IMF_INSTRUMENT_TINKLE_BELL					0x71
#define IMF_INSTRUMENT_AGOGO						0x72
#define IMF_INSTRUMENT_STEEL_DRUMS					0x73
#define IMF_INSTRUMENT_WOODBLOCK					0x74
#define IMF_INSTRUMENT_TAIKO_DRUM					0x75
#define IMF_INSTRUMENT_MELODIC_TOM					0x76
#define IMF_INSTRUMENT_SYNTH_DRUM					0x77
#define IMF_INSTRUMENT_REVERSE_CYMBAL				0x78
#define IMF_INSTRUMENT_GUITAR_FRET_NOISE			0x79
#define IMF_INSTRUMENT_BREATH_NOISE					0x7A
#define IMF_INSTRUMENT_SEASHORE						0x7B
#define IMF_INSTRUMENT_BIRD_TWEET					0x7C
#define IMF_INSTRUMENT_TELEPHONE_RING				0x7D
#define IMF_INSTRUMENT_HELICOPTER					0x7E
#define IMF_INSTRUMENT_APPLAUSE						0x7F
#define IMF_INSTRUMENT_GUNSHOT						0x80

#define IMF_NOTE_C                                   0x00
#define IMF_NOTE_CH                                  0x01
#define IMF_NOTE_D                                   0x02
#define IMF_NOTE_DH                                  0x03
#define IMF_NOTE_E                                   0x04
#define IMF_NOTE_F                                   0x05
#define IMF_NOTE_FH                                  0x06
#define IMF_NOTE_G                                   0x07
#define IMF_NOTE_GH                                  0x08
#define IMF_NOTE_A                                   0x09
#define IMF_NOTE_AH                                  0x0A
#define IMF_NOTE_B                                   0x0B
#define IMF_NOTE_COUNT                               0x0C

#define IMF_OCTAVE_0                                 0x00
#define IMF_OCTAVE_1                                 0x01
#define IMF_OCTAVE_2                                 0x02
#define IMF_OCTAVE_3                                 0x03
#define IMF_OCTAVE_4                                 0x04
#define IMF_OCTAVE_5                                 0x05
#define IMF_OCTAVE_6                                 0x06
#define IMF_OCTAVE_7                                 0x07
#define IMF_OCTAVE_8                                 0x08
#define IMF_OCTAVE_9                                 0x09

#define IMF_NOTE( o, n )                                ( ( o ) * IMF_NOTE_COUNT + ( n ) )

char * strtoupper( char * _string );
char * basename( char * _path );

#define BUILD_CHECK_FILETYPE(_environment, _filetype) \
    if ( _environment->outputFileType != _filetype ) { \
        CRITICAL_UNSUPPORTED_OUTPUT_FILE_TYPE( OUTPUT_FILE_TYPE_AS_STRING[_filetype] ); \
    }

#define BUILD_SAFE_REMOVE(_environment, filename) \
    system_remove_safe( _environment, filename );

#define BUILD_TOOLCHAIN_CC65_GET_EXECUTABLE( _environment, executableName ) \
    if ( _environment->compilerFileName ) { \
        sprintf( executableName, "%s", _environment->compilerFileName ); \
    } else if( access( "cc65\\bin\\cl65.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "cc65\\bin\\cl65.exe" ); \
    } else if( access( "modules\\cc65\\bin\\cl65.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "cc65\\bin\\cl65.exe" ); \
    } else if( access( "..\\modules\\cc65\\bin\\cl65.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "..\\modules\\cc65\\bin\\cl65.exe" ); \
    } else if( access( "cc65/bin/cl65", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "cc65/bin/cl65" ); \
    } else if( access( "modules//cc65/bin/cl65", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "modules/cc65/bin/cl65" ); \
    } else if( access( "../modules//cc65/bin/cl65", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "../modules/cc65/bin/cl65" ); \
    } else { \
        sprintf(executableName, "%s", "cl65" ); \
    }

#define BUILD_TOOLCHAIN_CC65_GET_LISTING_FILE( _environment, listingFileName ) \
    memset( listingFileName, 0, MAX_TEMPORARY_STORAGE ); \
    if ( _environment->listingFileName ) { \
        sprintf( listingFileName, "-l \"%s\"", _environment->listingFileName ); \
    } else { \
        strcpy( listingFileName, "" ); \
    }

#define BUILD_TOOLCHAIN_CC65_EXEC( _environment, target, executableName, listingFileName, additionalParameters ) \
    sprintf( commandLine, "\"%s\" %s -o \"%s\" %s -t %s -C \"%s\" \"%s\"", \
        executableName, \
        listingFileName, \
        _environment->exeFileName, \
        additionalParameters, \
        target, \
        _environment->configurationFileName, \
        _environment->asmFileName ); \
    if ( system_call( _environment,  commandLine ) ) { \
        printf("The compilation of assembly program failed.\n\n"); \
        printf("Please check if %s is correctly installed.\n\n", executableName); \
        printf("For more info, please visit: https://ugbasic.iwashere.eu/install.\n\n"); \
    };

#define BUILD_TOOLCHAIN_Z88DK_GET_EXECUTABLE_Z80ASM( _environment, executableName ) \
    if ( _environment->compilerFileName ) { \
        sprintf( executableName, "%s", _environment->compilerFileName ); \
    } else if( access( "modules\\z88dk\\src\\z80asm\\z88dk-z80asm.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "..\\modules\\z88dk\\src\\z80asm\\z88dk-z80asm.exe" ); \
    } else if( access( "..\\modules\\z88dk\\src\\z80asm\\z88dk-z80asm.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "..\\modules\\z88dk\\src\\z80asm\\z88dk-z80asm.exe" ); \
    } else if( access( "modules/z88dk/src/z80asm/z88dk-z80asm", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "../modules/z88dk/src/z80asm/z88dk-z80asm" ); \
    } else if( access( "../modules/z88dk/src/z80asm/z88dk-z80asm", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "../modules/z88dk/src/z80asm/z88dk-z80asm" ); \
    } else { \
        sprintf(executableName, "%s", "z88dk-z80asm" ); \
    }

#define BUILD_TOOLCHAIN_Z88DK_GET_LISTING_FILE( _environment, listingFileName ) \
    (void) listingFileName; \
    if ( _environment->listingFileName ) { \
        sprintf( listingFileName, "-l -m -s -g" ); /* -m -s -g */ \
    } else { \
        strcpy( listingFileName, "-m -s -g" ); \
    }

#define BUILD_TOOLCHAIN_Z88DK_EXEC( _environment, target, executableName, listingFileName ) \
    sprintf( commandLine, "\"%s\" %s -D__%s__ -b \"%s\"", \
        executableName, \
        listingFileName, \
        target, \
        _environment->asmFileName ); \
    if ( system_call( _environment,  commandLine ) ) { \
        printf("The compilation of assembly program failed.\n\n"); \
        printf("Please check if %s is correctly installed.\n\n", executableName); \
        printf("For more info, please visit: https://ugbasic.iwashere.eu/install.\n\n"); \
        return; \
    }; \
    if ( _environment->listingFileName ) { \
        char * p = strdup( _environment->asmFileName ); \
        char * q = strchr( p, '.' ); \
        if ( q ) { \
            strcpy( q, ".lis" ); \
        } \
        rename( p, _environment->listingFileName ); \
    }

#define BUILD_TOOLCHAIN_Z88DK_GET_EXECUTABLE_APPMAKE( _environment, executableName ) \
    if ( _environment->appMakerFileName ) { \
        sprintf( executableName, "%s", _environment->appMakerFileName ); \
    } else if( access( "modules\\z88dk\\src\\appmake\\z88dk-appmake.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "..\\modules\\z88dk\\src\\appmake\\z88dk-appmake.exe" ); \
    } else if( access( "..\\modules\\z88dk\\src\\appmake\\z88dk-appmake.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "..\\modules\\z88dk\\src\\appmake\\z88dk-appmake.exe" ); \
    } else if( access( "modules/z88dk/src/appmake/z88dk-appmake", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "../modules/z88dk/src/appmake/z88dk-appmake" ); \
    } else if( access( "../modules/z88dk/src/appmake/z88dk-appmake", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "../modules/z88dk/src/appmake/z88dk-appmake" ); \
    } else { \
        sprintf(executableName, "%s", "z88dk-appmake" ); \
    }

#define BUILD_TOOLCHAIN_ASM6809_GET_EXECUTABLE( _environment, executableName ) \
    if ( _environment->compilerFileName ) { \
        sprintf(executableName, "%s", _environment->compilerFileName ); \
    } else if( access( "modules\\asm6809\\src\\asm6809.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "modules\\asm6809\\src\\asm6809.exe" ); \
    } else if( access( "..\\modules\\asm6809\\src\\asm6809.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "..\\modules\\asm6809\\src\\asm6809.exe" ); \
    } else if( access( "modules/asm6809/src/asm6809", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "modules/asm6809/src/asm6809" ); \
    } else if( access( "../modules/asm6809/src/asm6809", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "../modules/asm6809/src/asm6809" ); \
    } else if( access( "asm6809\\bin\\asm6809.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "asm6809\\bin\\asm6809.exe" ); \
    } else { \
        sprintf(executableName, "%s", "asm6809" ); \
    }

#define BUILD_TOOLCHAIN_ASM6809_GET_LISTING_FILE( _environment, listingFileName ) \
    memset( listingFileName, 0, MAX_TEMPORARY_STORAGE ); \
    if ( _environment->listingFileName ) { \
        sprintf( listingFileName, "-l \"%s\"", _environment->listingFileName ); \
    } else { \
        strcpy( listingFileName, "" ); \
    }

#define BUILD_TOOLCHAIN_ASM6809EXEC( _environment, flag, startingAddress, executableName, listingFileName ) \
    sprintf( commandLine, "\"%s\" %s -o \"%s\" %s -e %d \"%s\"", \
        executableName, \
        listingFileName, \
        _environment->exeFileName,  \
        flag, \
        startingAddress, \
        _environment->asmFileName ); \
    if ( system_call( _environment,  commandLine ) ) { \
        printf("The compilation of assembly program failed.\n\n"); \
        printf("Please check if %s is correctly installed.\n\n", executableName); \
        printf("For more informations, please visit: https://ugbasic.iwashere.eu/install.\n\n"); \
    };

#define BUILD_TOOLCHAIN_DECB_GET_EXECUTABLE( _environment, executableName ) \
    if ( _environment->decbFileName ) { \
        sprintf(executableName, "%s", _environment->decbFileName ); \
    } else if( access( "modules\\toolshed\\build\\unix\\decb\\decb.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "modules\\toolshed\\build\\unix\\decb\\decb.exe" ); \
    } else if( access( "modules/toolshed/build/unix/decb/decb", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "modules/toolshed/build/unix/decb/decb" ); \
    } else if( access( "..\\modules\\toolshed\\build\\unix\\decb\\decb.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "..\\modules\\toolshed\\build\\unix\\decb\\decb.exe" ); \
    } else if( access( "../modules/toolshed/build/unix/decb/decb", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "../modules/toolshed/build/unix/decb/decb" ); \
    } else { \
        sprintf(executableName, "%s", "decb" ); \
    }

#define BUILD_TOOLCHAIN_DECB( _environment, executableName, binaryFileName ) \
    sprintf( commandLine, "\"%s\" dskini \"%s\"", \
        executableName, \
        _environment->exeFileName ); \
    if ( system_call( _environment,  commandLine ) ) { \
        printf("The compilation of assembly program failed.\n\n"); \
        printf("Please check if %s is correctly installed.\n\n", executableName); \
        printf("For more informations, please visit: https://ugbasic.iwashere.eu/install.\n\n"); \
    }; \
    sprintf( commandLine, "\"%s\" copy -2 \"%s\" \"%s,%s\"", \
        executableName, \
        binaryFileName,  \
        _environment->exeFileName, \
        strtoupper( basename( binaryFileName ) ) ); \
    if ( system_call( _environment,  commandLine ) ) { \
        printf("The compilation of assembly program failed.\n\n"); \
        printf("Please check if %s is correctly installed.\n\n", executableName); \
        printf("For more informations, please visit: https://ugbasic.iwashere.eu/install.\n\n"); \
    };

#define BUILD_TOOLCHAIN_DIR2ATR_GET_EXECUTABLE( _environment, executableName ) { \
    if ( _environment->dir2atrFileName ) { \
        sprintf(executableName, "%s", _environment->dir2atrFileName ); \
    } else if( access( "..\\modules\\atarisio\\tools\\dir2atr.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "..\\modules\\atarisio\\tools\\dir2atr.exe" ); \
    } else if( access( "../modules/atarisio/tools/dir2atr", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "../modules/atarisio/tools/dir2atr" ); \
    } else if( access( "modules\\atarisio\\tools\\dir2atr.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "modules\\atarisio\\tools\\dir2atr.exe" ); \
    } else if( access( "modules/atarisio/tools/dir2atr", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "modules/atarisio/tools/dir2atr" ); \
    } else { \
        sprintf(executableName, "%s", "dir2atr" ); \
    } \
}

#define BUILD_TOOLCHAIN_DSKTOOLS_GET_EXECUTABLE( _environment, executableName ) { \
    if ( _environment->dsktoolsFileName ) { \
        sprintf(executableName, "%s", _environment->dsktoolsFileName ); \
    } else if( access( "..\\modules\\dsktools\\bin\\cptodsk.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "..\\ugbasic\\modules\\dsktools\\bin\\cptodsk.exe" ); \
    } else if( access( "../modules/dsktools/bin/cptodsk", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "../modules/dsktools/bin/cptodsk" ); \
    } else if( access( "modules\\dsktools\\bin\\cptodsk.exe", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "modules\\dsktools\\bin\\cptodsk.exe" ); \
    } else if( access( "modules/dsktools/bin/cptodsk", F_OK ) == 0 ) { \
        sprintf(executableName, "%s", "modules/dsktools/bin/cptodsk" ); \
    } else { \
        sprintf(executableName, "%s", "cptodsk" ); \
    } \
}

#define BUILD_TOOLCHAIN_DIR2ATR( _environment, executableName, bootCodePath, contentPath, atrFileName, pipes ) \
    { \
        int pathSeparatorChanged = 0; \
        if ( contentPath[strlen(contentPath)-1] == PATH_SEPARATOR ) { \
            contentPath[strlen(contentPath)-1] = 0; \
            pathSeparatorChanged = 1; \
        } \
        sprintf( commandLine, "\"%s\" -S -p -B \"%s\" \"%s\" \"%s\" %s", \
            executableName, \
            bootCodePath, \
            atrFileName, \
            contentPath, \
            pipes \
            ); \
        if ( system_call( _environment,  commandLine ) ) { \
            printf("The compilation of assembly program failed.\n\n"); \
            printf("Please check if %s is correctly installed.\n\n", executableName); \
            printf("For more informations, please visit: https://ugbasic.iwashere.eu/install.\n\n"); \
        }; \
        if ( pathSeparatorChanged ) { \
            contentPath[strlen(contentPath)] = PATH_SEPARATOR; \
        } \
    }

void setup_embedded( Environment *_environment );
void begin_compilation( Environment * _environment );
void target_initialization( Environment *_environment );
void shell_injection( Environment * _environment );
void target_finalization( Environment * _environment );
void target_analysis( Environment * _environment );
void target_deep_analyzer( Environment * _environment );
void end_compilation( Environment * _environment );
void target_peephole_optimizer( Environment * _environment );
void begin_build( Environment * _environment );
void target_linkage( Environment *_environment );
void target_finalize( Environment * _environment );
void target_cleanup( Environment *_environment );
void end_build( Environment * _environment );
void bank_cleanup( Environment * _environment );
void gameloop_cleanup( Environment * _environment );
void linker_cleanup( Environment * _environment );
void linker_setup( Environment * _environment );
int pattern_match( char * _pattern, char * _value );
void setup_text_variables( Environment * _environment );
ScreenMode * find_screen_mode_by_suggestion( Environment * _environment, int _bitmap, int _width, int _height, int _colors, int _tile_width, int _tile_height );
ScreenMode * find_screen_mode_by_id( Environment * _environment, int _id );
Bank * bank_find( Bank * _first, char * _name );

#define FUNCTION_STUB( t )   Variable * result = variable_temporary( _environment, t, "(stub)" ); return result;

POBuffer po_buf_del( POBuffer buf );
POBuffer po_buf_new( int size );
POBuffer po_buf_cat(POBuffer buf, char *string);
POBuffer po_buf_cpy(POBuffer buf, char *string);
POBuffer po_buf_add(POBuffer buf, char c);
POBuffer po_buf_vprintf(POBuffer buf, const char *fmt, va_list ap);
POBuffer po_buf_printf(POBuffer buf, const char *fmt, ...);
POBuffer po_buf_fgets(POBuffer buf, FILE *f);
int po_buf_trim(POBuffer buf);
int po_buf_cmp(POBuffer a, POBuffer b);
POBuffer tmp_buf(void *key1, unsigned int key2);
void tmp_buf_clr(void *key1);
POBuffer po_buf_match(POBuffer _buf, const char *_pattern, ...);
int po_buf_strcmp(POBuffer _s, POBuffer _t);

#define TMP_BUF         tmp_buf(__FILE__, __LINE__)
#define TMP_BUF_CLR     tmp_buf_clr(__FILE__)

char * get_default_temporary_path( );
char * find_last_path_separator( char * _path );

//----------------------------------------------------------------------------
// Common functions used by parser only
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Array
//----------------------------------------------------------------------------

void parser_array_init( Environment * _environment );
void parser_array_index_symbolic( Environment * _environment, char * _index );
void parser_array_index_numeric( Environment * _environment, int _index );
void parser_array_cleanup( Environment * _environment );

//----------------------------------------------------------------------------
// Other
//----------------------------------------------------------------------------

Variable * parser_adapted_numeric( Environment * _environment, int _number );
Variable * parser_casted_numeric( Environment * _environment, VariableType _type, int _number );

//----------------------------------------------------------------------------
// Common accessibile functions used by language and parser
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// *A*
//----------------------------------------------------------------------------

Variable *              absolute( Environment * _environment, char * _value );
void                    add_complex( Environment * _environment, char * _variable, int _expression, int _limit_lower, int _limit_upper );
void                    add_complex_vars( Environment * _environment, char * _variable, char * _expression, char * _limit_lower, char * _limit_upper );
void                    add_complex_array( Environment * _environment, char * _variable, char * _expression, char * _limit_lower, char * _limit_upper );
void                    add_complex_mt( Environment * _environment, char * _variable, char * _expression, char * _limit_lower, char * _limit_upper );
char *                  address_displacement( Environment * _environment, char * _address, char * _displacement );
void                    allow( Environment * _environment );

//----------------------------------------------------------------------------
// *B*
//----------------------------------------------------------------------------

void                    back( Environment * _environment, char * _color );
Bank *                  bank_define( Environment * _environment, char * _name, BankType _type, int _address, char * _filename );
Variable *              bank_get( Environment * _environment );
Variable *              bank_get_address( Environment * _environment, int _bank );
Variable *              bank_get_address_var( Environment * _environment, char * _bank );
Variable *              bank_get_size( Environment * _environment, int _bank );
Variable *              bank_get_size_var( Environment * _environment, char * _bank );
Variable *              bank_get_count( Environment * _environment );
void                    bank_read_semi_var( Environment * _environment, int _bank, int _address1, char * _address2, int _size );
void                    bank_read_vars( Environment * _environment, char * _bank, char * _address1, char * _address2, char * _size );
void                    bank_read_vars_direct( Environment * _environment, char * _bank, char * _address1, char * _address2, char * _size );
void                    bank_uncompress_semi_var( Environment * _environment, int _bank, int _address1, char * _address2 );
void                    bank_uncompress_vars( Environment * _environment, char * _bank, char * _address1, char * _address2 );
void                    bank_set( Environment * _environment, int _bank );
void                    bank_set_var( Environment * _environment, char * _bank );
void                    bank_write_vars( Environment * _environment, char * _bank, char * _address1, char * _address2, char * _size );
void                    bar( Environment * _environment, char * _x0, char * _y0, char * _x1, char * _y1, char * _c );
void                    begin_for( Environment * _environment, char * _index, char * _from, char * _to );  
void                    begin_for_from( Environment * _environment, char * _index, char * _from, char * _to, char * _step );  
void                    begin_for_from_mt( Environment * _environment, char * _index, char * _from, char * _to, char * _step );  
void                    begin_for_identifier( Environment * _environment, char * _index );
void                    begin_for_identifier_mt( Environment * _environment, char * _index );
void                    begin_for_mt( Environment * _environment, char * _index, char * _from, char * _to );  
void                    begin_for_mt_step( Environment * _environment, char * _index, char * _from, char * _to, char * _step );  
void                    begin_for_step( Environment * _environment, char * _index, char * _from, char * _to, char * _step );  
void                    begin_for_step_prepare( Environment * _environment, char * _from, char * _to, char * _step );
void                    begin_for_step_prepare_mt( Environment * _environment, char * _from, char * _to, char * _step );
void                    begin_for_to( Environment * _environment, char *_to );
void                    begin_for_to_prepare( Environment * _environment );
void                    begin_for_to_prepare_mt( Environment * _environment );
void                    begin_for_to_mt( Environment * _environment, char *_to );
void                    begin_gameloop( Environment * _environment );
void                    begin_loop( Environment * _environment );
void                    begin_procedure( Environment * _environment, char * _name );
void                    begin_repeat( Environment * _environment );
void                    begin_storage( Environment * _environment, char * _name, char * _file_name );
void                    begin_while( Environment * _environment );
void                    begin_while_condition( Environment * _environment, char * _expression );
void                    bell( Environment * _environment, int _note, int _channels );
void                    bell_vars( Environment * _environment, char * _note, char * _channels );
void                    bitmap_at( Environment * _environment, int _address );
void                    bitmap_at_var( Environment * _environment, char * _address );
void                    bitmap_clear( Environment * _environment );
void                    bitmap_clear_with( Environment * _environment, int _value );
void                    bitmap_clear_with_vars( Environment * _environment, char * _value );
void                    bitmap_disable( Environment * _environment );
void                    bitmap_enable( Environment * _environment, int _width, int _height, int _colors );
void                    blit_define( Environment * _environment, char * _name, int _sop, int _mop, int _smop, int _iop, int _dop, int _idop, int _top );
void                    blit_define_begin_compound( Environment * _environment, char * _name );
void                    blit_define_compound_binary( Environment * _environment, int _operation, int _operand1, int _operand2, int _result );
void                    blit_define_compound_unary( Environment * _environment, int _operation, int _operand, int _result );
void                    blit_define_compound_operand_to_register( Environment * _environment, int _register, int _source );
void                    blit_define_end_compound( Environment * _environment, int _register );
void                    blit_image( Environment * _environment, char * _blit, char * _x, char * _y, char * _frame, char * _sequence, int _flags );
void                    boom( Environment * _environment, int _channels );
void                    boom_var( Environment * _environment, char * _channels );
void                    box( Environment * _environment, char * _x1, char * _y1, char * _x2, char * _y2, char * _c );
Resource *              build_resource_for_sequence( Environment * _environment, char * _image, char * _frame, char * _sequence );

//----------------------------------------------------------------------------
// *C*
//----------------------------------------------------------------------------

int                     calculate_nearest_tile( TileDescriptor * _tile, TileDescriptors * _tiles );
int                     calculate_exact_tile( TileDescriptor * _tile, TileDescriptors * _tiles );
int                     calculate_tile_affinity( TileDescriptor * _first, TileDescriptor * _second );
TileDescriptor *        calculate_tile_descriptor( TileData * _tileData );
Variable *              calculate_frame_by_type( Environment * _environment, TsxTileset * _tileset, char * _images, char * _description );
void                    call_procedure( Environment * _environment, char * _name );
void                    case_else( Environment * _environment );
void                    case_equals( Environment * _environment, int _value );
void                    case_equals_var( Environment * _environment, char * _value );
void                    case_equals_label( Environment * _environment );
void                    center( Environment * _environment, char * _string, int _newline );
int                     check_if_filename_is_valid( Environment * _environment,  char * _filename );
void                    circle( Environment * _environment, char * _x, char * _y, char * _r, char *_c );
void                    clear( Environment * _environment );
Variable *              clear_key( Environment * _environment );
void                    cline( Environment * _environment, char * _characters );
void                    clip( Environment * _environment, char * _x1, char * _y1, char * _x2, char * _y2 );
void                    cls( Environment * _environment, char * _paper );
void                    cmove( Environment * _environment, char * _dx, char * _dy );
void                    cmove_direct( Environment * _environment, int _dx, int _dy );
Variable *              collision_to( Environment * _environment, int _sprite );
Variable *              collision_to_vars( Environment * _environment, char * _sprite );
void                    color( Environment * _environment, int _index, int _shade );
Variable *              color_get_vars( Environment * _environment, char * _index );
void                    color_semivars( Environment * _environment, int _index, char * _shade );
void                    color_vars( Environment * _environment, char * _index, char * _shade );
void                    color_background( Environment * _environment, int _index, int _background_color );
void                    color_background_vars( Environment * _environment, char * _index, char * _background_color );
void                    color_border( Environment * _environment, int _border_color );
void                    color_border_var( Environment * _environment, char * _border_color );
void                    color_sprite( Environment * _environment, int _index, int _color );
void                    color_sprite_vars( Environment * _environment, char * _sprite, char * _color );
void                    colormap_at( Environment * _environment, int _address );
void                    colormap_at_var( Environment * _environment, char * _address );
void                    colormap_clear( Environment * _environment );
void                    colormap_clear_with( Environment * _environment, int _foreground, int _background );
void                    colormap_clear_with_vars( Environment * _environment, char * _foreground, char * _background );
void                    const_define_numeric( Environment * _environment, char * _name, int _value );
void                    const_define_string( Environment * _environment, char * _name, char * _value );
void                    const_define_float( Environment * _environment, char * _name, double _value );
void                    const_emit( Environment * _environment, char * _name );
Constant *              constant_find( Constant * _constant, char * _name );
Variable *              csprite_init( Environment * _environment, char * _image, char * _sprite, int _flags );

//----------------------------------------------------------------------------
// *D*
//----------------------------------------------------------------------------

void                    data_numeric( Environment * _environment, int _value );
void                    data_floating( Environment * _environment, double _value );
DataSegment *           data_segment_define( Environment * _environment, char * _name );
DataSegment *           data_segment_define_numeric( Environment * _environment, int _number );
DataSegment *           data_segment_find( Environment * _environment, char * _name );
DataSegment *           data_segment_find_numeric( Environment * _environment, int _number );
DataSegment *           data_segment_define_or_retrieve( Environment * _environment, char * _name );
DataSegment *           data_segment_define_or_retrieve_numeric( Environment * _environment, int _number );
void                    data_string( Environment * _environment, char * _value );
void                    declare_procedure( Environment * _environment, char * _name, int _address, int _system );
void                    defdgr_vars( Environment * _environment, char * _character, char * _b0, char * _b1, char * _b2, char * _b3, char * _b4, char * _b5, char * _b6, char * _b7 );
Variable *              distance( Environment * _environment, char * _x1, char * _y1, char * _x2, char * _y2 );
void                    dload( Environment * _environment, char * _filename, char * _offset, char * _address, char * _size );
void                    double_buffer( Environment * _environment, int _enabled );
void                    draw( Environment * _environment, char * _x0, char * _y0, char * _x1, char * _y1, char * _c );
void                    draw_tile_column( Environment * _environment, char * _tile, char * _x, char * _y1, char * _y2, char * _color );
void                    draw_tile_row( Environment * _environment, char * _tile, char * _y, char * _x1, char * _x2, char * _color );
void                    draw_string( Environment * _environment, char * _string );
void                    dsave( Environment * _environment, char * _filename, char * _offset, char * _address, char * _size );
void                    dstring_cleanup( Environment * _Environment );

//----------------------------------------------------------------------------
// *E*
//----------------------------------------------------------------------------

void                    ellipse( Environment * _environment, char * _x, char * _y, char * _rx, char * _ry, char * _c );
void                    else_if_then( Environment * _environment, char * _expression );
void                    else_if_then_label( Environment * _environment );
void                    end( Environment * _environment );
void                    end_for( Environment * _environment );
void                    end_for_identifier( Environment * _environment, char * _identifier );
void                    end_gameloop( Environment * _environment );
void                    end_if_then( Environment * _environment  );
void                    end_loop( Environment * _environment );
void                    end_procedure( Environment * _environment, char * _value );
void                    end_repeat( Environment * _environment );
void                    end_repeat_condition( Environment * _environment, char * _expression );
void                    end_select_case( Environment * _environment );
void                    end_storage( Environment * _environment );
void                    end_while( Environment * _environment );
char *                  escape_newlines( char * _string );
void                    every_cleanup( Environment * _environment );
void                    every_off( Environment * _environment, char * _timer );
void                    every_on( Environment * _environment, char * _timer );
void                    every_ticks_call( Environment * _environment, char * _timing, char * _label, char * _timer );
void                    every_ticks_gosub( Environment * _environment, char * _timing, char * _label, char * _timer );
void                    exit_loop( Environment * _environment, int _number );
void                    exit_loop_if( Environment * _environment, char * _expression, int _number );
void                    exit_proc_if( Environment * _environment, char * _expression, char * _value );
void                    exit_procedure( Environment * _environment );

//----------------------------------------------------------------------------
// *F*
//----------------------------------------------------------------------------

void                    file_storage( Environment * _environment, char * _source_name, char *_target_name );
int                     find_frame_by_type( Environment * _environment, TsxTileset * _tileset, char * _images, char * _description );
void                    font_descriptors_init( Environment * _environment, int _embedded_present );
void                    forbid( Environment * _environment );
int                     frames( Environment * _environment, char * _image );
Variable *              fp_cos( Environment * _environment, char * _angle );
Variable *              fp_sin( Environment * _environment, char * _angle );
Variable *              fp_tan( Environment * _environment, char * _angle );

//----------------------------------------------------------------------------
// *G*
//----------------------------------------------------------------------------

Variable *              get_at( Environment * _environment, char * _x, char * _y );
Variable *              get_cmove( Environment * _environment, char * _x, char * _y );
Variable *              get_cmove_direct( Environment * _environment, int _x, int _y );
void                    get_image( Environment * _environment, char * _image, char * _x1, char * _y1, char * _x2, char * _y2, char * _frame, char * _sequence, int _palette );
void                    get_image_overwrite_size( Environment * _environment, char * _image, char * _x1, char * _y1, char * _x2, char * _y2 );
Variable *              get_paper( Environment * _environment, char * _color );
Variable *              get_pen( Environment * _environment, char * _color );
Variable *              get_raster_line( Environment * _environment );
Variable *              get_tab( Environment * _environment );
char *                  get_temporary_filename( Environment * _environment );
Variable *              get_ticks_per_second( Environment * _environment );
Variable *              get_timer( Environment * _environment );
void                    global( Environment * _environment );
void                    gosub_label( Environment * _environment, char * _label );
void                    gosub_number( Environment * _environment, int _number );
void                    goto_label( Environment * _environment, char * _label );
void                    goto_number( Environment * _environment, int _number );
void                    graphic( Environment * _environment );
void                    gr_locate( Environment * _environment, char * _x, char * _y );

//----------------------------------------------------------------------------
// *H*
//----------------------------------------------------------------------------

void                    halt( Environment * _environment );
Variable *              hit_to( Environment * _environment, int _sprite );
Variable *              hit_to_vars( Environment * _environment, char * _sprite );
void                    home( Environment * _environment );

//----------------------------------------------------------------------------
// *I*
//----------------------------------------------------------------------------

void                    if_then( Environment * _environment, char * _expression );
char *                  image_cut( Environment * _environment, char * _source, int _x, int _y, int _width, int _height );
char *                  image_flip_x( Environment * _environment, char * _source, int _width, int _height, int _depth );
char *                  image_flip_y( Environment * _environment, char * _source, int _width, int _height, int _depth );
Variable *              image_load( Environment * _environment, char * _filename, char * _alias, int _mode, int _flags, int _transparent_color, int _background_color, int _bank_expansion );
Variable *              image_load_from_buffer( Environment * _environment, char * _buffer, int _buffer_size );
int                     image_size( Environment * _environment, int _width, int _height );
Variable *              image_converter( Environment * _environment, char * _data, int _width, int _height, int _depth, int _offset_x, int _offset_y, int _frame_width, int _frame_height, int _mode, int _transparent_color, int _flags );
void                    image_converter_asserts( Environment * _environment, int _width, int _height, int _offset_x, int _offset_y, int * _frame_width, int * _frame_height );
void                    image_converter_asserts_free_height( Environment * _environment, int _width, int _height, int _offset_x, int _offset_y, int * _frame_width, int * _frame_height );
Variable *              image_extract( Environment * _environment, char * _images, int _frame, int * _sequence );
char *                  image_extract_subimage( Environment * _environment, char * _source, int _width, int _height, int _frame_width, int _frame_height, int _x, int _y, int _depth );
Variable *              image_get_height( Environment * _environment, char * _image );
Variable *              image_get_width( Environment * _environment, char * _image );
char *                  image_enlarge_right( Environment * _environment, char * _source, int _width, int _height, int _delta );
char *                  image_enlarge_bottom( Environment * _environment, char * _source, int _width, int _height, int _delta );
RGBi *                  image_nearest_system_color( RGBi * _color );
char *                  image_roll_x_left( Environment * _environment, char * _source, int _width, int _height );
char *                  image_roll_x_right( Environment * _environment, char * _source, int _width, int _height );
char *                  image_roll_y_down( Environment * _environment, char * _source, int _width, int _height );
Variable *              image_storage( Environment * _environment, char * _source_name, char *_target_name, int _mode, int _flags, int _transparent_color, int _background_color, int _bank_expansion );
Variable *              images_storage( Environment * _environment, char * _source_name, char *_target_name, int _mode, int _frame_width, int _frame_height, int _flags, int _transparent_color, int _background_color, int _bank_expansion );
Variable *              images_load( Environment * _environment, char * _filename, char * _alias, int _mode, int _frame_width, int _frame_height, int _flags, int _transparent_color, int _background_color, int _bank_expansion );
Variable *              images_load_from_buffer( Environment * _environment, char * _buffer, int _buffer_size );
Variable *              in_var( Environment * _environment, char * _port );
void                    ink( Environment * _environment, char * _expression );
Variable *              inkey( Environment * _environment );
void                    input( Environment * _environment, char * _variable, VariableType _default_type );
Variable *              input_string( Environment * _environment, char * _size );
void                    instrument( Environment * _environment, int _instrument, int _channels );
void                    instrument_semi_var( Environment * _environment, int _instrument, char * _channels );
void                    interleaved_instructions( Environment * _environment );


//----------------------------------------------------------------------------
// *J*
//----------------------------------------------------------------------------

Variable *              joy( Environment * _environment, int _port );
Variable *              joy_vars( Environment * _environment, char * _port );
Variable *              joy_direction( Environment * _environment, int _port, int _direction );
Variable *              joy_direction_semivars( Environment * _environment, char * _port, int _direction );

//----------------------------------------------------------------------------
// *K*
//----------------------------------------------------------------------------

Variable *              keystate( Environment * _environment, char * _scancode );
Variable *              keyshift( Environment * _environment );
Variable *              key_pressed( Environment * _environment, int _scancode );
Variable *              key_pressed_var( Environment * _environment, char * _scancode );
void                    kill_procedure( Environment * _environment, char * _handle );

//----------------------------------------------------------------------------
// *L*
//----------------------------------------------------------------------------

void                    label_define_numeric( Environment * _environment, int _label );
void                    label_define_named( Environment * _environment, char * _label );
int                     label_exists_named( Environment * _environment, char * _label );
int                     label_exists_numeric( Environment * _environment, int _label );
Variable *              load( Environment * _environment, char * _filename, char * _alias, int _at, int _bank_expansion, int _flags );
void                    locate( Environment * _environment, char * _x, char * _y );
void                    loop( Environment * _environment, char *_label );

//----------------------------------------------------------------------------
// *M*
//----------------------------------------------------------------------------

RGBi                *   malloc_palette( int _size );
float                   max_of_two(float _x, float _y);
float                   max_of_three(float _m, float _n, float _p);
Variable *              maximum( Environment * _environment, char * _source, char * _dest );
void                    memorize( Environment * _environment );
void                    memory_area_assign( MemoryArea * _first, Variable * _variable );
float                   min_of_two(float _x, float _y);
float                   min_of_three(float _m, float _n, float _p);
Variable *              minimum( Environment * _environment, char * _source, char * _dest );
void                    mmove_memory_memory( Environment * _environment, char * _from, char * _to, char * _size );
void                    mmove_memory_video( Environment * _environment, char * _from, char * _to, char * _size );
void                    mmove_video_memory( Environment * _environment, char * _from, char * _to, char * _size );
void                    move_tile( Environment * _environment, char * _tile, char * _x, char * _y );
Variable *              music_load( Environment * _environment, char * _filename, char * _alias, int _bank_expansion );
Variable *              music_load_to_variable( Environment * _environment, char * _filename, char * _alias, int _bank_expansion );
Variable *              music_storage( Environment * _environment, char * _filename, char * _alias, int _bank_expansion );
void                    music_var( Environment * _environment, char * _music, int _loop );

//----------------------------------------------------------------------------
// *N*
//----------------------------------------------------------------------------

void                    next_raster( Environment * _environment );
void                    next_raster_at_with( Environment * _environment, int _at, char * _with );
void                    next_raster_at_with_var( Environment * _environment, char * _var, char * _with );
Variable *              new_image( Environment * _environment, int _width, int _height, int _mode );
Variable *              new_images( Environment * _environment, int _frames, int _width, int _height, int _mode );
Variable *              new_music( Environment * _environment, int _size );
Variable *              new_sequence( Environment * _environment, int _sequences, int _frames, int _width, int _height, int _mode );

//----------------------------------------------------------------------------
// *O*
//----------------------------------------------------------------------------

void                    offsetting_add_variable_reference( Environment * _environment, Offsetting * _first, Variable * _var, int _sequence );
Offsetting *            offsetting_size_count( Environment * _environment, int _size, int _count );
void                    on_gosub( Environment * _environment, char * _expression );
void                    on_gosub_end( Environment * _environment );
void                    on_gosub_index( Environment * _environment, char * _label );
void                    on_gosub_number( Environment * _environment, int _number );
void                    on_goto( Environment * _environment, char * _expression );
void                    on_goto_end( Environment * _environment );
void                    on_goto_index( Environment * _environment, char * _label );
void                    on_goto_number( Environment * _environment, int _number );
void                    on_proc( Environment * _environment, char * _expression );
void                    on_proc_end( Environment * _environment );
void                    on_proc_index( Environment * _environment, char * _label );
void                    on_scroll_call( Environment * _environment, int _x, int _y, char * _label );
void                    on_scroll_gosub( Environment * _environment, int _x, int _y, char * _label );
Variable *              origin_resolution_relative_transform_x( Environment * _environment, char * _x, int _is_relative );
Variable *              origin_resolution_relative_transform_y( Environment * _environment, char * _y, int _is_relative );
void                    out_var( Environment * _environment, char * _port, char * _value );

//----------------------------------------------------------------------------
// *P*
//----------------------------------------------------------------------------

void                    paint_vars( Environment * _environment, char * _x, char * _y, char * _c, char * _b );
int                     palette_extract( Environment * _environment, char * _data, int _width, int _height, int _depth, int _flags, RGBi * _palette );
RGBi *                  palette_match( RGBi * _source, int _source_size, RGBi * _system, int _system_size );
RGBi *                  palette_match_hardware_index( RGBi * _source, int _source_size, RGBi * _system, int _system_size );
RGBi *                  palette_merge( RGBi * _palette1, int _palette1_size, RGBi * _palette2, int _palette2_size, int * _size );
RGBi *                  palette_promote_color_as_background( int _index, RGBi * _source, int _source_size );
RGBi *                  palette_promote_color_as_foreground( int _index, RGBi * _source, int _source_size, int _max_size );
RGBi *                  palette_remove_duplicates( RGBi * _source, int _source_size, int * _unique_size );
RGBi *                  palette_shift( RGBi * _source, int _source_size, int _offset );
void                    paper( Environment * _environment, char * _paper );
Variable *              param_procedure( Environment * _environment, char * _name );
char *                  parse_buffer( Environment * _environment, char * _buffer, int * _size, int _hex_only );
Variable *              parse_buffer_definition( Environment * _environment, char * _buffer, VariableType _type, int _hex_only );
Variable *              peek_var( Environment * _environment, char * _location );
Variable *              peekw_var( Environment * _environment, char * _location );
Variable *              peekd_var( Environment * _environment, char * _location );
void                    pen( Environment * _environment, char * _color );
void                    play( Environment * _environment, int _note, int _duration, int _channels );
void                    play_vars( Environment * _environment, char * _note, char * _duration, char * _channels );
void                    play_off( Environment * _environment, int _channels );
void                    play_off_var( Environment * _environment, char * _channels );
void                    play_string( Environment * _environment, char * _string );
void                    plot( Environment * _environment, char * _x, char * _y, char *_c );
void                    pmode( Environment * _environment, int _mode, int _start_page );
Variable *              point( Environment * _environment, char * _x, char * _y );
void                    point_at( Environment * _environment, int _x, int _y );
void                    point_at_vars( Environment * _environment, char * _x, char * _y );
void                    poke_var( Environment * _environment, char * _address, char * _value );
void                    pokew_var( Environment * _environment, char * _address, char * _value );
void                    poked_var( Environment * _environment, char * _address, char * _value );
void                    pop( Environment * _environment );
Variable *              powering( Environment * _environment, char * _source, char * _dest );
TileDescriptors *       precalculate_tile_descriptors_for_font( char * _fontData, int _fontSize );
void                    prepare_variable_storage( Environment * _environment, char * _name, Variable * _variable );
void                    print( Environment * _environment, char * _text, int _new_line );
void                    print_buffer( Environment * _environment, char * _buffer, int _new_line, int _printable );
void                    print_newline( Environment * _environment );
void                    print_question_mark( Environment * _environment );
void                    print_tab( Environment * _environment, int _new_line );
void                    put_image( Environment * _environment, char * _image, char * _x1, char * _y1, char * _x2, char * _y2, char * _frame, char * _sequence, int _flags );
void                    put_image_vars( Environment * _environment, char * _image, char * _x1, char * _y1, char * _x2, char * _y2, char * _frame, char * _sequence, char * _flags );
void                    put_tile( Environment * _environment, char * _tile, char * _x, char * _y, char * _w, char * _h );
void                    put_tilemap_vars( Environment * _environment, char * _tilemap, int _flags, char * _dx, char * _dy, char * _layer, char * _padding_tile );
void                    put_tilemap_inline( Environment * _environment, char * _tilemap, int _flags, char * _dx, char * _dy, char * _layer, int _padding_tile );

//----------------------------------------------------------------------------
// *Q*
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// *R*
//----------------------------------------------------------------------------

Variable *              random_height( Environment * _environment );
Variable *              random_value( Environment * _environment, VariableType _type );
Variable *              random_width( Environment * _environment );
void                    randomize( Environment * _environment, char * _seed );
void                    raster_at( Environment * _environment, char * _label, int _position );
void                    raster_at_var( Environment * _environment, char * _label, char * _position );
Variable *              read_end( Environment * _environment );
Variable *              read_end_unsafe( Environment * _environment );
void                    read_data( Environment * _environment, char * _variable, int _safe );
void                    read_data_unsafe( Environment * _environment, char * _variable );
void                    remember( Environment * _environment );
void                    repeat( Environment * _environment, char *_label );
char *                  resource_load_asserts( Environment * _environment, char * _filename );
Variable *              respawn_procedure( Environment * _environment, char * _name );
void                    restore_label( Environment * _environment, char * _label );
void                    restore_label_unsafe( Environment * _environment, char * _label );
void                    return_label( Environment * _environment );
void                    return_procedure( Environment * _environment, char * _value );
int                     rgbi_equals_rgb( RGBi * _first, RGBi * _second );
int                     rgbi_equals_rgba( RGBi * _first, RGBi * _second );
int                     rgbi_extract_palette( Environment * _environment, unsigned char* _source, int _width, int _height, int _depth, RGBi _palette[], int _palette_size, int _sorted);
void                    rgbi_move( RGBi * _source, RGBi * _destination );
int                     rgbi_distance( RGBi * _source, RGBi * _destination );
Variable *              rnd( Environment * _environment, char * _value );
void                    run( Environment * _environment );
void                    run_parallel( Environment * _environment );

//----------------------------------------------------------------------------
// *S*
//----------------------------------------------------------------------------

Variable *              scancode( Environment * _environment );
Variable *              scanshift( Environment * _environment );
Variable *              screen_can( Environment * _environment, int _mode );
void                    screen_columns( Environment * _environment, int _columns );
void                    screen_columns_var( Environment * _environment, char * _columns );
Variable *              screen_get_height( Environment * _environment );
Variable *              screen_get_width( Environment * _environment );
void                    screen_horizontal_scroll( Environment * _environment, int _displacement );
void                    screen_horizontal_scroll_var( Environment * _environment, char * _displacement );
void                    screen_mode( Environment * _environment, int _mode );
void                    screen_type_color_set( Environment * _environment, int _type, int _color_set );
void                    screen_off( Environment * _environment );
void                    screen_on( Environment * _environment );
void                    screen_rows( Environment * _environment, int _rows );
void                    screen_rows_var( Environment * _environment, char * _rows );
void                    screen_swap( Environment * _environment );
Variable *              screen_page( Environment * _environment );
Variable *              screen_tiles_get( Environment * _environment );
Variable *              screen_tiles_get_height( Environment * _environment );
Variable *              screen_tiles_get_width( Environment * _environment );
void                    screen_vertical_scroll( Environment * _environment, int _displacement );
void                    screen_vertical_scroll_var( Environment * _environment, char * _displacement );
void                    scroll( Environment * _environment, int _dx, int _dy );
void                    select_case( Environment * _environment, char * _expression );
Variable *              sequence_load( Environment * _environment, char * _filename, char * _alias, int _mode, int _frame_width, int _frame_height, int _flags, int _transparent_color, int _background_color, int _bank_expansion );
Variable *              sequence_storage( Environment * _environment, char * _filename, char * _alias, int _mode, int _frame_width, int _frame_height, int _flags, int _transparent_color, int _background_color, int _bank_expansion );
void                    set_timer( Environment * _environment, char * _value );
void                    shared( Environment * _environment );
void                    shoot( Environment * _environment, int _channels );
void                    slice_image( Environment * _environment, char * _image, char * _frame, char * _sequence, char * _destination );
void                    sound( Environment * _environment, int _freq, int _duration, int _channels );
void                    sound_vars( Environment * _environment, char * _freq, char * _duration, char * _channels );
void                    sound_off( Environment * _environment, int _channels );
void                    sound_off_var( Environment * _environment, char * _channels );
Variable *              sign( Environment * _environment, char * _value );
Variable *              spawn_procedure( Environment * _environment, char * _name , int _halted );
void                    spc( Environment * _environment, char * _spaces );
void                    sprite_color( Environment * _environment, int _sprite, int _color );
void                    sprite_color_vars( Environment * _environment, char * _sprite, char * _color );
void                    sprite_compress_horizontal( Environment * _environment, int _sprite );
void                    sprite_compress_horizontal_var( Environment * _environment, char * _sprite );
void                    sprite_compress_vertical( Environment * _environment, int _sprite );
void                    sprite_compress_vertical_var( Environment * _environment, char * _sprite );
Variable *              sprite_converter( Environment * _environment, char * _data, int _width, int _height, int _depth, RGBi * _colorm, int _flags );
void                    sprite_data_from( Environment * _environment, int _sprite, int _address );
void                    sprite_data_from_vars( Environment * _environment, char * _sprite, char * _address );
void                    sprite_disable( Environment * _environment, int _sprite );
void                    sprite_disable_var( Environment * _environment, char * _sprite );
void                    sprite_enable( Environment * _environment, int _sprite );
void                    sprite_enable_var( Environment * _environment, char * _sprite );
void                    sprite_expand_horizontal( Environment * _environment, int _sprite );
void                    sprite_expand_horizontal_var( Environment * _environment, char * _sprite );
void                    sprite_expand_vertical( Environment * _environment, int _sprite );
void                    sprite_expand_vertical_var( Environment * _environment, char * _sprite );
Variable *              sprite_init( Environment * _environment, char * _image, char * _sprite, int _flags );
void                    sprite_monocolor( Environment * _environment, int _sprite );
void                    sprite_monocolor_var( Environment * _environment, char * _sprite );
void                    sprite_multicolor( Environment * _environment, int _sprite );
void                    sprite_multicolor_var( Environment * _environment, char * _sprite );
void                    sprite_at( Environment * _environment, int _sprite, int _x, int _y );
void                    sprite_at_vars( Environment * _environment, char * _sprite, char * _x, char * _y );
Variable *              sqroot( Environment * _environment, char * _value );
StaticString *          string_reserve( Environment * _environment, char * _value );
Variable *              strptr( Environment * _environment, char * _name );
void                    sys( Environment * _environment, int _address );
void                    sys_var( Environment * _environment, char * _address );
void                    sys_call( Environment * _environment, int _address );
int                     system_call( Environment * _environment, char * _command );
int                     system_remove_safe( Environment * _environment, char * _filename );

//----------------------------------------------------------------------------
// *T*
//----------------------------------------------------------------------------


void                    text_at( Environment * _environment, char * _x, char * _y, char * _text );
void                    text_encoded( Environment * _environment, char * _text, char * _pen, char * _paper );
Variable *              text_get_xcurs( Environment * _environment );
Variable *              text_get_ycurs( Environment * _environment );
void                    text_hscroll_line( Environment * _environment, int _direction );
void                    text_hscroll_screen( Environment * _environment, int _direction );
void                    text_newline( Environment * _environment );
void                    text_question_mark( Environment * _environment );
void                    text_set_tab( Environment * _environment, char * _net_tab );
void                    text_shade( Environment * _environment, int _value );
void                    text_tab( Environment * _environment );
void                    text_text( Environment * _environment, char * _text );
void                    text_under( Environment * _environment, int _value );
void                    text_vscroll( Environment * _environment );
void                    text_vscroll_screen( Environment * _environment, int _direction );
void                    textmap_at( Environment * _environment, int _address );
void                    textmap_at_var( Environment * _environment, char * _address );
Variable *              tilemap_at( Environment * _environment, char * _tilemap, char * _x, char * _y, char * _layer );
void                    tilemap_disable( Environment * _environment );
void                    tilemap_enable( Environment * _environment, int _width, int _height, int _colors, int _tile_width, int _tile_height );
Variable *              tile_at( Environment * _environment, char * _x, char * _y );
int                     tile_allocate( TileDescriptors * _tiles, char * _data );
Variable *              tile_belong( Environment * _environment, char * _tile, char * _tiles );
Variable *              tile_class( Environment * _environment, char * _tileset, int _id );
Variable *              tile_get_first( Environment * _environment, char * _tile );
Variable *              tile_get_height( Environment * _environment, char * _tile );
Variable *              tile_get_width( Environment * _environment, char * _tile );
int                     tile_id( Environment * _environment, char * _tileset, char * _id );
Variable *              tile_probability( Environment * _environment, char * _tileset, int _id );
Variable *              tilemap_get_height( Environment * _environment, char * _tilemap );
Variable *              tilemap_get_width( Environment * _environment, char * _tilemap );
Variable *              tileset_tile_get_height( Environment * _environment, char * _tileset );
Variable *              tileset_tile_get_width( Environment * _environment, char * _tileset );
Variable *              tile_load( Environment * _environment, char * _filename, int _flags, char * _tileset, int _index );
Variable *              tiles_load( Environment * _environment, char * _filename, int _flags, char * _tileset, int _index );
void                    tiles_at( Environment * _environment, int _address );
void                    tiles_at_var( Environment * _environment, char * _address );
Variable *              tilemap_index_vars( Environment * _environment, char * _tilemap, char * _column, char * _row, char * _layer );
Variable *              tilemap_load( Environment * _environment, char * _filename, char * _alias, int _mode, int _flags, int _transparent_color, int _background_color, int _bank_expansion );
Variable *              tileset_load( Environment * _environment, char * _filename, char * _alias, int _mode, int _flags, int _transparent_color, int _background_color, int _bank_expansion );
Variable *              tilemap_storage( Environment * _environment, char * _filename, char * _alias, int _mode, int _flags, int _transparent_color, int _background_color, int _bank_expansion );
Variable *              tileset_storage( Environment * _environment, char * _source_name, char * _target_name, int _mode, int _flags, int _transparent_color, int _background_color, int _bank_expansion );
Variable *              tileset_of_vars( Environment * _environment, char * _tilemap );

//----------------------------------------------------------------------------
// *u*
//----------------------------------------------------------------------------

void                    use_tileset( Environment * _environment, char * _tileset );
char *                  unescape_string( Environment * _environment, char * _value, int _printing, int * _final_size );
Variable *              uncompress( Environment * _environment, char * _value );

//----------------------------------------------------------------------------
// *V*
//----------------------------------------------------------------------------

Variable *              variable_add( Environment * _environment, char * _source, char * _dest );
Variable *              variable_add_const( Environment * _environment, char * _source, int _dest );
void                    variable_add_inplace( Environment * _environment, char * _source, int _dest );
void                    variable_add_inplace_vars( Environment * _environment, char * _source, char * _dest );
void                    variable_add_inplace_array( Environment * _environment, char * _source, char * _destination );
void                    variable_add_inplace_mt( Environment * _environment, char * _source, char * _destination );
Variable *              variable_and( Environment * _environment, char * _left, char * _right );
Variable *              variable_and_const( Environment * _environment, char * _source, int _mask );
void                    variable_array_fill( Environment * _environment, char * _name, int _value );
Variable *              variable_array_type( Environment * _environment, char *_name, VariableType _type );
Variable *              variable_bin( Environment * _environment, char * _value, char * _digits );
Variable *              variable_bit( Environment * _environment, char * _value, char * _position );
Variable *              variable_cast( Environment * _environment, char * _source, VariableType _type );
void                    variable_cleanup( Environment * _Environment );
Variable *              variable_compare( Environment * _environment, char * _source, char * _dest );
void                    variable_compare_and_branch_const( Environment * _environment, char *_source, int _destination,  char *_name, int _positive );
Variable *              variable_compare_const( Environment * _environment, char * _source, int _dest );
Variable *              variable_compare_not( Environment * _environment, char * _source, char * _dest );
Variable *              variable_compare_not_const( Environment * _environment, char * _source, int _dest );
Variable *              variable_complement_const( Environment * _environment, char * _source, int _mask );
Variable *              variable_decrement( Environment * _environment, char * _source );
Variable *              variable_decrement_array( Environment * _environment, char * _source );
Variable *              variable_decrement_mt( Environment * _environment, char * _source );
Variable *              variable_define( Environment * _environment, char * _name, VariableType _type, int _value );
Variable *              variable_define_no_init( Environment * _environment, char * _name, VariableType _type );
int                     variable_delete( Environment * _environment, char * _name );
Variable *              variable_direct_assign( Environment * _environment, char * _var, char * _expr );
Variable *              variable_div( Environment * _environment, char * _source, char * _dest, char * _remainder );
Variable *              variable_div2_const( Environment * _environment, char * _source, int _bits );
void                    variable_global( Environment * _environment, char * _pattern );
Variable *              variable_greater_than( Environment * _environment, char * _source, char * _dest, int _equal );
Variable *              variable_greater_than_const( Environment * _environment, char * _source, int _dest, int _equal );
Variable *              variable_hex( Environment * _environment, char * _value );
Variable *              variable_import( Environment * _environment, char * _name, VariableType _type, int _size_or_value );
Variable *              variable_increment( Environment * _environment, char * _source );
Variable *              variable_increment_array( Environment * _environment, char * _source );
Variable *              variable_increment_mt( Environment * _environment, char * _source );
Variable *              variable_int( Environment * _environment, char * _expression );
Variable *              variable_less_than( Environment * _environment, char * _source, char * _dest, int _equal );
Variable *              variable_less_than_const( Environment * _environment, char * _source, int _dest, int _equal );
Variable *              variable_mod( Environment * _environment, char * _source, char * _destination );
Variable *              variable_move( Environment * _environment, char * _source, char * _dest );
void                    variable_move_array( Environment * _environment, char * _array, char * _value  );
void                    variable_move_array_string( Environment * _environment, char * _array, char * _string  );
Variable *              variable_move_from_array( Environment * _environment, char * _array );
Variable *              variable_move_from_mt( Environment * _environment, char * _source, char * _destination );
Variable *              variable_move_to_mt( Environment * _environment, char * _source, char * _destination );
Variable *              variable_move_naked( Environment * _environment, char * _source, char * _dest );
Variable *              variable_mul( Environment * _environment, char * _source, char * _dest );
Variable *              variable_mul2_const( Environment * _environment, char * _source, int _bits );
Variable *              variable_not( Environment * _environment, char * _value );
void                    variable_on_memory_init( Environment * _environment, int _imported_too );
Variable *              variable_or( Environment * _environment, char * _left, char * _right );
Variable *              variable_or( Environment * _environment, char * _source, char * _dest );
void                    variable_temporary_remove( Environment * _environment, char * _name );
void                    variable_reset( Environment * _environment );
Variable *              variable_resize_buffer( Environment * _environment, char * _destination, int _size );
int                     variable_exists( Environment * _environment, char * _name );
Variable *              variable_retrieve( Environment * _environment, char * _name );
Variable *              variable_retrieve_by_realname( Environment * _environment, char * _name );
Variable *              variable_retrieve_or_define( Environment * _environment, char * _name, VariableType _type, int _value );
void                    variable_set( Environment * _environment );
Variable *              variable_store( Environment * _environment, char * _source, unsigned int _value );
void                    variable_store_mt( Environment * _environment, char * _source, unsigned int _value );
Variable *              variable_store_array( Environment * _environment, char * _destination, unsigned char * _buffer, int _size, int _at );
void                    variable_store_array_const( Environment * _environment, char * _array, int _value  );
Variable *              variable_store_buffer( Environment * _environment, char * _destination, unsigned char * _buffer, int _size, int _at );
Variable *              variable_store_string( Environment * _environment, char * _source, char * _string );
Variable *              variable_store_float( Environment * _environment, char * _destination, double _value );
Variable *              variable_string_asc( Environment * _environment, char * _char );
Variable *              variable_string_chr( Environment * _environment, char * _ascii  );
Variable *              variable_string_flip( Environment * _environment, char * _string  );
Variable *              variable_string_instr( Environment * _environment, char * _search, char * _searched, char * _start );
Variable *              variable_string_left( Environment * _environment, char * _string, char * _position );
void                    variable_string_left_assign( Environment * _environment, char * _string, char * _position, char * _expression );
Variable *              variable_string_len( Environment * _environment, char * _string );
Variable *              variable_string_lower( Environment * _environment, char * _string );
Variable *              variable_string_mid( Environment * _environment, char * _string, char * _position, char * _len );
void                    variable_string_mid_assign( Environment * _environment, char * _string, char * _position, char * _len, char * _expression );
Variable *              variable_string_right( Environment * _environment, char * _string, char * _position );
void                    variable_string_right_assign( Environment * _environment, char * _string, char * _position, char * _expression );
Variable *              variable_string_space( Environment * _environment, char * _repetitions  );
Variable *              variable_string_str( Environment * _environment, char * _value );
Variable *              variable_string_string( Environment * _environment, char * _string, char * _repetitions  );
Variable *              variable_string_upper( Environment * _environment, char * _string );
Variable *              variable_string_val( Environment * _environment, char * _value );
Variable *              variable_sub( Environment * _environment, char * _source, char * _dest );
Variable *              variable_sub_const( Environment * _environment, char * _source, int _dest );
void                    variable_sub_inplace( Environment * _environment, char * _source, char * _dest );
void                    variable_swap( Environment * _environment, char * _source, char * _dest );
Variable *              variable_temporary( Environment * _environment, VariableType _type, char * _meaning );
VariableType            variable_type_from_numeric_value( Environment * _environment, int _number );
Variable *              variable_resident( Environment * _environment, VariableType _type, char * _meaning );
Variable *              variable_xor( Environment * _environment, char * _left, char * _right );
Variable *              varptr( Environment * _environment, char * _identifier );
void                    volume( Environment * _environment, int _volume, int _channels );
void                    volume_vars( Environment * _environment, char * _volume, char * _channels );
void                    volume_off( Environment * _environment, int _channels );
void                    volume_off_var( Environment * _environment, char * _channels );

//----------------------------------------------------------------------------
// *W*
//----------------------------------------------------------------------------

void                    wait_cycles( Environment * _environment, int _timing, int _parallel );
void                    wait_cycles_var( Environment * _environment, char * _timing, int _parallel );
void                    wait_key( Environment * _environment );
void                    wait_milliseconds( Environment * _environment, int _timing );
void                    wait_milliseconds_var( Environment * _environment, char * _timing );
void                    wait_ticks( Environment * _environment, int _timing );
void                    wait_ticks_var( Environment * _environment, char * _timing );
void                    wait_vbl( Environment * _environment, char * _raster_line );
void                    wait_until( Environment * _environment );
void                    wait_until_condition( Environment * _environment, char * _condition );
void                    wait_while( Environment * _environment );
void                    wait_while_condition( Environment * _environment, char * _condition );
void                    wait_parallel( Environment * _environment, char * _thread );
void                    writing( Environment * _environment, char * _mode, char * _parts );

//----------------------------------------------------------------------------
// *X*
//----------------------------------------------------------------------------

Variable *              xpen( Environment * _environment );
Variable *              x_graphic_get( Environment * _environment, char * _x );
Variable *              x_text_get( Environment * _environment, char * _x );

//----------------------------------------------------------------------------
// *Y*
//----------------------------------------------------------------------------

void                    yield( Environment * _environment );
Variable *              ypen( Environment * _environment );
Variable *              y_graphic_get( Environment * _environment, char * _y );
Variable *              y_text_get( Environment * _environment, char * _y );

#if defined(__atari__) 
    #include "../src-generated/modules_atari.h"
    #include "hw/6502.h"
    #include "hw/antic.h"
    #include "hw/gtia.h"
    #include "hw/pokey.h"
    #include "hw/atari.h"
#elif defined(__atarixl__) 
    #include "../src-generated/modules_atarixl.h"
    #include "hw/6502.h"
    #include "hw/antic.h"
    #include "hw/gtia.h"
    #include "hw/pokey.h"
    #include "hw/atari.h"
#elif __c64__
    #include "../src-generated/modules_c64.h"
    #include "hw/6502.h"
    #include "hw/vic2.h"
    #include "hw/sid.h"
    #include "hw/c64.h"
    #include "outputs/d64.h"
#elif __plus4__
    #include "../src-generated/modules_plus4.h"
    #include "hw/6502.h"
    #include "hw/ted.h"
    #include "hw/plus4.h"
#elif __zx__
    #include "../src-generated/modules_zx.h"
    #include "hw/z80.h"
    #include "hw/zx.h"
#elif __coco__ 
    #include "../src-generated/modules_coco.h"
    #include "hw/6809.h"
    #include "hw/6847.h"
    #include "hw/coco.h"
#elif __coco3__ 
    #include "../src-generated/modules_coco3.h"
    #include "hw/6809.h"
    #include "hw/gime.h"
    #include "hw/coco3.h"
#elif __d32__ 
    #include "../src-generated/modules_d32.h"
    #include "hw/6809.h"
    #include "hw/6847.h"
    #include "hw/d32.h"
#elif __d64__ 
    #include "../src-generated/modules_d64.h"
    #include "hw/6809.h"
    #include "hw/6847.h"
    #include "hw/d64.h"
#elif __pc128op__ 
    #include "../src-generated/modules_pc128op.h"
    #include "hw/6809.h"
    #include "hw/ef936x.h"
    #include "hw/pc128op.h"
#elif __mo5__ 
    #include "../src-generated/modules_mo5.h"
    #include "hw/6809.h"
    #include "hw/ef936x.h"
    #include "hw/mo5.h"
#elif __vic20__
    #include "../src-generated/modules_vic20.h"
    #include "hw/6502.h"
    #include "hw/vic1.h"
    #include "hw/vic20.h"
    #include "outputs/d64.h"
#elif __msx1__
    #include "../src-generated/modules_msx1.h"
    #include "hw/z80.h"
    #include "hw/msx1.h"
    #include "hw/tms9918.h"
    #include "hw/ay8910.h"
#elif __coleco__
    #include "../src-generated/modules_coleco.h"
    #include "hw/z80.h"
    #include "hw/coleco.h"
    #include "hw/tms9918.h"
    #include "hw/sn76489.h"
#elif __sc3000__
    #include "../src-generated/modules_sc3000.h"
    #include "hw/z80.h"
    #include "hw/sc3000.h"
    #include "hw/tms9918.h"
    #include "hw/sn76489.h"
#elif __sg1000__
    #include "../src-generated/modules_sg1000.h"
    #include "hw/z80.h"
    #include "hw/sg1000.h"
    #include "hw/tms9918.h"
    #include "hw/sn76489.h"
#elif __cpc__
    #include "../src-generated/modules_cpc.h"
    #include "hw/z80.h"
    #include "hw/cpc.h"
    #include "hw/ay8910.h"
#elif __c128__
    #include "../src-generated/modules_c128.h"
    #include "hw/6502.h"
    #include "hw/vic2.h"
    #include "hw/sid.h"
    #include "hw/c128.h"
    #include "outputs/d64.h"
#elif __c128z__
    #include "../src-generated/modules_c128z.h"
    #include "hw/z80.h"
    #include "hw/vdcz.h"
    #include "hw/sidz.h"
    #include "hw/c128z.h"
#elif __vg5000__
    #include "../src-generated/modules_vg5000.h"
    #include "hw/z80.h"
    #include "hw/vg5000.h"
    #include "hw/ef9345.h"
#endif

#ifdef CPU_BIG_ENDIAN
    #define IMAGE_GET_WIDTH( buffer, offset, width ) \
        if ( IMAGE_WIDTH_SIZE == 1 ) { \
            width = buffer[offset+IMAGE_WIDTH_OFFSET]; \
        } else { \
            width = 256*buffer[offset+IMAGE_WIDTH_OFFSET] + buffer[offset+IMAGE_WIDTH_OFFSET+1]; \
        }
    #define IMAGE_GET_HEIGHT( buffer, offset, height ) \
        if ( IMAGE_HEIGHT_SIZE == 1 ) { \
            height = buffer[offset+IMAGE_HEIGHT_OFFSET]; \
        } else { \
            height = 256*buffer[offset+IMAGE_HEIGHT_OFFSET] + buffer[offset+IMAGE_HEIGHT_OFFSET+1]; \
        }
#else
    #define IMAGE_GET_WIDTH( buffer, offset, width ) \
        if ( IMAGE_WIDTH_SIZE == 1 ) { \
            width = buffer[offset+IMAGE_WIDTH_OFFSET]; \
        } else { \
            width = buffer[offset+IMAGE_WIDTH_OFFSET] + 256 * buffer[offset+IMAGE_WIDTH_OFFSET+1]; \
        }
    #define IMAGE_GET_HEIGHT( buffer, offset, height ) \
        if ( IMAGE_HEIGHT_SIZE == 1 ) { \
            height = buffer[offset+IMAGE_HEIGHT_OFFSET]; \
        } else { \
            height = buffer[offset+IMAGE_HEIGHT_OFFSET] + 256 * buffer[offset+IMAGE_HEIGHT_OFFSET+1]; \
        }
#endif

#endif