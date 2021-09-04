; /*****************************************************************************
;  * ugBASIC - an isomorphic BASIC language compiler for retrocomputers        *
;  *****************************************************************************
;  * Copyright 2021 Marco Spedaletti (asimov@mclink.it)
;  *
;  * Licensed under the Apache License, Version 2.0 (the "License");
;  * you may not use this file except in compliance with the License.
;  * You may obtain a copy of the License at
;  *
;  * http://www.apache.org/licenses/LICENSE-2.0
;  *
;  * Unless required by applicable law or agreed to in writing, software
;  * distributed under the License is distributed on an "AS IS" BASIS,
;  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;  * See the License for the specific language governing permissions and
;  * limitations under the License.
;  *----------------------------------------------------------------------------
;  * Concesso in licenza secondo i termini della Licenza Apache, versione 2.0
;  * (la "Licenza"); è proibito usare questo file se non in conformità alla
;  * Licenza. Una copia della Licenza è disponibile all'indirizzo:
;  *
;  * http://www.apache.org/licenses/LICENSE-2.0
;  *
;  * Se non richiesto dalla legislazione vigente o concordato per iscritto,
;  * il software distribuito nei termini della Licenza è distribuito
;  * "COSì COM'è", SENZA GARANZIE O CONDIZIONI DI ALCUN TIPO, esplicite o
;  * implicite. Consultare la Licenza per il testo specifico che regola le
;  * autorizzazioni e le limitazioni previste dalla medesima.
;  ****************************************************************************/
;* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
;*                                                                             *
;*                          VERTICAL SCROLL ON 6847                            *
;*                                                                             *
;*                             by Marco Spedaletti                             *
;*                                                                             *
;* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

VSCROLLT
    PSHS D,X,Y,U
    LDA DIRECTION
    CMPA #0
    BGT VSCROLLTDOWN

VSCROLLTUP
    LDX TEXTADDRESS
    LDY TEXTADDRESS
    STY COPYOFTEXTADDRESS
    LEAY 32, Y
    STY COPYOFTEXTADDRESS2

    LDU #2

VSCROLLTUPYSCR
    LDB #0
VSCROLLTUPYSCR1
    LDA B,Y
    STA B,X
    INCB
    CMPB #$7F
    BNE VSCROLLTUPYSCR1
    LEAX 127, X
    LEAX 1, X
    LEAY 127, Y
    LEAY 1, Y    

    LDB #0
VSCROLLTUPYSCR2
    LDA B,Y
    STA B,X
    INCB
    CMPB #$7F
    BNE VSCROLLTUPYSCR2
    LEAX 127, X
    LEAX 1, X
    LEAY 127, Y
    LEAY 1, Y    

    LEAU -1, U
    CMPU #0
    BNE VSCROLLTUPYSCR

;     LDY #192
; VSCROLLTUPREFILL:
;     LDA #32
;     STA (COPYOFTEXTADDRESS),Y
;     INY
;     CPY #232
;     BNE VSCROLLTUPREFILL
; VSCROLLTUEND:

    JMP VSCROLLTE

VSCROLLTDOWN
    LDX TEXTADDRESS
    LDY TEXTADDRESS
    STY COPYOFTEXTADDRESS
    LEAY 32, Y
    STY COPYOFTEXTADDRESS2

    LEAX 127, X
    LEAX 1, X
    LEAX 127, X
    LEAX 1, X
    LEAY 127, Y
    LEAY 1, Y    
    LEAY 127, Y
    LEAY 1, Y    

    LEAX 127, X
    LEAX 1, X
    LEAX 127, X
    LEAX 1, X
    LEAY 127, Y
    LEAY 1, Y    
    LEAY 127, Y
    LEAY 1, Y    

    LDB #$7F
VSCROLLTDOWNYS31
    LDA B,X
    STA B,Y
    DECB
    CMPB #0
    BNE VSCROLLTDOWNYS31
    LEAX -127, X
    LEAX -1, X
    LEAY -127, Y
    LEAY -1, Y

    LDB #$7F
VSCROLLTDOWNYS32
    LDA B,X
    STA B,Y
    DECB
    CMPB #0
    BNE VSCROLLTDOWNYS32
    LEAX -127, X
    LEAX -1, X
    LEAY -127, Y
    LEAY -1, Y

;     LDY #0
; SCROLLFILLUP:
;     LDA EMPTYTILE
;     STA (COPYOFTEXTADDRESS),Y
;     INY
;     CPY #40
;     BNE SCROLLFILLUP

    JMP VSCROLLTE

VSCROLLTE
    PULS D,X,Y,U
    RTS