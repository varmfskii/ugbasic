/*****************************************************************************
 * ugBASIC - an isomorphic BASIC language compiler for retrocomputers        *
 *****************************************************************************
 * Copyright 2021-2023 Marco Spedaletti (asimov@mclink.it)
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

/**
 * @brief Emit code for <strong>YIELD</strong>
 * 
 * @param _environment Current calling environment
 */
/* <usermanual>
@keyword YIELD

@english
This keyword will suspend the current thread, waiting for the
next slice of time. 

@italian
Questa parola chiave sospende il thread corrente, in attesa
di un nuovo quanto di tempo.

@syntax YIELD

@example YIELD

@target all
</usermanual> */
void yield( Environment * _environment ) {

    if ( _environment->anyProtothread ) {

        char protothreadLabel[MAX_TEMPORARY_STORAGE]; sprintf(protothreadLabel, "%spt%d", _environment->procedureName, _environment->protothreadStep );

        cpu_protothread_save( _environment, "PROTOTHREADCT", _environment->protothreadStep );
        cpu_protothread_set_state( _environment, "PROTOTHREADCT", PROTOTHREAD_STATUS_YIELDED );

        cpu_return( _environment );
        cpu_label( _environment, protothreadLabel );
        cpu_protothread_set_state( _environment, "PROTOTHREADCT", PROTOTHREAD_STATUS_RUNNING );

        ++_environment->protothreadStep;

    }

}

