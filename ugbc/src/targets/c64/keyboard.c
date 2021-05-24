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

/****************************************************************************
 * INCLUDE SECTION 
 ****************************************************************************/

#include "../../ugbc.h"

/****************************************************************************
 * CODE SECTION 
 ****************************************************************************/

Variable * inkey( Environment * _environment ) {

    Variable * result = variable_temporary( _environment, VT_STRING, "(result of INKEY$)");

    char resultString[MAX_TEMPORARY_STORAGE]; sprintf( resultString, " " );
    char stringAddress[MAX_TEMPORARY_STORAGE]; sprintf( stringAddress, "%s+1", result->realName );

    variable_store_string(_environment, result->name, resultString );

    MAKE_LABEL

    Variable * pressed = variable_temporary( _environment, VT_BYTE, "(key pressed?)");
    Variable * key = variable_temporary( _environment, VT_BYTE, "(key pressed)");

    char noKeyPressedLabel[MAX_TEMPORARY_STORAGE]; sprintf(noKeyPressedLabel, "%snokeyPressed", label );
    char finishedLabel[MAX_TEMPORARY_STORAGE]; sprintf(finishedLabel, "%sfinished", label );

    c64_inkey( _environment, pressed->realName, key->realName );

    cpu_bveq( _environment, pressed->realName, noKeyPressedLabel );

    cpu_move_8bit_indirect_with_offset(_environment, key->realName, stringAddress, 0 );

    cpu_jump( _environment, finishedLabel );

    cpu_label( _environment, noKeyPressedLabel );

    cpu_store_8bit(_environment, result->realName, 0 );

    cpu_label( _environment, finishedLabel );
    
    return result;

}

Variable * scancode( Environment * _environment ) {

    Variable * result = variable_temporary( _environment, VT_BYTE, "(result of SCANCODE)");

    Variable * pressed = variable_temporary( _environment, VT_BYTE, "(key pressed?)");

    c64_scancode( _environment, pressed->realName, result->realName );

    return result;

}

Variable * scanshift( Environment * _environment ) {

    Variable * result = variable_temporary( _environment, VT_BYTE, "(result of SCANSHIFT)");

    c64_scanshift( _environment, result->realName );

    return result;

}

Variable * key_state( Environment * _environment, char * _scancode ) {

    Variable * s = variable_retrieve_or_define( _environment, _scancode, VT_BYTE, 0 );

    Variable * result = variable_temporary( _environment, VT_BYTE, "(result of KEY STATE)");

    Variable * key = scancode( _environment );

    return variable_compare( _environment, s->name, key->name );

}
