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
 * "COSì COM'è", SENZA GARANZIE O CONDIZIONI DI ALCUN TIPO, esplicite o
 * implicite. Consultare la Licenza per il testo specifico che regola le
 * autorizzazioni e le limitazioni previste dalla medesima.
 ****************************************************************************/

/****************************************************************************
 * INCLUDE SECTION 
 ****************************************************************************/

#ifdef __c64__

#include "../ugbc.h"

/****************************************************************************
 * CODE SECTION
 ****************************************************************************/

void c64_xpen( Environment * _environment, char * _destination ) {

    MAKE_LABEL

    outline0("LDA $D013");    
    outline0("ASL" );
    outline1("STA %s", _destination);
    outline1("BCC %s", label );
    outline0("LDA #1");    
    outline1("STA _%s", address_displacement(_environment, _destination, "1"));
    outhead1("%s:", label );

}

void c64_ypen( Environment * _environment, char * _destination ) {

    MAKE_LABEL

    outline0("LDA $d014");
    outline1("STA %s", _destination);    
    outline0("LDA #0");
    outline1("STA %s", address_displacement(_environment, _destination, "1"));    
   
}

void c64_inkey( Environment * _environment, char * _pressed, char * _key ) {

    deploy( scancode, src_hw_c64_scancode_asm);

    MAKE_LABEL

    outline0("LDA #$0");
    outline1("STA %s", _pressed );
    outline0("LDA #$0");
    outline1("STA %s", _key );

    outline0("JSR SCANCODE");

    outline0("LDX $c6");
    outline0("CPX #$0");
    outline1("BEQ %snokey", label );

    outline0("LDA $0277" );
    outline0("CMP #$FF");
    outline1("BEQ %snopetscii", label );
    outline1("STA %s", _key );
    outline0("LDA #$FF");
    outline1("STA %s", _pressed );

    outline0("LDX #0");
    outhead1("%sclkeys:", label);
    outline0("LDA $0278,X" );
    outline0("LDA $0277,X" );
    outline0("INX");
    outline0("CPX $c6");
    outline1("BNE %sclkeys", label);
    outline0("DEC $c6");

    outline1("JMP %snokey", label );

    outhead1("%snopetscii:", label );
    outline0("LDA #0");
    outline1("STA %s", _key );
    outhead1("%snokey:", label );
   
}

void c64_scancode( Environment * _environment, char * _pressed, char * _scancode ) {

    deploy( scancode, src_hw_c64_scancode_asm);

    MAKE_LABEL

    outline0("LDA #$0");
    outline1("STA %s", _pressed );
    outline0("LDA #$0");
    outline1("STA %s", _scancode );

    outline0("JSR SCANCODE");

    outline0("LDY $c5");
    outline0("CPY #$40");
    outline1("BEQ %snokey", label );

    outline1("STY %s", _scancode );
    outline0("LDA #$ff");
    outline1("STA %s", _pressed );

    outhead1("%snokey:", label );
   
}

void c64_key_pressed( Environment * _environment, char *_scancode, char * _result ) {

    MAKE_LABEL

    char nokeyLabel[MAX_TEMPORARY_STORAGE];
    sprintf( nokeyLabel, "%slabel", label );
    
    Variable * temp = variable_temporary( _environment, VT_BYTE, "(pressed)" );

    c64_scancode( _environment, temp->realName, _result );
    cpu_compare_8bit( _environment, _result, _scancode,  temp->realName, 1 );
    cpu_compare_and_branch_8bit_const( _environment, temp->realName, 0, nokeyLabel, 1 );
    cpu_store_8bit( _environment, _result, 0xff );
    cpu_jump( _environment, label );
    cpu_label( _environment, nokeyLabel );
    cpu_store_8bit( _environment, _result, 0x00 );
    cpu_label( _environment, label );

}

void c64_scanshift( Environment * _environment, char * _shifts ) {

    MAKE_LABEL

    outline0("LDA #0");
    outline1("STA %s", _shifts);
    outline0("LDA #$10");
    outline0("STA $DC00");
    outline0("LDA $DC01");
    outline0("AND #$80");
    outline1("BNE %snoleft", label);
    outline0("LDA #1");
    outline1("STA %s", _shifts);
    outhead1("%snoleft:", label );

    outline0("LDA #$20");
    outline0("STA $DC00");
    outline0("LDA $DC01");
    outline0("AND #$10");
    outline1("BNE %snoright", label);
    outline1("LDA %s", _shifts);
    outline0("ORA #2");
    outline1("STA %s", _shifts);
    outhead1("%snoright:", label );

}

void c64_keyshift( Environment * _environment, char * _shifts ) {

    deploy( scancode, src_hw_c64_scancode_asm);

    MAKE_LABEL

    outline0("JSR SCANCODE");

    outline0("LDA #0");
    outline1("STA %s", _shifts);
    outline0("LDA #$10");
    outline0("STA $DC00");
    outline0("LDA $DC01");
    outline0("AND #$80");
    outline1("BNE %snoleft", label);
    outline0("LDA #1");
    outline1("STA %s", _shifts);
    outhead1("%snoleft:", label );

    outline0("LDA #$20");
    outline0("STA $DC00");
    outline0("LDA $DC01");
    outline0("AND #$10");
    outline1("BNE %snoright", label);
    outline1("LDA %s", _shifts);
    outline0("ORA #2");
    outline1("STA %s", _shifts);
    outhead1("%snoright:", label );

    outline0("LDA $028D");
    outline0("AND #$01");
    outline1("BEQ %snocaps", label);
    outline1("LDA %s", _shifts);
    outline0("ORA #4");
    outline1("STA %s", _shifts);
    outhead1("%snocaps:", label );

    outline0("LDA $028D");
    outline0("AND #$04");
    outline1("BEQ %snocontrol", label);
    outline1("LDA %s", _shifts);
    outline0("ORA #8");
    outline1("STA %s", _shifts);
    outhead1("%snocontrol:", label );

    outline0("LDA $028D");
    outline0("AND #$02");
    outline1("BEQ %snoalt", label);
    outline1("LDA %s", _shifts);
    outline0("ORA #$30");
    outline1("STA %s", _shifts);
    outhead1("%snoalt:", label );

}

void c64_clear_key( Environment * _environment ) {

    outline0("LDA #$0");
    outline0("STA $c6");
   
}

void c64_dload( Environment * _environment, char * _filename, char * _offset, char * _address, char * _size ) {

    deploy( dload, src_hw_c64_dload_asm);

    MAKE_LABEL
    
    Variable * filename = variable_retrieve( _environment, _filename );
    Variable * tnaddress = variable_temporary( _environment, VT_ADDRESS, "(address of target_name)");
    Variable * tnsize = variable_temporary( _environment, VT_BYTE, "(size of target_name)");

    Variable * address = NULL;
    if ( _address ) {
        address = variable_retrieve( _environment, _address );
    }
    Variable * size = NULL;
    if ( _size ) {
        size = variable_retrieve( _environment, _size );
    }

    switch( filename->type ) {
        case VT_STRING:
            cpu_move_8bit( _environment, filename->realName, tnsize->realName );
            cpu_addressof_16bit( _environment, filename->realName, tnaddress->realName );
            cpu_inc_16bit( _environment, tnaddress->realName );
            break;
        case VT_DSTRING:
            cpu_dsdescriptor( _environment, filename->realName, tnaddress->realName, tnsize->realName );
            break;
    }

    outline1("LDA %s", tnaddress->realName);
    outline0("STA TMPPTR");
    outline1("LDA %s", address_displacement(_environment, tnaddress->realName, "1"));
    outline0("STA TMPPTR+1");
    outline1("LDA %s", tnsize->realName);
    outline0("STA MATHPTR0");

    if ( address ) {

        outline1("LDA %s", address->realName);
        outline0("STA TMPPTR2");
        outline1("LDA %s", address_displacement(_environment, address->realName, "1"));
        outline0("STA TMPPTR2+1");
        outline0("LDA #0");
        outline0("STA MATHPTR1");

    }

    outline0("JSR C64DLOAD");

}

void c64_dsave( Environment * _environment, char * _filename, char * _offset, char * _address, char * _size ) {

    deploy( dsave, src_hw_c64_dsave_asm);

    MAKE_LABEL
    
    Variable * filename = variable_retrieve( _environment, _filename );
    Variable * tnaddress = variable_temporary( _environment, VT_ADDRESS, "(address of target_name)");
    Variable * tnsize = variable_temporary( _environment, VT_BYTE, "(size of target_name)");

    Variable * address = NULL;
    if ( _address ) {
        address = variable_retrieve( _environment, _address );
    }
    Variable * size = NULL;
    if ( _size ) {
        size = variable_retrieve( _environment, _size );
    }

    switch( filename->type ) {
        case VT_STRING:
            cpu_move_8bit( _environment, filename->realName, tnsize->realName );
            cpu_addressof_16bit( _environment, filename->realName, tnaddress->realName );
            cpu_inc_16bit( _environment, tnaddress->realName );
            break;
        case VT_DSTRING:
            cpu_dsdescriptor( _environment, filename->realName, tnaddress->realName, tnsize->realName );
            break;
    }

    outline1("LDA %s", tnaddress->realName);
    outline0("STA TMPPTR");
    outline1("LDA %s", address_displacement(_environment, tnaddress->realName, "1"));
    outline0("STA TMPPTR+1");
    outline1("LDA %s", tnsize->realName);
    outline0("STA MATHPTR0");

    if ( address ) {

        outline1("LDA %s", address->realName);
        outline0("STA TMPPTR2");
        outline1("LDA %s", address_displacement(_environment, address->realName, "1"));
        outline0("STA TMPPTR2+1");
        outline0("LDA #0");
        outline0("STA MATHPTR1");

    }

    if ( size ) {

        outline1("LDA %s", size->realName);
        outline0("STA MATHPTR4");
        outline1("LDA %s", address_displacement(_environment, size->realName, "1"));
        outline0("STA MATHPTR5");

    } else {

        outline0("LDA #$00");
        outline0("STA MATHPTR4");
        outline0("STA MATHPTR5");

    }

    outline0("JSR C64DSAVE");

}

void c64_sys_call( Environment * _environment, int _destination ) {

    outline0("PHA");
    outline1("LDA #$%2.2x", (_destination & 0xff ) );
    outline0("STA SYSCALL0+1");
    outline1("LDA #$%2.2x", ((_destination>>8) & 0xff ) );
    outline0("STA SYSCALL0+2");
    outline0("PLA");
    outline0("JSR SYSCALL");

}

void c64_timer_set_status_on( Environment * _environment, char * _timer ) {
    
    deploy( timer, src_hw_6502_timer_asm);

    if ( _timer ) {
        outline1("LDX %s", _timer );
    } else {
        outline0("LDX #0" );
    }
    outline0("LDY #$1" );
    outline0("JSR TIMERSETSTATUS" );

}

void c64_timer_set_status_off( Environment * _environment, char * _timer ) {

    deploy( timer, src_hw_6502_timer_asm);

    if ( _timer ) {
        outline1("LDX %s", _timer );
    } else {
        outline0("LDX #0" );
    }
    outline0("LDY #$0" );
    outline0("JSR TIMERSETSTATUS" );

}

void c64_timer_set_counter( Environment * _environment, char * _timer, char * _counter ) {

    deploy( timer, src_hw_6502_timer_asm);

    if ( _timer ) {
        outline1("LDX %s", _timer );
    } else {
        outline0("LDX #0" );
    }
    if ( _counter ) {
        outline1("LDA %s", _counter );
    } else {
        outline0("LDA #0" );
    }
    outline0("STA MATHPTR2");
    if ( _counter ) {
        outline1("LDA %s", address_displacement( _environment, _counter, "1" ) );
    }
    outline0("STA MATHPTR3");
    outline0("JSR TIMERSETCOUNTER" );

}

void c64_timer_set_init( Environment * _environment, char * _timer, char * _init ) {

    deploy( timer, src_hw_6502_timer_asm);

    if ( _timer ) {
        outline1("LDX %s", _timer );
    } else {
        outline0("LDX #0" );
    }
    outline1("LDA %s", _init );
    outline0("STA MATHPTR2");
    outline1("LDA %s", address_displacement( _environment, _init, "1" ) );
    outline0("STA MATHPTR3");
    outline0("JSR TIMERSETINIT" );

}

void c64_timer_set_address( Environment * _environment, char * _timer, char * _address ) {

    deploy( timer, src_hw_6502_timer_asm);

    if ( _timer ) {
        outline1("LDX %s", _timer );
    } else {
        outline0("LDX #0" );
    }
    outline1("LDA #<%s", _address );
    outline0("STA MATHPTR2");
    outline1("LDA #>%s", _address );
    outline0("STA MATHPTR3");
    outline0("JSR TIMERSETADDRESS" );

}

#endif