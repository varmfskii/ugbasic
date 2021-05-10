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

    /** Strings (static or dynamic) */
    VT_STRING = 10,

    /** Static buffer of a specific size */
    VT_BUFFER = 11

    // TODO: support for arrays.
} VariableType;

#define VT_BW_8BIT( t, v )              ( ( (t) == (v) ) ? 8 : 0 )
#define VT_BW_16BIT( t, v )             ( ( (t) == (v) ) ? 16 : 0 )
#define VT_BW_32BIT( t, v )             ( ( (t) == (v) ) ? 32 : 0 )

#define VT_BITWIDTH( t ) \
        ( VT_BW_8BIT( t, VT_BYTE ) + VT_BW_8BIT( t, VT_SBYTE ) + VT_BW_8BIT( t, VT_COLOR ) + \
        VT_BW_16BIT( t, VT_WORD ) + VT_BW_16BIT( t, VT_SWORD ) + VT_BW_16BIT( t, VT_ADDRESS ) + VT_BW_16BIT( t, VT_POSITION ) + \
        VT_BW_32BIT( t, VT_DWORD ) + VT_BW_32BIT( t, VT_SDWORD ) )

#define VT_SIGNED( t ) \
        ( ( (t) == VT_SBYTE ) || ( (t) == VT_SWORD ) || ( (t) == VT_SDWORD ) )

#define VT_SIGN_8BIT( v ) ( v < 0 ? ( ((~(unsigned char)(abs(v))) ) ) : (v) )
#define VT_SIGN_16BIT( v ) ( v < 0 ? ( ((~(unsigned short)(abs(v))) ) ) : (v) )
#define VT_SIGN_32BIT( v ) ( v < 0 ? ( (~((unsigned int) (abs(v))) ) ) : (v) )

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
     * The initial value of the variable, as given by last (re)definition.
     */
    int value;

    /** 
     * The size of the static buffer (in bytes).
     */
    int size;

    /** 
     * Pointer to the bank where this variable belongs to.
     */
    Bank * bank;

    /** Link to the next variable (NULL if this is the last one) */
    struct _Variable * next;

} Variable;

/**
 * @brief Types of conditional jumps supported.
 */
typedef enum _ConditionalType {
    /** IF ... THEN ... ENDIF */
    CT_IF = 0,

    /** ON ... GOTO ... */
    CT_ON_GOTO = 1,

    /** ON ... GOSUB ... */
    CT_ON_GOSUB = 2

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

    /** Variable with step. */
    Variable *step;

    /** Next conditional */
    struct _Loop * next;

} Loop;

/**
 * @brief Structure of compilation environment
 * 
 * @todo implement DEF FN
 * @todo implement PROCEDURE
 */
typedef struct _Environment {

    // TODO: implement DEF FN
    // TODO: implement procedures

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

    /**
     * List of temporary (and reused) variables.
     */
    Variable * tempVariables;

    /**
     * List of variables defined in the program.
     */
    Variable * variables;

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
#define CRITICAL2( s, v ) fprintf(stderr, "CRITICAL ERROR during compilation of %s:\n\t%s (%s) at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, v, _environment->yylineno ); exit( EXIT_FAILURE );
#define CRITICAL3( s, v1, v2 ) fprintf(stderr, "CRITICAL ERROR during compilation of %s:\n\t%s (%s, %s) at %d\n", ((struct _Environment *)_environment)->sourceFileName, s, v1, v2, _environment->yylineno ); exit( EXIT_FAILURE );
#define CRITICAL_UNIMPLEMENTED( v ) CRITICAL2("E000 - Internal method not implemented:", v );
#define CRITICAL_TEMPORARY2( v ) CRITICAL2("E001 - Unable to create space for temporary variable", v );
#define CRITICAL_VARIABLE( v ) CRITICAL2("E002 - Using of an undefined variable", v );
#define CRITICAL_DATATYPE_UNSUPPORTED( k, v ) CRITICAL3("E003 - Datatype not supported for keyword", k, v );
#define CRITICAL_CANNOT_CAST( t1, t2 ) CRITICAL3("E004 - Cannot cast types", t1, t2 );
#define CRITICAL_STORE_UNSUPPORTED( t ) CRITICAL2("E005 - Datatype cannot be stored directly", t );
#define CRITICAL_RESIZE_UNSUPPORTED( t ) CRITICAL2("E006 - Datatype cannot be resized directly", t );
#define CRITICAL_MOVE_UNSUPPORTED( t ) CRITICAL2("E007 - Datatype cannot be copied directly", t );
#define CRITICAL_BUFFER_SIZE_MISMATCH( v1, v2 ) CRITICAL3("E008 - Buffer sizes mismatch -- cannot be copied", v1, v2 );
#define CRITICAL_DATATYPE_MISMATCH( v1, v2 ) CRITICAL3("E009 - Datatype mismatch", v1, v2 );
#define CRITICAL_ADD_UNSUPPORTED( v, t ) CRITICAL3("E007 - Add unsupported for variable of given datatype", v, t );
#define CRITICAL_SUB_UNSUPPORTED( v, t ) CRITICAL3("E008 - Sub unsupported for variable of given datatype", v, t );
#define CRITICAL_COMPLEMENT_UNSUPPORTED( v, t ) CRITICAL3("E009 - Complement unsupported for variable of given datatype", v, t );
#define CRITICAL_MUL_UNSUPPORTED( v, t ) CRITICAL3("E010 - Multiplication unsupported for variable of given datatype", v, t );
#define CRITICAL_DIV_UNSUPPORTED( v, t ) CRITICAL3("E011 - Division unsupported for variable of given datatype", v, t );
#define CRITICAL_CANNOT_COMPARE( t1, t2 ) CRITICAL3("E012 - Cannot compare types", t1, t2 );
#define CRITICAL_MUL2_UNSUPPORTED( v, t ) CRITICAL3("E013 - Double unsupported for variable of given datatype", v, t );
#define CRITICAL_DIV2_UNSUPPORTED( v, t ) CRITICAL3("E014 - Division by 2 unsupported for variable of given datatype", v, t );
#define CRITICAL_AND_UNSUPPORTED( v, t ) CRITICAL3("E015 - Bitwise AND unsupported for variable of given datatype", v, t );
#define CRITICAL_LEFT_UNSUPPORTED( v, t ) CRITICAL3("E016 - LEFT unsupported for variable of given datatype", v, t );
#define CRITICAL_RIGHT_UNSUPPORTED( v, t ) CRITICAL3("E017 - RIGHT unsupported for variable of given datatype", v, t );
#define CRITICAL_MID_UNSUPPORTED( v, t ) CRITICAL3("E018 - MID unsupported for variable of given datatype", v, t );
#define CRITICAL_INSTR_UNSUPPORTED( v, t ) CRITICAL3("E019 - INSTR unsupported for variable of given datatype", v, t );
#define CRITICAL_STRING_UNSUPPORTED( v, t ) CRITICAL3("E020 - STRING unsupported for variable of given datatype", v, t );
#define CRITICAL_UPPER_UNSUPPORTED( v, t ) CRITICAL3("E021 - UPPER unsupported for variable of given datatype", v, t );
#define CRITICAL_LOWER_UNSUPPORTED( v, t ) CRITICAL3("E022 - LOWER unsupported for variable of given datatype", v, t );
#define CRITICAL_STR_UNSUPPORTED( v, t ) CRITICAL3("E023 - STR unsupported for variable of given datatype", v, t );
#define CRITICAL_VAL_UNSUPPORTED( v, t ) CRITICAL3("E024 - VAL unsupported for variable of given datatype", v, t );
#define CRITICAL_CHR_UNSUPPORTED( v, t ) CRITICAL3("E025 - CHR unsupported for variable of given datatype", v, t );
#define CRITICAL_ASC_UNSUPPORTED( v, t ) CRITICAL3("E026 - ASC unsupported for variable of given datatype", v, t );
#define CRITICAL_LEN_UNSUPPORTED( v, t ) CRITICAL3("E027 - LEN unsupported for variable of given datatype", v, t );
#define CRITICAL_POW_UNSUPPORTED( v, t ) CRITICAL3("E028 - ^ unsupported for variable of given datatype", v, t );
#define CRITICAL_SGN_UNSUPPORTED( v, t ) CRITICAL3("E029 - SGN unsupported for variable of given datatype", v, t );
#define CRITICAL_ABS_UNSUPPORTED( v, t ) CRITICAL3("E030 - ABS unsupported for variable of given datatype", v, t );
#define CRITICAL_DEBUG_UNSUPPORTED( v, t ) CRITICAL3("E031 - DEBUG unsupported for variable of given datatype", v, t );
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

Variable * absolute( Environment * _environment, char * _value );
void add_complex( Environment * _environment, char * _variable, char * _expression, char * _limit_lower, char * _limit_upper );
Bank * bank_define( Environment * _environment, char * _name, BankType _type, int _address, char * _filename );
void   bank_cleanup( Environment * _environment );
void begin_loop( Environment * _environment );
void begin_while( Environment * _environment );
void begin_while_condition( Environment * _environment, char * _expression );
void begin_gameloop( Environment * _environment );
void begin_repeat( Environment * _environment );
void begin_for( Environment * _environment, char * _index, char * _from, char * _to );  
void begin_for_step( Environment * _environment, char * _index, char * _from, char * _to, char * _step );  
void bitmap_at( Environment * _environment, int _address );
void bitmap_at_var( Environment * _environment, char * _address );
void bitmap_enable( Environment * _environment );
void bitmap_disable( Environment * _environment );
void bitmap_clear( Environment * _environment );
void bitmap_clear_with( Environment * _environment, int _value );
void bitmap_clear_with_vars( Environment * _environment, char * _value );
Variable * collision_to( Environment * _environment, int _sprite );
Variable * collision_to_vars( Environment * _environment, char * _sprite );
void color_border( Environment * _environment, int _border_color );
void color_border_var( Environment * _environment, char * _border_color );
void color_background( Environment * _environment, int _index, int _background_color );
void color_background_vars( Environment * _environment, char * _index, char * _background_color );
void color_sprite( Environment * _environment, int _index, int _color );
void color_sprite_vars( Environment * _environment, char * _sprite, char * _color );
void colormap_at( Environment * _environment, int _address );
void colormap_at_var( Environment * _environment, char * _address );
void colormap_clear( Environment * _environment );
void colormap_clear_with( Environment * _environment, int _foreground, int _background );
void colormap_clear_with_vars( Environment * _environment, char * _foreground, char * _background );
void debug_var( Environment * _environment, char *_name );
void else_if_then( Environment * _environment, char * _expression );
void end_loop( Environment * _environment );
void end_while( Environment * _environment );
void end_gameloop( Environment * _environment );
void end_if_then( Environment * _environment  );
void end_repeat( Environment * _environment, char * _expression );
void end_for( Environment * _environment );
void every_on( Environment * _environment );
void every_off( Environment * _environment );
void every_ticks_gosub( Environment * _environment, char * _timing, char * _label );
void exit_loop( Environment * _environment, int _number );
void exit_loop_if( Environment * _environment, char * _expression, int _number );
void gameloop_cleanup( Environment * _environment );
void goto_label( Environment * _environment, char * _label );
void goto_number( Environment * _environment, int _number );
void gosub_label( Environment * _environment, char * _label );
void gosub_number( Environment * _environment, int _number );
void graphic( Environment * _environment );
void halt( Environment * _environment );
Variable * hit_to( Environment * _environment, int _sprite );
Variable * hit_to_vars( Environment * _environment, char * _sprite );
void if_then( Environment * _environment, char * _expression );
void ink( Environment * _environment, char * _expression );
int is_word( Environment * _environment, char * _name );
void linker_setup( Environment * _environment );
void linker_cleanup( Environment * _environment );
void loop( Environment * _environment, char *_label );
Variable * maximum( Environment * _environment, char * _source, char * _dest );
Variable * minimum( Environment * _environment, char * _source, char * _dest );
void next_raster( Environment * _environment );
void next_raster_at_with( Environment * _environment, int _at, char * _with );
void next_raster_at_with_var( Environment * _environment, char * _var, char * _with );
void on_gosub( Environment * _environment, char * _expression );
void on_gosub_index( Environment * _environment, char * _label );
void on_gosub_end( Environment * _environment );
void on_goto( Environment * _environment, char * _expression );
void on_goto_index( Environment * _environment, char * _label );
void on_goto_end( Environment * _environment );
Variable * peek( Environment * _environment, int _location );
Variable * peek_var( Environment * _environment, char * _location );
void point_at( Environment * _environment, int _x, int _y );
void point_at_vars( Environment * _environment, char * _x, char * _y );
void pop( Environment * _environment );
Variable * powering( Environment * _environment, char * _source, char * _dest );
void randomize( Environment * _environment, char * _seed );
Variable * random_value( Environment * _environment, VariableType _type );
Variable * random_width( Environment * _environment );
Variable * random_height( Environment * _environment );
Variable * get_timer( Environment * _environment );
void raster_at( Environment * _environment, char * _label, int _position );
void raster_at_var( Environment * _environment, char * _label, char * _position );
void repeat( Environment * _environment, char *_label );
void return_label( Environment * _environment );
Variable * rnd( Environment * _environment, char * _value );
void screen_on( Environment * _environment );
void screen_off( Environment * _environment );
void screen_rows( Environment * _environment, int _rows );
void screen_rows_var( Environment * _environment, char * _rows );
void screen_vertical_scroll( Environment * _environment, int _displacement );
void screen_vertical_scroll_var( Environment * _environment, char * _displacement );
void screen_horizontal_scroll( Environment * _environment, int _displacement );
void screen_horizontal_scroll_var( Environment * _environment, char * _displacement );
Variable * screen_get_width( Environment * _environment );
Variable * screen_get_height( Environment * _environment );
Variable * sign( Environment * _environment, char * _value );
void sprite_data_from( Environment * _environment, int _sprite, int _address );
void sprite_data_from_vars( Environment * _environment, char * _sprite, char * _address );
void sprite_disable( Environment * _environment, int _sprite );
void sprite_disable_var( Environment * _environment, char * _sprite );
void sprite_enable( Environment * _environment, int _sprite );
void sprite_enable_var( Environment * _environment, char * _sprite );
void sprite_expand_vertical( Environment * _environment, int _sprite );
void sprite_expand_vertical_var( Environment * _environment, char * _sprite );
void sprite_expand_horizontal( Environment * _environment, int _sprite );
void sprite_expand_horizontal_var( Environment * _environment, char * _sprite );
void sprite_color( Environment * _environment, int _sprite, int _color );
void sprite_color_vars( Environment * _environment, char * _sprite, char * _color );
void sprite_compress_vertical( Environment * _environment, int _sprite );
void sprite_compress_vertical_var( Environment * _environment, char * _sprite );
void sprite_compress_horizontal( Environment * _environment, int _sprite );
void sprite_compress_horizontal_var( Environment * _environment, char * _sprite );
void sprite_monocolor( Environment * _environment, int _sprite );
void sprite_monocolor_var( Environment * _environment, char * _sprite );
void sprite_multicolor( Environment * _environment, int _sprite );
void sprite_multicolor_var( Environment * _environment, char * _sprite );
void sprite_position( Environment * _environment, int _sprite, int _x, int _y );
void sprite_position_vars( Environment * _environment, char * _sprite, char * _x, char * _y );
void text_enable( Environment * _environment );
void text_disable( Environment * _environment );
void text_at( Environment * _environment, char * _x, char * _y, char * _text );
void textmap_at( Environment * _environment, int _address );
void textmap_at_var( Environment * _environment, char * _address );
void tiles_at( Environment * _environment, int _address );
void tiles_at_var( Environment * _environment, char * _address );
void       variable_reset( Environment * _environment );
Variable * variable_define( Environment * _environment, char * _name, VariableType _type, int _value );
Variable * variable_retrieve( Environment * _environment, char * _name );
Variable * variable_resize_buffer( Environment * _environment, char * _destination, int _size );
Variable * variable_cast( Environment * _environment, char * _source, VariableType _type );
Variable * variable_temporary( Environment * _environment, VariableType _type, char * _meaning );
void variable_cleanup( Environment * _Environment );
Variable * variable_store( Environment * _environment, char * _source, int _value );
Variable * variable_store_string( Environment * _environment, char * _source, char * _string );
Variable * variable_move( Environment * _environment, char * _source, char * _dest );
Variable * variable_move_naked( Environment * _environment, char * _source, char * _dest );
Variable * variable_compare( Environment * _environment, char * _source, char * _dest );
Variable * variable_compare_not( Environment * _environment, char * _source, char * _dest );
Variable * variable_less_than( Environment * _environment, char * _source, char * _dest, int _equal );
Variable * variable_greater_than( Environment * _environment, char * _source, char * _dest, int _equal );
Variable * variable_increment( Environment * _environment, char * _source );
Variable * variable_decrement( Environment * _environment, char * _source );
Variable * variable_add( Environment * _environment, char * _source, char * _dest );
Variable * variable_or( Environment * _environment, char * _source, char * _dest );
Variable * variable_sub( Environment * _environment, char * _source, char * _dest );
Variable * variable_mul( Environment * _environment, char * _source, char * _dest );
Variable * variable_div( Environment * _environment, char * _source, char * _dest );
Variable * variable_and( Environment * _environment, char * _left, char * _right );
Variable * variable_or( Environment * _environment, char * _left, char * _right );
Variable * variable_not( Environment * _environment, char * _value );
Variable * variable_div2_const( Environment * _environment, char * _source, int _bits );
Variable * variable_mul2_const( Environment * _environment, char * _source, int _bits );
Variable * variable_and_const( Environment * _environment, char * _source, int _mask );
Variable * variable_complement_const( Environment * _environment, char * _source, int _mask );
Variable * variable_string_left( Environment * _environment, char * _string, char * _position );
void variable_string_left_assign( Environment * _environment, char * _string, char * _position, char * _expression );
Variable * variable_string_right( Environment * _environment, char * _string, char * _position );
void variable_string_right_assign( Environment * _environment, char * _string, char * _position, char * _expression );
Variable * variable_string_mid( Environment * _environment, char * _string, char * _position, char * _len );
void variable_string_mid_assign( Environment * _environment, char * _string, char * _position, char * _len, char * _expression );
Variable * variable_string_instr( Environment * _environment, char * _search, char * _searched, char * _start );
Variable * variable_string_upper( Environment * _environment, char * _string );
Variable * variable_string_lower( Environment * _environment, char * _string );
Variable * variable_string_str( Environment * _environment, char * _value );
Variable * variable_string_val( Environment * _environment, char * _value );
Variable * variable_string_string( Environment * _environment, char * _string, char * _repetitions  );
Variable * variable_string_space( Environment * _environment, char * _repetitions  );
Variable * variable_string_flip( Environment * _environment, char * _string  );
Variable * variable_string_chr( Environment * _environment, char * _ascii  );
Variable * variable_string_asc( Environment * _environment, char * _char );
Variable * variable_string_len( Environment * _environment, char * _string );
void wait_cycles( Environment * _environment, int _timing );
void wait_cycles_var( Environment * _environment, char * _timing );
void wait_ticks( Environment * _environment, int _timing );
void wait_ticks_var( Environment * _environment, char * _timing );
void wait_milliseconds( Environment * _environment, int _timing );
void wait_milliseconds_var( Environment * _environment, char * _timing );
Variable * xpen( Environment * _environment );
Variable * ypen( Environment * _environment );

#ifdef __c64__
    #include "hw/vic2.h"
    #include "hw/6502.h"
#elif __zx__
    #include "hw/z80.h"
    #include "hw/zx.h"
#endif

#endif