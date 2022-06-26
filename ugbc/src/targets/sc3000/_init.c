/*****************************************************************************
 * ugBASIC - an isomorphic BASIC language compiler for retrocomputers        *
 *****************************************************************************
 * Copyright 2021-2022 Marco Spedaletti (asimov@mclink.it)
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

/****************************************************************************
 * INCLUDE SECTION 
 ****************************************************************************/

#include "../../ugbc.h"

/****************************************************************************
 * CODE SECTION 
 ****************************************************************************/

extern char OUTPUT_FILE_TYPE_AS_STRING[][16];

void setup_embedded( Environment * _environment ) {

    _environment->embedded.cpu_fill_blocks = 1;
    _environment->embedded.cpu_fill = 1;
    _environment->embedded.cpu_math_div2_8bit = 1;
    _environment->embedded.cpu_math_mul_8bit_to_16bit = 1;
    _environment->embedded.cpu_math_div_8bit_to_8bit = 1;
    _environment->embedded.cpu_math_div2_const_8bit = 1;
    _environment->embedded.cpu_math_mul2_const_8bit = 1;
    _environment->embedded.cpu_math_mul_16bit_to_32bit = 1;
    _environment->embedded.cpu_math_div_16bit_to_16bit = 1;
    _environment->embedded.cpu_math_div_32bit_to_16bit = 1;
    _environment->embedded.cpu_random = 1;
    _environment->embedded.cpu_mem_move = 1;
    _environment->embedded.cpu_uppercase = 1;
    _environment->embedded.cpu_lowercase = 1;

}

void target_initialization( Environment * _environment ) {

    // MEMORY_AREA_DEFINE( MAT_RAM, 0xd000, 0xdff0 );

    _environment->dstring.count = 16;
    _environment->dstring.space = 128;
    
    variable_import( _environment, "EVERYSTATUS", VT_BYTE, 0 );
    variable_global( _environment, "EVERYSTATUS" );

    variable_import( _environment, "BITMAPADDRESS", VT_ADDRESS, 0x0000 );
    variable_global( _environment, "BITMAPADDRESS" );
    variable_import( _environment, "COLORMAPADDRESS", VT_ADDRESS, 0x0000 );
    variable_global( _environment, "COLORMAPADDRESS" );
    variable_import( _environment, "TEXTADDRESS", VT_ADDRESS, 0x0000 );
    variable_global( _environment, "TEXTADDRESS" );    
    variable_import( _environment, "TILESADDRESS", VT_ADDRESS, 0x0000 );
    variable_global( _environment, "TILESADDRESS" );    
    variable_import( _environment, "EMPTYTILE", VT_TILE, 32 );
    variable_global( _environment, "EMPTYTILE" );    
    variable_import( _environment, "USING", VT_BYTE, 0 );

    variable_import( _environment, "COPYOFBITMAPADDRESS", VT_ADDRESS, 0x0000 );
    variable_global( _environment, "COPYOFBITMAPADDRESS" );
    variable_import( _environment, "COPYOFCOLORMAPADDRESS", VT_ADDRESS, 0x0000 );
    variable_global( _environment, "COPYOFCOLORMAPADDRESS" );
    variable_import( _environment, "COPYOFTEXTADDRESS", VT_ADDRESS, 0x0000 );
    variable_global( _environment, "COPYOFTEXTADDRESS" );    
    variable_import( _environment, "COPYOFTILESADDRESS", VT_ADDRESS, 0x0000 );
    variable_global( _environment, "COPYOFTILESADDRESS" );    

    variable_import( _environment, "VDPDATAPORTREAD", VT_BYTE, 0xbe );
    variable_global( _environment, "VDPDATAPORTREAD" );

    variable_import( _environment, "VDPDATAPORTWRITE", VT_BYTE, 0xbe );
    variable_global( _environment, "VDPDATAPORTWRITE" );

    variable_import( _environment, "VDPCONTROLPORTREAD", VT_BYTE, 0xbf );
    variable_global( _environment, "VDPCONTROLPORTREAD" );

    variable_import( _environment, "VDPCONTROLPORTWRITE", VT_BYTE, 0xbf );
    variable_global( _environment, "VDPCONTROLPORTWRITE" );

    variable_import( _environment, "SC3000TIMER", VT_WORD, 0 );
    variable_global( _environment, "SC3000TIMER" );   

    variable_import( _environment, "PPIKEYBOARD", VT_BYTE, 0 );
    variable_global( _environment, "PPIKEYBOARD" );   

    variable_import( _environment, "VDP60HZ", VT_BYTE, 0 );
    variable_global( _environment, "VDP60HZ" );   

    variable_import( _environment, "IRQVECTOR", VT_BUFFER, 3 );
    variable_global( _environment, "IRQVECTOR" );   
    variable_import( _environment, "IRQVECTORREADY", VT_BYTE, 0 );
    variable_global( _environment, "IRQVECTORREADY" );   

    bank_define( _environment, "VARIABLES", BT_VARIABLES, 0x5000, NULL );
    bank_define( _environment, "TEMPORARY", BT_TEMPORARY, 0x5100, NULL );

    outhead0("SECTION code_user");
    outhead0("ORG $0000");
    outhead0("SECTION data_user");
    outhead0("ORG $C000");
    outhead0("SECTION code_user");

    deploy_inplace(startup,src_hw_sc3000_startup_asm);
    deploy( font, src_hw_tms9918_font_asm );

    outhead0("CODESTART:")
    
    outline0("CALL VARINIT2");
    outline0("CALL VARINIT");
    outline0("CALL PROTOTHREADINIT" );
    outline0("CALL SC3000STARTUP");

    setup_text_variables( _environment );
    
    tms9918_initialization( _environment );
    sn76489_initialization( _environment );

    outline0("call	CheckIf60Hz");
    outline0("ld		(VDP60HZ),a				; save it, 00/01 = 50/60 Hz		");

    z80_compare_and_branch_8bit_const( _environment, "LASTVAR", 0x42, "CODESTARTRUN", 1 );

    Variable * outOfMemoryMessage = variable_define( _environment, "OOM", VT_STRING, 0 );
    variable_store_string( _environment, outOfMemoryMessage->name, "OOM" );
    print( _environment, outOfMemoryMessage->name, 1 );

    cpu_halt( _environment );

    outhead0("CODESTARTRUN:")

    if ( _environment->tenLinerRulesEnforced ) {
        shell_injection( _environment );
    }

}

void target_linkage( Environment * _environment ) {

    char commandLine[8*MAX_TEMPORARY_STORAGE];
    char executableName[MAX_TEMPORARY_STORAGE];
    char binaryName[64];
    char listingFileName[64];
    
    if ( _environment->outputFileType != OUTPUT_FILE_TYPE_ROM ) {
        CRITICAL_UNSUPPORTED_OUTPUT_FILE_TYPE( OUTPUT_FILE_TYPE_AS_STRING[_environment->outputFileType] );
    }

    char pipes[256];

    #ifdef _WIN32
        strcpy( pipes, ">nul 2>nul");
    #else
        strcpy( pipes, ">/dev/null 2>/dev/null");
    #endif

    if ( _environment->compilerFileName ) {
        sprintf(executableName, "%s", _environment->compilerFileName );
    } else if( access( "z88dk-z80asm.exe", F_OK ) == 0 ) {
        sprintf(executableName, "%s", "z88dk-z80asm.exe" );
    } else {
        sprintf(executableName, "%s", "z88dk-z80asm" );
    }

    if ( _environment->listingFileName ) {
        sprintf( listingFileName, "-l" );
    } else {
        strcpy( listingFileName, "" );
    }

    sprintf( commandLine, "\"%s\" %s -D__sc3000__ -b \"%s\"",
        executableName,
        listingFileName,
        _environment->asmFileName );

    if ( system_call( _environment,  commandLine ) ) {
        printf("The compilation of assembly program failed.\n\n");
        printf("Please use option '-I' to install chain tool.\n\n");
        return;
    }; 

    strcpy( binaryName, _environment->asmFileName );
    char * p = strstr( binaryName, ".asm" );
    if ( p ) {
        *(p+1) = 'o';
        *(p+2) = 0;
    }
    remove(binaryName);

    strcpy( binaryName, _environment->asmFileName );
    p = strstr( binaryName, ".asm" );
    if ( p ) {
        *p = 0;
        --p;
        strcat( p, "_code_user.bin");
    }

    FILE * binaryFile = fopen( binaryName, "rb" );
    fseek( binaryFile, 0, SEEK_END );
    long size = ftell( binaryFile );
    fseek( binaryFile, 0, SEEK_SET );
    char * part = malloc( size );
    (void)!fread( part, size, 1, binaryFile );
    fclose( binaryFile );

    strcpy( binaryName, _environment->asmFileName );
    p = strstr( binaryName, ".asm" );
    if ( p ) {
        *p = 0;
        --p;
        strcat( p, ".bin");
    }

    binaryFile = fopen( binaryName, "wb" );
    fwrite( part, size, 1, binaryFile );
    fclose( binaryFile );

    strcpy( binaryName, _environment->asmFileName );
    p = strstr( binaryName, ".asm" );
    if ( p ) {
        *p = 0;
        --p;
        strcat( p, "_data_user.bin");
    }

    binaryFile = fopen( binaryName, "rb" );
    fseek( binaryFile, 0, SEEK_END );
    size = ftell( binaryFile );
    fseek( binaryFile, 0, SEEK_SET );
    part = malloc( size );
    (void)!fread( part, size, 1, binaryFile );
    fclose( binaryFile );

    strcpy( binaryName, _environment->asmFileName );
    p = strstr( binaryName, ".asm" );
    if ( p ) {
        *p = 0;
        --p;
        strcat( p, ".bin");
    }

    binaryFile = fopen( binaryName, "a+b" );
    fwrite( part, size, 1, binaryFile );
    fclose( binaryFile );

    remove( _environment->exeFileName );

    rename( binaryName, _environment->exeFileName );

    if ( _environment->listingFileName ) {
        strcpy( binaryName, _environment->asmFileName );
        p = strstr( binaryName, ".asm" );
        if ( p ) {
            *p = 0;
            --p;
            strcat( p, ".lis");
            rename( binaryName, _environment->listingFileName );
        }

        if ( _environment->profileFileName ) {
            strcpy( binaryName, _environment->profileFileName );
            if ( _environment->executerFileName ) {
                sprintf(executableName, "%s", _environment->executerFileName );
            } else if( access( "runz80.exe", F_OK ) == 0 ) {
                sprintf(executableName, "%s", "runz80.exe" );
            } else {
                sprintf(executableName, "%s", "runz80" );
            }

            sprintf( commandLine, "\"%s\" -c -p \"%s\" %d -l 0000 \"%s\" -R 0000 -u \"%s\" \"%s\"",
                executableName,
                binaryName,
                _environment->profileCycles ? _environment->profileCycles : 1000000,
                _environment->exeFileName,
                _environment->listingFileName,
                pipes );

            if ( system_call( _environment,  commandLine ) ) {
                printf("The profiling of assembly program failed.\n\n");
                return;
            }; 

        }

    }

    strcpy( binaryName, _environment->asmFileName );
    p = strstr( binaryName, ".asm" );
    if ( p ) {
        strcat( p, "_data_user.bin");
    }
    remove(binaryName);

    strcpy( binaryName, _environment->asmFileName );
    p = strstr( binaryName, ".asm" );
    if ( p ) {
        strcat( p, "_code_user.bin");
    }
    remove(binaryName);

}

void interleaved_instructions( Environment * _environment ) {

}