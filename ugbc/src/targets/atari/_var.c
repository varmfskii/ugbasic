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

/****************************************************************************
 * INCLUDE SECTION 
 ****************************************************************************/

#include "../../ugbc.h"

/****************************************************************************
 * CODE SECTION 
 ****************************************************************************/

extern char BANK_TYPE_AS_STRING[][16];
extern char DATATYPE_AS_STRING[][16];

static void variable_cleanup_entry( Environment * _environment, Variable * _first ) {

    Variable * variable = _first;

    while( variable ) {

        if ( ( !variable->assigned || ( variable->assigned && !variable->temporary ) ) && !variable->imported && !variable->memoryArea ) {

            if ( variable->memoryArea && _environment->debuggerLabelsFile ) {
                fprintf( _environment->debuggerLabelsFile, "%4.4x %s\r\n", variable->absoluteAddress, variable->realName );
            }

            switch( variable->type ) {
                case VT_CHAR:
                case VT_BYTE:
                case VT_SBYTE:
                case VT_COLOR:
                case VT_THREAD:
                    if ( variable->memoryArea ) {
                        // outline2("%s = $%4.4x", variable->realName, variable->absoluteAddress);
                    } else {
                        outline1("%s: .res 1,0", variable->realName);
                    }        
                    break;
                case VT_WORD:
                case VT_SWORD:
                case VT_POSITION:
                case VT_ADDRESS:
                    if ( variable->memoryArea ) {
                        // outline2("%s = $%4.4x", variable->realName, variable->absoluteAddress);
                    } else {
                        outline1("%s: .res 2,0", variable->realName);
                    }
                    break;
                case VT_DWORD:
                case VT_SDWORD:
                    if ( variable->memoryArea ) {
                        // outline2("%s = $%4.4x", variable->realName, variable->absoluteAddress);
                    } else {
                        outline1("%s: .res 4,0", variable->realName);
                    }
                    break;
                case VT_FLOAT:
                    if ( variable->memoryArea ) {
                        // outline2("%s = $%4.4x", variable->realName, variable->absoluteAddress);
                    } else {
                        outline1("%s: .res 4,0", variable->realName);
                    }
                    break;
                case VT_STRING:
                    if ( variable->memoryArea ) {
                        // outline2("%s = $%4.4x", variable->realName, variable->absoluteAddress);
                    } else {
                        // if ( variable->printable ) {
                        //     int c = strlen( variable->valueString->value );
                        //     out2("%s: .byte %d,", variable->realName, c);
                        //     int i=0;
                        //     for (i=0; i<(c-1); ++i ) {
                        //         out1("$%2.2x,", (unsigned char)variable->valueString->value[i]);
                        //     }
                        //     outline1("$%2.2x", (unsigned char)variable->valueString->value[(c-1)]);                        
                        // } else {
                        //     outline3("%s: .byte %d,%s", variable->realName, (int)strlen(variable->valueString->value), escape_newlines( variable->valueString->value ) );
                        // }
                        outline2("%s = cstring%d", variable->realName, variable->valueString->id );
                    }
                    break;
                case VT_DSTRING:
                    if ( variable->memoryArea ) {
                        // outline2("%s = $%4.4x", variable->realName, variable->absoluteAddress);
                    } else {
                        outline1("%s: .res 1,0", variable->realName);
                    }
                    break;
                case VT_TILE:
                case VT_SPRITE:
                case VT_TILESET:
                    if ( variable->memoryArea ) {
                        // outline2("%s = $%4.4x", variable->realName, variable->absoluteAddress);
                    } else {
                        outline1("%s: .res 1,0", variable->realName);
                    }
                    break;
                case VT_TILES:
                    if ( variable->memoryArea ) {
                        // outline2("%s = $%4.4x", variable->realName, variable->absoluteAddress);
                    } else {
                        outline1("%s: .res 4,0", variable->realName);
                    }
                    break;
                case VT_IMAGE:
                case VT_IMAGES:
                case VT_SEQUENCE:
                case VT_MUSIC:
                case VT_BUFFER:
                    if ( ! variable->absoluteAddress ) {
                        if ( variable->valueBuffer ) {
                            if ( variable->printable ) {
                                char * string = malloc( variable->size + 1 );
                                memset( string, 0, variable->size );
                                memcpy( string, variable->valueBuffer, variable->size );
                                outline2("%s: .byte %s", variable->realName, escape_newlines( string ) );
                            } else {
                                out1("%s: .byte ", variable->realName);
                                int i=0;
                                for (i=0; i<(variable->size-1); ++i ) {
                                    if ( (i+1) % 16 == 0 ) {
                                        outline1("$%2.2x", (unsigned char)( variable->valueBuffer[i] & 0xff ) );
                                        out0("    .byte ");
                                    } else {
                                        out1("$%2.2x,", (unsigned char)( variable->valueBuffer[i] & 0xff ) );
                                    }
                                }
                                outline1("$%2.2x", (unsigned char)( variable->valueBuffer[(variable->size-1)] ) );
                            }
                        } else {
                            outline2("%s: .res %d,0", variable->realName, variable->size);
                        }
                    } else {
                        if ( ! variable->memoryArea && variable->valueBuffer ) {
                            outline2("%s = $%4.4x", variable->realName, variable->absoluteAddress);
                            if ( variable->printable ) {
                                char * string = malloc( variable->size + 1 );
                                memset( string, 0, variable->size );
                                memcpy( string, variable->valueBuffer, variable->size );
                                outline2("%scopy: .byte %s", variable->realName, escape_newlines( string ) );
                            } else {
                                out1("%scopy: .byte ", variable->realName);
                                int i=0;
                                for (i=0; i<(variable->size-1); ++i ) {
                                    out1("$%2.2x,", (unsigned char) ( variable->valueBuffer[i] & 0xff ) );
                                }
                                outline1("$%2.2x", (unsigned char) ( variable->valueBuffer[(variable->size-1)] & 0xff ) );
                            }
                        }
                    }
                    break;
                case VT_BLIT:
                    break;
                case VT_TILEMAP:
                case VT_ARRAY: {
                    if ( ! variable->memoryArea && variable->valueBuffer ) {
                        out1("%s: .byte ", variable->realName);
                        int i=0;
                        for (i=0; i<(variable->size-1); ++i ) {
                            out1("$%2.2x,", (unsigned char) ( variable->valueBuffer[i] & 0xff ) );
                        }
                        outline1("$2.2x", (unsigned char) ( variable->valueBuffer[(variable->size-1)] & 0xff ) );
                    } else if ( variable->memoryArea && ! variable->value ) {
                        // outline2("%s = $%4.4x", variable->realName, variable->absoluteAddress);
                    } else {
                        if ( variable->value ) {
                            outline3("%s: .res %d, $%2.2x", variable->realName, variable->size, (unsigned char)(variable->value&0xff));
                        } else {
                            outline2("%s: .res %d, 0", variable->realName, variable->size);
                        }
                    }
                    break;
                }
            }
        }
        variable = variable->next;
    }

}

static void variable_cleanup_memory_mapped( Environment * _environment, Variable * _variable ) {

    outhead2("; %s (%4.4x)", _variable->realName, _variable->absoluteAddress );
    outhead1("%s:", _variable->realName );

    switch( _variable->type ) {
        case VT_CHAR:
        case VT_BYTE:
        case VT_SBYTE:
        case VT_COLOR:
        case VT_THREAD:
            outline1(" .byte $%1.1x", ( _variable->value & 0xff ) );
            break;
        case VT_WORD:
        case VT_SWORD:
        case VT_POSITION:
        case VT_ADDRESS:
            outline1(" .word $%2.2x", ( _variable->value & 0xffff ) );
            break;
        case VT_DWORD:
        case VT_SDWORD:
            outline1(" .dword $%4.4x", ( _variable->value & 0xffff ) );
            break;
        case VT_FLOAT: {
            int bytes = VT_FLOAT_BITWIDTH( _variable->precision ) >> 3;
            int * data = malloc( bytes * sizeof( int ) );
            switch( _variable->precision ) {
                case FT_FAST:
                    cpu_float_fast_from_double_to_int_array( _environment, _variable->valueFloating, data );
                    break;
                case FT_SINGLE:
                    cpu_float_single_from_double_to_int_array( _environment, _variable->valueFloating, data );
                    break;
            }
            for( int i=0; i<bytes; ++i ) {
                outline1(" .byte $%2.2x", (unsigned char)( ( data[i] ) & 0xff ) );
            }
            break;
        }
        case VT_STRING:
            // if ( _variable->printable ) {
            //     int c = strlen( _variable->valueString->value );
            //     out1("   .byte %d,", c);
            //     int i=0;
            //     for (i=0; i<(c-1); ++i ) {
            //         out1("$%2.2x,", (unsigned char)_variable->valueString->value[i]);
            //     }
            //     outline1("$%2.2x", (unsigned char)_variable->valueString->value[(c-1)]);                        
            // } else {
            //     outline2("   .byte %d,%s", (int)strlen(_variable->valueString->value), escape_newlines( _variable->valueString->value ) );
            // }
            break;
        case VT_DSTRING:
        case VT_SPRITE:
        case VT_TILE:
        case VT_TILESET:
            outline0("   .byte 0" );
            break;
        case VT_TILES:
            outline0("   .byte 0, 0, 0, 0" );
            break;            
        case VT_IMAGE:
        case VT_IMAGES:
        case VT_SEQUENCE:
        case VT_MUSIC:
        case VT_BUFFER:
            if ( _variable->valueBuffer ) {
                if ( _variable->printable ) {
                    char * string = malloc( _variable->size + 1 );
                    memset( string, 0, _variable->size );
                    memcpy( string, _variable->valueBuffer, _variable->size );
                    outline1("    .byte %s", escape_newlines( string ) );
                } else {
                    out0("    .byte ");
                    int i=0;
                    for (i=0; i<(_variable->size-1); ++i ) {
                        out1("$%2.2x,", (unsigned char) ( _variable->valueBuffer[i] & 0xff ) );
                    }
                    outline1("$%2.2x", (unsigned char) ( _variable->valueBuffer[(_variable->size-1)] & 0xff ) );
                }
            } else {
                outline1(" .res %d,0", _variable->size);
            }
            break;
        case VT_BLIT:
            break;
        case VT_TILEMAP:
        case VT_ARRAY: {
            if ( _variable->valueBuffer ) {
                out0("    .byte ");
                int i=0;
                for (i=0; i<(_variable->size-1); ++i ) {
                    out1("$%2.2x,", (unsigned char) ( _variable->valueBuffer[i] & 0xff ) );
                }
                outline1("$%2.2x", (unsigned char) ( _variable->valueBuffer[(_variable->size-1)] & 0xff ) );
            } else {
                if ( _variable->value ) {
                    outline2("    .res %d, $%2.2x", _variable->size, (unsigned char)(_variable->value&0xff));
                } else {
                    outline1("    .res %d, 0", _variable->size);
                }
            }
            break;
        }
    }

}

static void variable_cleanup_entry_bit( Environment * _environment, Variable * _first ) {

    Variable * variable = _first;

    int bitCount = 0;

    while( variable ) {

        if ( ( !variable->assigned || ( variable->assigned && !variable->temporary ) ) && !variable->imported && !variable->memoryArea ) {

            if ( variable->memoryArea && _environment->debuggerLabelsFile ) {
                fprintf( _environment->debuggerLabelsFile, "%4.4x %s\r\n", variable->absoluteAddress, variable->realName );
            }

            switch( variable->type ) {
                case VT_BIT:
                    if ( variable->memoryArea ) {
                        // outline2("%s = $%4.4x", variable->realName, variable->absoluteAddress);
                    } else {
                        outline1("%s:", variable->realName);
                    }
                    ++bitCount;
                    if ( bitCount == 8 ) {
                        outline0("   .res 1,0");
                    }        
                    break;
            }

        }

        variable = variable->next;

    }

    outline0("   .res 1,0");

}

/**
 * @brief Emit source and configuration lines for variables
 * 
 * This function can be called to generate all the definitions (on the source
 * file, on the configuration file and on any support file) necessary to 
 * implement the variables.
 * 
 * @param _environment Current calling environment
 */
void variable_cleanup( Environment * _environment ) {
    int i=0;

    if ( _environment->dataSegment ) {
        outhead1("DATAFIRSTSEGMENT = %s", _environment->dataSegment->realName );
        if ( _environment->readDataUsed && _environment->restoreDynamic ) {
            outhead0("DATASEGMENTNUMERIC:" );
            DataSegment * actual = _environment->dataSegment;
            while( actual ) {
                if ( actual->isNumeric ) {
                    outline2( ".word $%4.4x, %s", actual->lineNumber, actual->realName );
                }
                actual = actual->next;
            }
            outline0( ".word $ffff, DATAPTRE" );
        }
    }

    if ( _environment->offsetting ) {
        Offsetting * actual = _environment->offsetting;
        while( actual ) {
            out1("OFFSETS%4.4x: .word ", actual->size );
            for( i=0; i<actual->count; ++i ) {
                out1("$%4.4x", i * actual->size );
                if ( i < ( actual->count - 1 ) ) {
                    out0(",");
                } else {
                    outline0("");
                }
            }
            if ( actual->variables ) {
                OffsettingVariable * actualVariable = actual->variables;
                while( actualVariable ) {
                    if ( actualVariable->sequence ) {
                        outhead1("%soffsetsequence:", actualVariable->variable->realName );
                    } else {
                        outhead1("%soffsetframe:", actualVariable->variable->realName );
                    }
                    actualVariable = actualVariable->next;
                }
                outhead1("fs%4.4xoffsetsequence:", actual->size );
                outhead1("fs%4.4xsoffsetframe:", actual->size );
                outline1("LDA #<OFFSETS%4.4x", actual->size );
                outline0("STA MATHPTR1" );
                outline1("LDA #>OFFSETS%4.4x", actual->size );
                outline0("STA MATHPTR1+1" );
                outline0("CLC" );
                outline0("LDA MATHPTR0" );
                outline0("ASL" );
                outline0("TAY" );
                outline0("LDA TMPPTR" );
                outline0("ADC (MATHPTR1), Y" );
                outline0("STA TMPPTR" );
                outline0("INY" );
                outline0("LDA TMPPTR+1" );
                outline0("ADC (MATHPTR1), Y" );
                outline0("STA TMPPTR+1" );
                outline0("RTS" );
            }
            actual = actual->next;
        }

        int values[MAX_TEMPORARY_STORAGE];
        char * address[MAX_TEMPORARY_STORAGE];

        actual = _environment->offsetting;
        int count = 0;
        while( actual ) {
            values[count] = actual->size;
            address[count] = malloc( MAX_TEMPORARY_STORAGE );
            sprintf( address[count], "fs%4.4xsoffsetframe", actual->size );
            actual = actual->next;
            ++count;
        }

        cpu_address_table_build( _environment, "EXECOFFSETS", values, address, count );

        cpu_address_table_lookup( _environment, "EXECOFFSETS", count );

    }

    for(i=0; i<BANK_TYPE_COUNT; ++i) {
        Bank * actual = _environment->banks[i];
        while( actual ) {
            if ( actual->type == BT_VARIABLES ) {
                // cfgline3("# BANK %s %s AT $%4.4x", BANK_TYPE_AS_STRING[actual->type], actual->name, actual->address);
                // cfgline2("%s:   load = MAIN,     type = ro,  optional = yes, start = $%4.4x;", actual->name, actual->address);
                // outhead1(".segment \"%s\"", actual->name);
                Variable * variable = _environment->variables;
                
                variable_cleanup_entry( _environment, variable );
                
                variable_cleanup_entry_bit( _environment, variable );

            } else if ( actual->type == BT_TEMPORARY ) {
                // cfgline3("# BANK %s %s AT $%4.4x", BANK_TYPE_AS_STRING[actual->type], actual->name, actual->address);
                // cfgline2("%s:   load = MAIN,     type = ro,  optional = yes, start = $%4.4x;", actual->name, actual->address);
                // outhead1(".segment \"%s\"", actual->name);
                if ( _environment->bitmaskNeeded ) {
                    outhead0("BITMASK: .byte $01,$02,$04,$08,$10,$20,$40,$80");
                    outhead0("BITMASKN: .byte $fe,$fd,$fb,$f7,$ef,$df,$bf,$7f");
                }
                if ( _environment->deployed.dstring ) {
                    outhead1("max_free_string = $%4.4x", _environment->dstring.space == 0 ? DSTRING_DEFAULT_SPACE : _environment->dstring.space );
                }

                for( int j=0; j< (_environment->currentProcedure+1); ++j ) {
                    Variable * variable = _environment->tempVariables[j];
                    variable_cleanup_entry( _environment, variable );
                    variable_cleanup_entry_bit( _environment, variable );
                }
                
                Variable * variable = _environment->tempResidentVariables;

                variable_cleanup_entry( _environment, variable );
                variable_cleanup_entry_bit( _environment, variable );

            // } else if ( actual->type == BT_STRINGS ) {
                // cfgline3("# BANK %s %s AT $%4.4x", BANK_TYPE_AS_STRING[actual->type], actual->name, actual->address);
                // cfgline2("%s:   load = MAIN,     type = ro,  optional = yes, start = $%4.4x;", actual->name, actual->address);
            } else {

            }
           actual = actual->next;
        }
    }    

    if ( _environment->descriptors ) {
        outhead0("UDCCHAR:" );
        int i=0,j=0;
        for(i=0;i<_environment->descriptors->count;++i) {
            outline1("; $%2.2x ", i);
            out0(".byte " );
            for(j=0;j<7;++j) {
                out1("$%2.2x,", ((unsigned char)_environment->descriptors->data[_environment->descriptors->first+i].data[j]) );
            }
            outline1("$%2.2x", ((unsigned char)_environment->descriptors->data[_environment->descriptors->first+i].data[j]) );
        }
        outhead0(".segment \"CODE\"" );
        outhead0("GTIAUDCCHAR:" );
        char startAddress[MAX_TEMPORARY_STORAGE]; sprintf( startAddress, "$B000+$%4.4x", ( _environment->descriptors->first * 8) );
        cpu6502_mem_move_direct_size( _environment, "UDCCHAR", startAddress, _environment->descriptors->count*8 );
        outline0("LDA #$B0" );
        outline0("STA $2F4" );
        outline0("RTS" );
    } else {
        outhead0("GTIAUDCCHAR:" );
        outline0("RTS" );
    }

    if ( _environment->memoryAreas ) {
        MemoryArea * memoryArea = _environment->memoryAreas;
        while( memoryArea ) {
            // if ( memoryArea->type == MAT_RAM ) {
            //     cfgline3("MA%3.3x:  load = RAM%3.3x, type = overwrite,  optional = yes, start = $%4.4x;", memoryArea->id, memoryArea->id, memoryArea->start);
            // } else {
            //     cfgline2("MA%3.3x:  load = MAIN, type = overwrite,  optional = yes, start = $%4.4x;", memoryArea->id, memoryArea->start);
            // }
            outhead1(".segment \"MA%3.3x\"", memoryArea->id );
            for( i=memoryArea->start; i<memoryArea->end; ++i ) {
                Variable * variable = _environment->variables;
                while( variable ) {
                    if ( !variable->assigned && variable->memoryArea == memoryArea && variable->absoluteAddress == i ) {
                        variable_cleanup_memory_mapped( _environment, variable );
                        variable->staticalInit = ( memoryArea->type == MAT_RAM ? 0 : 1 );
                        break;
                    }
                    variable = variable->next;
                }
                for( int j=0; j< (_environment->currentProcedure+1); ++j ) {
                    Variable * variable = _environment->tempVariables[j];
                    while( variable ) {
                        if ( !variable->assigned && variable->memoryArea == memoryArea && variable->absoluteAddress == i ) {
                            variable_cleanup_memory_mapped( _environment, variable );
                            variable->staticalInit = ( memoryArea->type == MAT_RAM ? 0 : 1 );
                            break;
                        }
                        variable = variable->next;
                    }
                }
                variable = _environment->tempResidentVariables;
                while( variable ) {
                    if ( !variable->assigned && variable->memoryArea == memoryArea && variable->absoluteAddress == i ) {
                        variable_cleanup_memory_mapped( _environment, variable );
                        variable->staticalInit = ( memoryArea->type == MAT_RAM ? 0 : 1 );
                        break;
                    }
                    variable = variable->next;
                }
            }
            memoryArea = memoryArea->next;
        }

    }    

    outhead0(".segment \"CODE\"" );
    
    variable_on_memory_init( _environment, 0 );

    DataSegment * dataSegment = _environment->dataSegment;
    while( dataSegment ) {
        int i=0;
        out1("%s: .BYTE ", dataSegment->realName );
        DataDataSegment * dataDataSegment = dataSegment->data;
        while( dataDataSegment ) {
            if ( dataSegment->type ) {
                if ( dataDataSegment->type == VT_STRING || dataDataSegment->type == VT_DSTRING ) {
                    out1("$%2.2x,", (unsigned char)(dataDataSegment->size) );
                    out1("\"%s\"", dataDataSegment->data );
                } else {
                    for( i=0; i<(dataDataSegment->size-1); ++i ) {
                        out1("$%2.2x,", (unsigned char)(dataDataSegment->data[i]&0xff) );
                    }
                    out1("$%2.2x", (unsigned char)(dataDataSegment->data[i]&0xff) );
                }
            } else {
                if ( dataDataSegment->type == VT_STRING || dataDataSegment->type == VT_DSTRING ) {
                    out1("$%2.2x,", (unsigned char)(dataDataSegment->type) );
                    out1("$%2.2x,", (unsigned char)(dataDataSegment->size) );
                    out1("\"%s\"", dataDataSegment->data );
                } else {
                    out1("$%2.2x,", (unsigned char)(dataDataSegment->type) );
                    for( i=0; i<(dataDataSegment->size-1); ++i ) {
                        out1("$%2.2x,", (unsigned char)(dataDataSegment->data[i]&0xff) );
                    }
                    out1("$%2.2x", (unsigned char)(dataDataSegment->data[i]&0xff) );
                }
            }
            dataDataSegment = dataDataSegment->next;
            if ( dataDataSegment ) {
                out0(",");
            }
        }
        outline0("");
        dataSegment = dataSegment->next;
    }
    outhead0("DATAPTRE:");

    StaticString * staticStrings = _environment->strings;
    while( staticStrings ) {
        outline3("cstring%d: .byte %d, %s", staticStrings->id, (int)strlen(staticStrings->value), escape_newlines( staticStrings->value ) );
        staticStrings = staticStrings->next;
    }

}
