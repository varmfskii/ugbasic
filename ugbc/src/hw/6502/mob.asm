; /*****************************************************************************
;  * ugBASIC - an isomorphic BASIC language compiler for retrocomputers        *
;  *****************************************************************************
;  * Copyright 2021 Marco Spedaletti (asimov@mclink.it)
;  *
;  * Licensed under the Apache License, Version 2.0 (the "License");
;  * you may not use this file eXcept in compliance with the License.
;  * You may obtain a copy of the License at
;  *
;  * http://www.apache.org/licenses/LICENSE-2.0
;  *
;  * Unless required by applicable law or agreed to in writing, software
;  * distributed under the License is distributed on an "AS IS" BASIS,
;  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either eXpress or implied.
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
;*                  MOVABLE OBJECTS UNDER 6502 (generic algorithms)            *
;*                                                                             *
;*                             by Marco Spedaletti                             *
;*                                                                             *
;* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

MOB_COUNT = 16

MOBI = $23
MOBX = $24
MOBY = $26
MOBW = $28
MOBH = $29
MOBADDR = $32
MOBSIZE = $34
MOBLASTX = $35

; Generic initialization
; MOBINIT(X:indeX,X,y,w,h,draw)
MOBINIT:

    STX MOBI
    
    ; Initialize status
    LDA #$0
    STA MOBDESCRIPTORS_S,X

    ; Initialize position
    LDA MOBX
    STA MOBDESCRIPTORS_XL,X
    STA MOBDESCRIPTORS_PXL,X
    LDA MOBX+1
    STA MOBDESCRIPTORS_XH,X
    STA MOBDESCRIPTORS_PXH,X
    LDA MOBY
    STA MOBDESCRIPTORS_YL,X
    STA MOBDESCRIPTORS_PYL,X
    LDA MOBY+1
    STA MOBDESCRIPTORS_YH,X
    STA MOBDESCRIPTORS_PYH,X

    ; Save address of the given data.
    ; Note that specific chipset 
    ; initialization can easily override this.
    LDA MOBADDR
    STA MOBDESCRIPTORS_DL,X
    LDA MOBADDR+1
    STA MOBDESCRIPTORS_DH,X

    ; Initialize to 0 the space for saving
    ; background (again, this is a chipset specific
    ; initialization routine).
    LDA #$0
    STA MOBDESCRIPTORS_SL,X
    STA MOBDESCRIPTORS_SH,X

    ; Initialize the chipset specific part
    JSR MOBINITCS

    RTS

; MOBSHOW(X:indeX)
MOBSHOW:
    LDA MOBDESCRIPTORS_S,X
    ORA #$01
    STA MOBDESCRIPTORS_S,X
    RTS

; MOBHIDE(X:indeX)
MOBHIDE:
    LDA MOBDESCRIPTORS_S,X
    AND #$FE
    STA MOBDESCRIPTORS_S,X
    RTS

; MOBSAVE(X:indeX) -> chipset
; MOBRESTORE(X:indeX) -> chipset
; MOBDRAW(X:indeX) -> chipset

MOBADJUST:
    LDA MOBDESCRIPTORS_S,X
    AND #$01
    BEQ MOBADJUSTN
    LDA MOBDESCRIPTORS_S,X
    ORA #$03
    STA MOBDESCRIPTORS_S,X
    RTS
MOBADJUSTN:
    LDA MOBDESCRIPTORS_S,X
    AND #$FC
    STA MOBDESCRIPTORS_S,X
    RTS

; MOBAT(X:indeX, X, y)
MOBAT:
    LDA MOBX
    STA MOBDESCRIPTORS_XL,X
    CMP MOBDESCRIPTORS_PXL,X
    BEQ MOBAT2
    LDA MOBDESCRIPTORS_S,X
    ORA #$40
MOBAT2:
    LDA MOBX+1
    STA MOBDESCRIPTORS_XH,X
    CMP MOBDESCRIPTORS_PXH,X
    BEQ MOBAT3
    LDA MOBDESCRIPTORS_S,X
    ORA #$40
MOBAT3:
    LDA MOBY
    STA MOBDESCRIPTORS_YL,X
    CMP MOBDESCRIPTORS_PXL,X
    BEQ MOBAT4
    LDA MOBDESCRIPTORS_S,X
    ORA #$80
MOBAT4:
    LDA MOBY+1
    STA MOBDESCRIPTORS_YH,X
    STA MOBDESCRIPTORS_PYH,X
    BEQ MOBAT5
    LDA MOBDESCRIPTORS_S,X
    ORA #$80
MOBAT5:
    RTS

MOBALLOC:
    CLC
    LDA MOBADDRESS
    ADC MOBALLOCATED
    STA MOBADDR
    LDA MOBADDRESS+1
    ADC MOBALLOCATED+1
    STA MOBADDR+1

    CLC
    LDA MOBALLOCATED
    ADC MOBSIZE
    STA MOBALLOCATED
    LDA MOBALLOCATED+1
    ADC #0
    STA MOBALLOCATED+1

    RTS

MOBFREE:
    SEC
    LDA MOBALLOCATED
    SBC MOBSIZE
    STA MOBALLOCATED
    LDA MOBALLOCATED+1
    SBC #0
    STA MOBALLOCATED+1
    RTS

MOBDESCRIPTORS_S:
    .RES    MOB_COUNT
MOBDESCRIPTORS_XL:
    .RES    MOB_COUNT
MOBDESCRIPTORS_XH:
    .RES    MOB_COUNT
MOBDESCRIPTORS_YL:
    .RES    MOB_COUNT
MOBDESCRIPTORS_YH:
    .RES    MOB_COUNT
MOBDESCRIPTORS_PXL:
    .RES    MOB_COUNT
MOBDESCRIPTORS_PXH:
    .RES    MOB_COUNT
MOBDESCRIPTORS_PYL:
    .RES    MOB_COUNT
MOBDESCRIPTORS_PYH:
    .RES    MOB_COUNT
MOBDESCRIPTORS_W:
    .RES    MOB_COUNT
MOBDESCRIPTORS_H:
    .RES    MOB_COUNT
MOBDESCRIPTORS_DL:
    .RES    MOB_COUNT
MOBDESCRIPTORS_DH:
    .RES    MOB_COUNT
MOBDESCRIPTORS_SL:
    .RES    MOB_COUNT
MOBDESCRIPTORS_SH:
    .RES    MOB_COUNT

MOBALLOCATED:       .WORD   $0



MOBRENDERER:
    ; X = 0
    LDX #0

MOBRENDERERL1:
    ; take descriptor X
    LDA MOBDESCRIPTORS_S,X

    ; moved + visibled? = $C1
    ; moved + unvisibled? = $C2
    CMP #$C1
    BEQ MOBRENDERERV1
    CMP #$C2
    BEQ MOBRENDERERV1

    ; ++X
    INX

    ; X < N ?
    CPX MOB_COUNT
    BNE MOBRENDERERL1
    RTS

MOBRENDERERV1:
    ; LASTX = X
    STX MOBLASTX

    ; X = N - 1
    LDX MOB_COUNT
    DEX

MOBRENDERERL2:
    ; previously visible?
    LDA MOBDESCRIPTORS_S, X
    AND #$02
    BEQ MOBRENDERERV2

    ; restore background at pX,py (w,h) save area
    JSR MOBRESTORE

    ; adjust visibility flag
    JSR MOBADJUST

    ; update positions
    LDA MOBDESCRIPTORS_XL, X
    STA MOBDESCRIPTORS_PXL, X
    LDA MOBDESCRIPTORS_XH, X
    STA MOBDESCRIPTORS_PXH, X
    LDA MOBDESCRIPTORS_YL, X
    STA MOBDESCRIPTORS_PYL, X
    LDA MOBDESCRIPTORS_YH, X
    STA MOBDESCRIPTORS_PYH, X
    
MOBRENDERERV2:
    ; --X
    DEX

    ; X >= LASTX
    CPX MOBLASTX
    BCS MOBRENDERERL2
    BEQ MOBRENDERERL2

    ; Reset the save area to LAST X

MOBRENDERERV3:
MOBRENDERERL3:
    ; visible ?
    LDA MOBDESCRIPTORS_S, X
    AND #$01
    BEQ MOBRENDERERV4

    ; save background at X,y (w,h) to save area
    JSR MOBSAVE

    ; draw sprite at X,y (w,h) from draw area
    JSR MOBDRAW

    ; adjust visibility flag
    JSR MOBADJUST

MOBRENDERERV4:
    ; ++X
    INX

    ; X < N ?
    CPX MOB_COUNT
    BCC MOBRENDERERL3

    RTS