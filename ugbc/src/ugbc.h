#ifndef __UGBASICCOMPILER__
#define __UGBASICCOMPILER__

/*****************************************************************************
 * ugBASIC - an isomorphic BASIC language compiler for retrocomputers        *
 *****************************************************************************
 * Copyright 2021 Marco Spedaletti (asimov@mclink.it)
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

#include "../src/stb_image.h"

/****************************************************************************
 * DECLARATIONS AND DEFINITIONS SECTION 
 ****************************************************************************/

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

    /** Name of the bank */
    char * name;

    /** Starting address */
    int address;

    /** Bank type */
    BankType type;

    /** (optional) file name that will be loaded into the bank */
    char *filename;

    /** Link to the next bank (NULL if this is the last one) */
    struct _Bank * next;

} Bank;

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
    VT_DSTRING = 13
    
} VariableType;

#define MAX_ARRAY_DIMENSIONS            256
#define MAX_PARAMETERS                  256

#define VT_BW_8BIT( t, v )              ( ( (t) == (v) ) ? 8 : 0 )
#define VT_BW_16BIT( t, v )             ( ( (t) == (v) ) ? 16 : 0 )
#define VT_BW_32BIT( t, v )             ( ( (t) == (v) ) ? 32 : 0 )

#define VT_BITWIDTH( t ) \
        ( VT_BW_8BIT( t, VT_BYTE ) + VT_BW_8BIT( t, VT_SBYTE ) + VT_BW_8BIT( t, VT_COLOR ) + \
        VT_BW_16BIT( t, VT_WORD ) + VT_BW_16BIT( t, VT_SWORD ) + VT_BW_16BIT( t, VT_ADDRESS ) + VT_BW_16BIT( t, VT_POSITION ) + \
        VT_BW_32BIT( t, VT_DWORD ) + VT_BW_32BIT( t, VT_SDWORD ) )

#define VT_SIGNED( t ) \
        ( ( (t) == VT_SBYTE ) || ( (t) == VT_SWORD ) || ( (t) == VT_SDWORD ) )

#define VT_SIGN_8BIT( v ) ( v < 0 ? ( ((~(unsigned char)(abs(v)))+1 ) ) : (v) )
#define VT_SIGN_16BIT( v ) ( v < 0 ? ( ((~(unsigned short)(abs(v)))+1 ) ) : (v) )
#define VT_SIGN_32BIT( v ) ( v < 0 ? ( (~((unsigned int) (abs(v)))+1 ) ) : (v) )

#define VT_ESIGN_8BIT( t, v ) ( VT_SIGNED(t) ? VT_SIGN_8BIT(v) : (v) )
#define VT_ESIGN_16BIT( t, v ) ( VT_SIGNED(t) ? VT_SIGN_16BIT(v) : (v) ) 
#define VT_ESIGN_32BIT( t, v ) ( VT_SIGNED(t) ? VT_SIGN_32BIT(v) : (v) ) 

/**
 * @brief Maximum number of variable types
 */
#define VARIABLE_TYPE_COUNT   8

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
     * The initial value of the variable, as given by last (re)definition.
     */
    int value;

    /** 
     * The static string's valu, as given by last (re)definition.
     */
    char * valueString;

    /** 
     * The size of the static buffer (in bytes).
     */
    int size;

    /** 
     * Pointer to the bank where this variable belongs to.
     */
    Bank * bank;

    /**
     * Number of dimensions of this array
     */
    int arrayDimensions;

    /**
     * Size of each dimension
     */
    int arrayDimensionsEach[MAX_ARRAY_DIMENSIONS];

    /** Variable type */
    VariableType arrayType;

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
    VariableType parametersTypeEach[MAX_PARAMETERS];

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
    CT_ON_PROC = 3

} ConditionalType;

/**
 * @brief Maximum number of conditional types
 */
#define CONDITIONAL_TYPE_COUNT   1

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
    LT_WHILE = 0,

    /** REPEAT ... UNTIL */
    LT_REPEAT = 0,

    /** FOR ... NEXT */
    LT_FOR = 0,

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

    /** Variable with to. */
    Variable *to;

    /** Variable with step. */
    Variable *step;

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

typedef struct _ScreenMode {

    int         id;

    char *      description;

    int         bitmap;

    int         width;

    int         height;

    int         colors;

    int         score;

    struct _ScreenMode  * next;

} ScreenMode;

#define SCREEN_MODE_DEFINE( _id, _bitmap, _width, _height, _colors, _description ) \
    { \
        ScreenMode * screenMode = malloc( sizeof( ScreenMode ) ); \
        memset( screenMode, 0, sizeof( screenMode ) ) ; \
        screenMode->bitmap = _bitmap; \
        screenMode->id = _id; \
        screenMode->width = _width; \
        screenMode->height = _height; \
        screenMode->colors = _colors; \
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
     * Filename of linker's configuration file (*.cfg) 
     */
    char * configurationFileName;

    /**
     * Enable the visualization of warnings during compilation.
     */

    int warningsEnabled;

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
     * Set of banks defined during compilation. 
     * It contains all the banks, divided by type.
     */
    Bank * banks[BANK_TYPE_COUNT];

    ScreenMode * screenModes;

    /**
     * List of temporary (and reused) variables.
     */
    Variable * tempVariables;

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
     * This flag marks if the program has opened a "game loop".
     */
    int hasGameLoop;

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

    /**
     * "Every" status
     */
    Variable * everyStatus;

    /**
     * "Every" counter
     */
    Variable * everyCounter;

    /**
     * "Every" timing
     */
    Variable * everyTiming;

    /**
     * Temporary storage for array definition
     */
    int arrayDimensions;

    /**
     * Temporary storage for array definition
     */
    int arrayDimensionsEach[MAX_ARRAY_DIMENSIONS];

    /**
     * Temporary storage for array access
     */
    int arrayIndexes;

    /**
     * Temporary storage for array access
     */
    char * arrayIndexesEach[MAX_ARRAY_DIMENSIONS];

    /**
     * Current procedure
     */
    char * procedureName;

    /**
     * Temporary storage for parameters definition
     */
    int parameters;

    /**
     * Temporary storage for parameters definition
     */
    char * parametersEach[MAX_PARAMETERS];

    /**
     * Temporary storage for parameters definition
     */
    VariableType parametersTypeEach[MAX_PARAMETERS];

    /**
     * Deployed the vars
     */

    int varsDeployed;

    /**
     * Deployed the startup for VIC-II
     */

    int vicstartupDeployed;

    /**
     * Deployed the vars for VIC-II
     */

    int vic2varsDeployed;

    /**
     * Deployed plotting routine
     */

    int plotDeployed;

    /**
     * Deployed the dynamic string support
     */

    int dstringDeployed;

    /**
     * Deployed the scancode routine
     */

    int scancodeDeployed;

    /**
     * Deployed the text_encoded_at routine
     */

    int textEncodedAtDeployed;

    /**
     * Deployed the number to string routine
     */

    int numberToStringDeployed;

    /**
     * Deployed the bits to string routine
     */

    int bitsToStringDeployed;

    /**
     * Deployed the vertical scroll routine
     */

    int vScrollDeployed;

    /**
     * Deployed the vertical scroll text routine
     */

    int vScrollTextDeployed;

    /**
     * Deployed the vertical scroll text routine
     */

    int textVScrollScreenDeployed;

    /**
     * Deployed the cls text routine
     */

    int clsDeployed;

    /**
     * Deployed the cline text routine
     */

    int textClineDeployed;

    /**
     *  Deployed the code for horizontal line scrolling
     */
    int textHScrollLineDeployed;

    /**
     *  Deployed the code for horizontal screen scrolling
     */
    int textHScrollScreenDeployed;

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

} Environment;

#define UNIQUE_ID   _environment->uniqueId++
#define MAKE_LABEL  char label[12]; sprintf( label, "_label%d", UNIQUE_ID);
#define CRITICAL( s ) fprintf(stderr, "CRITICAL ERROR during compilation of %s:\n\t%s at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, ((struct _Environment *)_environment)->yylineno ); exit( EXIT_FAILURE );
#define CRITICAL2( s, v ) fprintf(stderr, "CRITICAL ERROR during compilation of %s:\n\t%s (%s) at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, v, ((struct _Environment *)_environment)->yylineno ); exit( EXIT_FAILURE );
#define CRITICAL3( s, v1, v2 ) fprintf(stderr, "CRITICAL ERROR during compilation of %s:\n\t%s (%s, %s) at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, v1, v2, ((struct _Environment *)_environment)->yylineno ); exit( EXIT_FAILURE );
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
#define CRITICAL_ARRAY_SIZE_MISMATCH( v ) CRITICAL2("E035 - number of indexes different from array dimensions", v );
#define CRITICAL_NOT_ARRAY( v ) CRITICAL2("E036 - accessing with indexes on a non array variable", v );
#define CRITICAL_PROCEDURE_NESTED_UNSUPPORTED( n ) CRITICAL2("E037 - cannot define a nested procedure (a procedure inside a procedure)", n ); 
#define CRITICAL_PROCEDURE_NOT_OPENED() CRITICAL("E038 - END PROC outside a procedure" ); 
#define CRITICAL_PROCEDURE_MISSING( n ) CRITICAL2("E039 - call to an undefined procedure", n ); 
#define CRITICAL_PROCEDURE_PARAMETERS_MISMATCH( n ) CRITICAL2("E040 - wrong number of parameters on procedure call", n ); 
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
#define WARNING( s ) if ( ((struct _Environment *)_environment)->warningsEnabled) { fprintf(stderr, "WARNING during compilation of %s:\n\t%s at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, ((struct _Environment *)_environment)->yylineno ); }
#define WARNING2( s, v ) if ( ((struct _Environment *)_environment)->warningsEnabled) { fprintf(stderr, "WARNING during compilation of %s:\n\t%s (%s) at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, v, _environment->yylineno ); }
#define WARNING3( s, v1, v2 ) if ( ((struct _Environment *)_environment)->warningsEnabled) { fprintf(stderr, "WARNING during compilation of %s:\n\t%s (%s, %s) at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, v1, v2, _environment->yylineno ); }
#define WARNING_BITWIDTH( v1, v2 ) WARNING3("W001 - Multiplication could loose precision", v1, v2 );
#define WARNING_DOWNCAST( v1, v2 ) WARNING3("W002 - Implicit downcasting to less bitwidth (precision loss)", v1, v2 );

#define outline0n(n,s,r)     \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->asmFile); \
        fputs(s,((Environment *)_environment)->asmFile); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->asmFile); \
    }

#define outline1n(n,s,a,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->asmFile); \
        fprintf(((Environment *)_environment)->asmFile, s, a); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->asmFile); \
    }

#define outline2n(n,s,a,b,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->asmFile); \
        fprintf(((Environment *)_environment)->asmFile, s, a, b); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->asmFile); \
    }

#define outline3n(n,s,a,b,c,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->asmFile); \
        fprintf(((Environment *)_environment)->asmFile, s, a, b, c); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->asmFile); \
    }

#define outline4n(n,s,a,b,c,d,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->asmFile); \
        fprintf(((Environment *)_environment)->asmFile, s, a, b, c, d); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->asmFile); \
    }

#define outline5n(n,s,a,b,c,d,e,r)   \
    { \
        int outsi; \
        for(outsi=0; outsi<n; ++outsi) \
            fputs("\t", ((Environment *)_environment)->asmFile); \
        fprintf(((Environment *)_environment)->asmFile, s, a, b, c, d, e); \
        if ( r ) \
            fputs("\n", ((Environment *)_environment)->asmFile); \
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
                    fputs( line, ((Environment *)_environment)->asmFile); \
                } \
            } \
            fclose( fh ); \
            fputs("\n", ((Environment *)_environment)->asmFile); \
        } else { \
            CRITICAL2("Unable to include ugbasic system file", f ); \
        } \
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

#define deploy(s,f)  \
        if ( ! _environment->s ) { \
            outline1("JMP %s_after", #s); \
            outfile0(f); \
            outhead1("%s_after:", #s); \
            _environment->s = 1; \
        }

#define MAX_TEMPORARY_STORAGE   1024

#define WW_PEN              1
#define WW_PAPER            2

#define JOY_UP              0
#define JOY_DOWN            1
#define JOY_LEFT            2
#define JOY_RIGHT           3
#define JOY_FIRE            4

#define SHIFT_LEFT          1
#define SHIFT_RIGHT         2
#define SHIFT_CAPSLOCK      4
#define SHIFT_CONTROL       8
#define SHIFT_LEFT_ALT      16
#define SHIFT_RIGHT_ALT     32

#define TILEMAP_NATIVE      0
#define BITMAP_NATIVE       1

void begin_compilation( Environment * _environment );
void target_initialization( Environment *_environment );
void end_compilation( Environment * _environment );
void bank_cleanup( Environment * _environment );
void gameloop_cleanup( Environment * _environment );
void linker_cleanup( Environment * _environment );
void linker_setup( Environment * _environment );
int pattern_match( char * _pattern, char * _value );
void setup_text_variables( Environment * _environment );
ScreenMode * find_screen_mode_by_suggestion( Environment * _environment, int _bitmap, int _width, int _height, int _colors );

//----------------------------------------------------------------------------
// *A*
//----------------------------------------------------------------------------

Variable *              absolute( Environment * _environment, char * _value );
void                    add_complex( Environment * _environment, char * _variable, char * _expression, char * _limit_lower, char * _limit_upper );

//----------------------------------------------------------------------------
// *B*
//----------------------------------------------------------------------------

Bank *                  bank_define( Environment * _environment, char * _name, BankType _type, int _address, char * _filename );
void                    begin_for( Environment * _environment, char * _index, char * _from, char * _to );  
void                    begin_for_step( Environment * _environment, char * _index, char * _from, char * _to, char * _step );  
void                    begin_gameloop( Environment * _environment );
void                    begin_loop( Environment * _environment );
void                    begin_procedure( Environment * _environment, char * _name );
void                    begin_repeat( Environment * _environment );
void                    begin_while( Environment * _environment );
void                    begin_while_condition( Environment * _environment, char * _expression );
void                    bitmap_at( Environment * _environment, int _address );
void                    bitmap_at_var( Environment * _environment, char * _address );
void                    bitmap_clear( Environment * _environment );
void                    bitmap_clear_with( Environment * _environment, int _value );
void                    bitmap_clear_with_vars( Environment * _environment, char * _value );
void                    bitmap_disable( Environment * _environment );
void                    bitmap_enable( Environment * _environment, int _width, int _height, int _colors );

//----------------------------------------------------------------------------
// *C*
//----------------------------------------------------------------------------

void                    call_procedure( Environment * _environment, char * _name );
Variable *              clear_key( Environment * _environment );
void                    cls( Environment * _environment, char * _paper );
Variable *              collision_to( Environment * _environment, int _sprite );
Variable *              collision_to_vars( Environment * _environment, char * _sprite );
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

//----------------------------------------------------------------------------
// *D*
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// *E*
//----------------------------------------------------------------------------

void                    else_if_then( Environment * _environment, char * _expression );
void                    end( Environment * _environment );
void                    end_for( Environment * _environment );
void                    end_gameloop( Environment * _environment );
void                    end_if_then( Environment * _environment  );
void                    end_loop( Environment * _environment );
void                    end_procedure( Environment * _environment, char * _value );
void                    end_repeat( Environment * _environment, char * _expression );
void                    end_while( Environment * _environment );
void                    every_off( Environment * _environment );
void                    every_on( Environment * _environment );
void                    every_ticks_call( Environment * _environment, char * _timing, char * _label );
void                    every_ticks_gosub( Environment * _environment, char * _timing, char * _label );
void                    exit_loop( Environment * _environment, int _number );
void                    exit_loop_if( Environment * _environment, char * _expression, int _number );
void                    exit_procedure( Environment * _environment );

//----------------------------------------------------------------------------
// *F*
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// *G*
//----------------------------------------------------------------------------

Variable *              get_timer( Environment * _environment );
void                    global( Environment * _environment );
void                    gosub_label( Environment * _environment, char * _label );
void                    gosub_number( Environment * _environment, int _number );
void                    goto_label( Environment * _environment, char * _label );
void                    goto_number( Environment * _environment, int _number );
void                    graphic( Environment * _environment );

//----------------------------------------------------------------------------
// *h*
//----------------------------------------------------------------------------

void                    halt( Environment * _environment );
Variable *              hit_to( Environment * _environment, int _sprite );
Variable *              hit_to_vars( Environment * _environment, char * _sprite );

//----------------------------------------------------------------------------
// *I*
//----------------------------------------------------------------------------

void                    if_then( Environment * _environment, char * _expression );
void                    ink( Environment * _environment, char * _expression );
Variable *              inkey( Environment * _environment );
void                    input( Environment * _environment, char * _variable );
Variable *              input_string( Environment * _environment, char * _size );

//----------------------------------------------------------------------------
// *J*
//----------------------------------------------------------------------------

Variable *              joy( Environment * _environment, char * _port );
Variable *              joy_direction( Environment * _environment, char * _port, int _direction );

//----------------------------------------------------------------------------
// *K*
//----------------------------------------------------------------------------

Variable *              key_state( Environment * _environment, char * _scancode );
Variable *              keyshift( Environment * _environment );

//----------------------------------------------------------------------------
// *L*
//----------------------------------------------------------------------------

void                    loop( Environment * _environment, char *_label );

//----------------------------------------------------------------------------
// *M*
//----------------------------------------------------------------------------

Variable *              maximum( Environment * _environment, char * _source, char * _dest );
Variable *              minimum( Environment * _environment, char * _source, char * _dest );

//----------------------------------------------------------------------------
// *N*
//----------------------------------------------------------------------------

void                    next_raster( Environment * _environment );
void                    next_raster_at_with( Environment * _environment, int _at, char * _with );
void                    next_raster_at_with_var( Environment * _environment, char * _var, char * _with );

//----------------------------------------------------------------------------
// *O*
//----------------------------------------------------------------------------

void                    on_gosub( Environment * _environment, char * _expression );
void                    on_gosub_end( Environment * _environment );
void                    on_gosub_index( Environment * _environment, char * _label );
void                    on_goto( Environment * _environment, char * _expression );
void                    on_goto_end( Environment * _environment );
void                    on_goto_index( Environment * _environment, char * _label );
void                    on_proc( Environment * _environment, char * _expression );
void                    on_proc_end( Environment * _environment );
void                    on_proc_index( Environment * _environment, char * _label );

//----------------------------------------------------------------------------
// *P*
//----------------------------------------------------------------------------

void                    paper( Environment * _environment, char * _paper );
Variable *              param_procedure( Environment * _environment, char * _name );
Variable *              peek( Environment * _environment, int _location );
Variable *              peek_var( Environment * _environment, char * _location );
void                    pen( Environment * _environment, char * _color );
void                    plot( Environment * _environment, char * _x, char * _y, char *_c );
Variable *              point( Environment * _environment, char * _x, char * _y );
void                    point_at( Environment * _environment, int _x, int _y );
void                    point_at_vars( Environment * _environment, char * _x, char * _y );
void                    pop( Environment * _environment );
Variable *              powering( Environment * _environment, char * _source, char * _dest );
void                    print( Environment * _environment, char * _text, int _new_line );
void                    print_newline( Environment * _environment );
void                    print_question_mark( Environment * _environment );
void                    print_tab( Environment * _environment, int _new_line );

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
void                    repeat( Environment * _environment, char *_label );
void                    return_label( Environment * _environment );
void                    return_procedure( Environment * _environment, char * _value );
Variable *              rnd( Environment * _environment, char * _value );

//----------------------------------------------------------------------------
// *S*
//----------------------------------------------------------------------------

Variable *              scancode( Environment * _environment );
Variable *              scanshift( Environment * _environment );
Variable *              screen_get_height( Environment * _environment );
Variable *              screen_get_width( Environment * _environment );
void                    screen_horizontal_scroll( Environment * _environment, int _displacement );
void                    screen_horizontal_scroll_var( Environment * _environment, char * _displacement );
void                    screen_off( Environment * _environment );
void                    screen_on( Environment * _environment );
void                    screen_rows( Environment * _environment, int _rows );
void                    screen_rows_var( Environment * _environment, char * _rows );
void                    screen_vertical_scroll( Environment * _environment, int _displacement );
void                    screen_vertical_scroll_var( Environment * _environment, char * _displacement );
void                    shared( Environment * _environment );
Variable *              sign( Environment * _environment, char * _value );
void                    sprite_color( Environment * _environment, int _sprite, int _color );
void                    sprite_color_vars( Environment * _environment, char * _sprite, char * _color );
void                    sprite_compress_horizontal( Environment * _environment, int _sprite );
void                    sprite_compress_horizontal_var( Environment * _environment, char * _sprite );
void                    sprite_compress_vertical( Environment * _environment, int _sprite );
void                    sprite_compress_vertical_var( Environment * _environment, char * _sprite );
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
void                    sprite_monocolor( Environment * _environment, int _sprite );
void                    sprite_monocolor_var( Environment * _environment, char * _sprite );
void                    sprite_multicolor( Environment * _environment, int _sprite );
void                    sprite_multicolor_var( Environment * _environment, char * _sprite );
void                    sprite_position( Environment * _environment, int _sprite, int _x, int _y );
void                    sprite_position_vars( Environment * _environment, char * _sprite, char * _x, char * _y );

//----------------------------------------------------------------------------
// *T*
//----------------------------------------------------------------------------

void                    text_at( Environment * _environment, char * _x, char * _y, char * _text );
void                    text_center( Environment * _environment, char * _string );
void                    text_cline( Environment * _environment, char * _characters );
void                    text_cmove( Environment * _environment, char * _dx, char * _dy );
void                    text_cmove_direct( Environment * _environment, int _dx, int _dy );
void                    text_encoded_at( Environment * _environment, char * _x, char * _y, char * _text, char * _pen, char * _paper, char * _ww );
Variable *              text_get_at( Environment * _environment, char * _x, char * _y );
Variable *              text_get_cmove( Environment * _environment, char * _x, char * _y );
Variable *              text_get_cmove_direct( Environment * _environment, int _x, int _y );
Variable *              text_get_paper( Environment * _environment, char * _color );
Variable *              text_get_pen( Environment * _environment, char * _color );
Variable *              text_get_tab( Environment * _environment );
Variable *              text_get_xcurs( Environment * _environment );
Variable *              text_get_ycurs( Environment * _environment );
void                    text_home( Environment * _environment );
void                    text_hscroll_line( Environment * _environment, int _direction );
void                    text_hscroll_screen( Environment * _environment, int _direction );
void                    text_locate( Environment * _environment, char * _x, char * _y );
void                    text_memorize( Environment * _environment );
void                    text_newline( Environment * _environment );
void                    text_question_mark( Environment * _environment );
void                    text_remember( Environment * _environment );
void                    text_set_tab( Environment * _environment, char * _net_tab );
void                    text_shade( Environment * _environment, int _value );
void                    text_tab( Environment * _environment );
void                    text_text( Environment * _environment, char * _text );
void                    text_under( Environment * _environment, int _value );
void                    text_vscroll( Environment * _environment );
void                    text_vscroll_screen( Environment * _environment, int _direction );
void                    text_writing( Environment * _environment, char * _mode, char * _parts );
void                    textmap_at( Environment * _environment, int _address );
void                    textmap_at_var( Environment * _environment, char * _address );
void                    tilemap_disable( Environment * _environment );
void                    tilemap_enable( Environment * _environment, int _width, int _height, int _colors );
void                    tiles_at( Environment * _environment, int _address );
void                    tiles_at_var( Environment * _environment, char * _address );

//----------------------------------------------------------------------------
// *V*
//----------------------------------------------------------------------------

Variable *              variable_add( Environment * _environment, char * _source, char * _dest );
Variable *              variable_and( Environment * _environment, char * _left, char * _right );
Variable *              variable_and_const( Environment * _environment, char * _source, int _mask );
Variable *              variable_array_type( Environment * _environment, char *_name, VariableType _type );
Variable *              variable_bin( Environment * _environment, char * _value, char * _digits );
Variable *              variable_bit( Environment * _environment, char * _value, char * _position );
Variable *              variable_cast( Environment * _environment, char * _source, VariableType _type );
void                    variable_cleanup( Environment * _Environment );
Variable *              variable_compare( Environment * _environment, char * _source, char * _dest );
Variable *              variable_compare_not( Environment * _environment, char * _source, char * _dest );
Variable *              variable_complement_const( Environment * _environment, char * _source, int _mask );
Variable *              variable_decrement( Environment * _environment, char * _source );
Variable *              variable_define( Environment * _environment, char * _name, VariableType _type, int _value );
Variable *              variable_define_no_init( Environment * _environment, char * _name, VariableType _type );
Variable *              variable_div( Environment * _environment, char * _source, char * _dest );
Variable *              variable_div2_const( Environment * _environment, char * _source, int _bits );
void                    variable_dump( Variable * _first );
void                    variable_global( Environment * _environment, char * _pattern );
Variable *              variable_greater_than( Environment * _environment, char * _source, char * _dest, int _equal );
Variable *              variable_import( Environment * _environment, char * _name, VariableType _type );
Variable *              variable_increment( Environment * _environment, char * _source );
Variable *              variable_less_than( Environment * _environment, char * _source, char * _dest, int _equal );
Variable *              variable_move( Environment * _environment, char * _source, char * _dest );
void                    variable_move_array( Environment * _environment, char * _array, char * _value  );
void                    variable_move_array_string( Environment * _environment, char * _array, char * _string  );
Variable *              variable_move_from_array( Environment * _environment, char * _array );
Variable *              variable_move_naked( Environment * _environment, char * _source, char * _dest );
Variable *              variable_mul( Environment * _environment, char * _source, char * _dest );
Variable *              variable_mul2_const( Environment * _environment, char * _source, int _bits );
Variable *              variable_not( Environment * _environment, char * _value );
Variable *              variable_or( Environment * _environment, char * _left, char * _right );
Variable *              variable_or( Environment * _environment, char * _source, char * _dest );
void                    variable_reset( Environment * _environment );
Variable *              variable_resize_buffer( Environment * _environment, char * _destination, int _size );
Variable *              variable_retrieve( Environment * _environment, char * _name );
Variable *              variable_retrieve_by_realname( Environment * _environment, char * _name );
Variable *              variable_retrieve_or_define( Environment * _environment, char * _name, VariableType _type, int _value );
Variable *              variable_store( Environment * _environment, char * _source, int _value );
Variable *              variable_store_string( Environment * _environment, char * _source, char * _string );
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
Variable *              variable_temporary( Environment * _environment, VariableType _type, char * _meaning );

//----------------------------------------------------------------------------
// *W*
//----------------------------------------------------------------------------

void                    wait_cycles( Environment * _environment, int _timing );
void                    wait_cycles_var( Environment * _environment, char * _timing );
void                    wait_key( Environment * _environment );
void                    wait_milliseconds( Environment * _environment, int _timing );
void                    wait_milliseconds_var( Environment * _environment, char * _timing );
void                    wait_ticks( Environment * _environment, int _timing );
void                    wait_ticks_var( Environment * _environment, char * _timing );

//----------------------------------------------------------------------------
// *X*
//----------------------------------------------------------------------------

Variable *              xpen( Environment * _environment );

//----------------------------------------------------------------------------
// *Y*
//----------------------------------------------------------------------------

Variable *              ypen( Environment * _environment );

#ifdef __c64__
    #include "hw/6502.h"
    #include "hw/vic2.h"
    #include "hw/c64.h"
#elif __zx__
    #include "hw/z80.h"
    #include "hw/zx.h"
#endif

#endif