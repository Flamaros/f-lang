// This file is generated by f-asm_nasm_db_to_code program

#include "ASM_x64.hpp"

#include "../ASM.hpp"

namespace f::ASM
{
    size_t g_instruction_desc_table_indices[(size_t)Instruction::COUNT + 1] = {
        0,	// UNKNOWN
        0,	// AAA
        1,	// AAD
        3,	// AAM
        5,	// AAS
        6,	// ADC
        45,	// ADD
        84,	// AND
        123,	// ARPL
        125,	// BOUND
        127,	// BSF
        133,	// BSR
        139,	// BT
        148,	// BTC
        157,	// BTR
        166,	// BTS
        175,	// CALL
        183,	// CBW
        184,	// CDQ
        185,	// CDQE
        186,	// CLC
        187,	// CLD
        188,	// CLI
        189,	// CLTS
        190,	// CMC
        191,	// CMP
        230,	// CMPSB
        231,	// CMPSD
        232,	// CMPSQ
        233,	// CMPSW
        236,	// CMPXCHG486
        242,	// CMPXCHG16B
        243,	// CQO
        244,	// CWD
        245,	// CWDE
        246,	// DAA
        247,	// DAS
        248,	// DEC
        252,	// DIV
        256,	// ENTER
        257,	// F2XM1
        258,	// FABS
        259,	// FADD
        262,	// FADDP
        263,	// FBLD
        265,	// FBSTP
        267,	// FCHS
        268,	// FCLEX
        269,	// FCOM
        272,	// FCOMP
        275,	// FCOMPP
        276,	// FCOS
        277,	// FDECSTP
        278,	// FDISI
        279,	// FDIV
        282,	// FDIVP
        283,	// FDIVR
        286,	// FDIVRP
        287,	// FENI
        288,	// FFREE
        289,	// FFREEP
        290,	// FIADD
        292,	// FICOM
        294,	// FICOMP
        296,	// FIDIV
        298,	// FIDIVR
        300,	// FILD
        303,	// FIMUL
        305,	// FINCSTP
        306,	// FINIT
        307,	// FIST
        309,	// FISTP
        312,	// FISUB
        314,	// FISUBR
        316,	// FLD
        320,	// FLD1
        321,	// FLDCW
        322,	// FLDENV
        323,	// FLDL2E
        324,	// FLDL2T
        325,	// FLDLG2
        326,	// FLDLN2
        327,	// FLDPI
        328,	// FLDZ
        329,	// FMUL
        332,	// FMULP
        333,	// FNCLEX
        334,	// FNDISI
        335,	// FNENI
        336,	// FNINIT
        337,	// FNOP
        338,	// FNSAVE
        339,	// FNSTCW
        340,	// FNSTENV
        341,	// FNSTSW
        343,	// FPATAN
        344,	// FPREM
        345,	// FPREM1
        346,	// FPTAN
        347,	// FRNDINT
        348,	// FRSTOR
        349,	// FSAVE
        350,	// FSCALE
        351,	// FSETPM
        352,	// FSIN
        353,	// FSINCOS
        354,	// FSQRT
        355,	// FST
        358,	// FSTCW
        359,	// FSTENV
        360,	// FSTP
        364,	// FSTSW
        366,	// FSUB
        369,	// FSUBP
        370,	// FSUBR
        373,	// FSUBRP
        374,	// FTST
        375,	// FUCOM
        376,	// FUCOMP
        377,	// FUCOMPP
        378,	// FXAM
        379,	// FXCH
        380,	// FXTRACT
        381,	// FYL2X
        382,	// FYL2XP1
        383,	// HLT
        384,	// IBTS
        388,	// ICEBP
        389,	// IDIV
        393,	// IMUL
        427,	// IN
        433,	// INC
        437,	// INSB
        438,	// INSD
        439,	// INSW
        440,	// INT
        441,	// INT01
        442,	// INT1
        443,	// INT03
        444,	// INT3
        445,	// INTO
        446,	// INVD
        447,	// INVLPG
        449,	// IRET
        450,	// IRETD
        451,	// IRETQ
        452,	// IRETW
        453,	// JCXZ
        454,	// JECXZ
        455,	// JRCXZ
        456,	// JMP
        465,	// LAHF
        466,	// LAR
        478,	// LDS
        480,	// LEA
        486,	// LEAVE
        487,	// LES
        489,	// LFENCE
        490,	// LFS
        493,	// LGDT
        494,	// LGS
        497,	// LIDT
        498,	// LLDT
        501,	// LMSW
        504,	// LOADALL
        505,	// LOADALL286
        506,	// LODSB
        507,	// LODSD
        508,	// LODSQ
        509,	// LODSW
        510,	// LOOP
        514,	// LOOPE
        518,	// LOOPNE
        522,	// LOOPNZ
        526,	// LOOPZ
        530,	// LSL
        542,	// LSS
        545,	// LTR
        548,	// MFENCE
        551,	// MOV
        608,	// MOVSB
        609,	// MOVSD
        610,	// MOVSQ
        611,	// MOVSW
        612,	// MOVSX
        618,	// MOVSXD
        619,	// MOVSX
        620,	// MOVZX
        626,	// MUL
        630,	// NEG
        634,	// NOP
        636,	// NOT
        640,	// OR
        679,	// OUT
        685,	// OUTSB
        686,	// OUTSD
        687,	// OUTSW
        688,	// PAUSE
        689,	// POP
        698,	// POPA
        699,	// POPAD
        700,	// POPAW
        701,	// POPF
        702,	// POPFD
        703,	// POPFQ
        704,	// POPFW
        705,	// PUSH
        725,	// PUSHA
        726,	// PUSHAD
        727,	// PUSHAW
        728,	// PUSHF
        729,	// PUSHFD
        730,	// PUSHFQ
        731,	// PUSHFW
        732,	// RCL
        744,	// RCR
        756,	// RET
        758,	// RETF
        760,	// RETN
        762,	// RETW
        764,	// RETFW
        766,	// RETNW
        768,	// RETD
        770,	// RETFD
        772,	// RETND
        774,	// RETQ
        776,	// RETFQ
        778,	// RETNQ
        780,	// ROL
        792,	// ROR
        804,	// RSDC
        805,	// RSLDT
        806,	// RSTS
        807,	// SAHF
        808,	// SAL
        820,	// SALC
        821,	// SAR
        833,	// SBB
        872,	// SCASB
        873,	// SCASD
        874,	// SCASQ
        875,	// SCASW
        876,	// SFENCE
        877,	// SGDT
        878,	// SHL
        890,	// SHLD
        902,	// SHR
        914,	// SHRD
        926,	// SIDT
        927,	// SLDT
        933,	// SKINIT
        934,	// SMI
        935,	// SMINTOLD
        936,	// SMSW
        941,	// STC
        942,	// STD
        943,	// STI
        944,	// STOSB
        945,	// STOSD
        946,	// STOSQ
        947,	// STOSW
        948,	// STR
        953,	// SUB
        992,	// SVDC
        993,	// SVLDT
        994,	// SVTS
        995,	// SWAPGS
        996,	// TEST
        1019,	// UD0
        1023,	// UD1
        1027,	// UD2B
        1031,	// UD2
        1032,	// UD2A
        1033,	// UMOV
        1045,	// VERR
        1048,	// VERW
        1051,	// FWAIT
        1052,	// WBINVD
        1053,	// XADD
        1061,	// XBTS
        1065,	// XCHG
        1082,	// XLATB
        1083,	// XLAT
        1084,	// XOR
        1128,	// FXRSTOR64
        1129,	// FXSAVE64
        1146,	// PEXTRQ
        1148,	// PINSRQ
        1153,	// RDPKRU
        1154,	// WRPKRU
        1162,	// INCSSPQ
        1163,	// RDSSPQ
        1164,	// WRUSSQ
        1165,	// WRSSQ
    };


    Instruction_Desc g_instruction_desc_table[] = {
        // UNKNOWN

        AAA void
        AAD void
        AAD imm
        AAM void
        AAM imm
        AAS void
        ADC mem,reg8
        ADC reg8,reg8
        ADC mem,reg16
        ADC reg16,reg16
        ADC mem,reg32
        ADC reg32,reg32
        ADC mem,reg64
        ADC reg64,reg64
        ADC reg8,mem
        ADC reg8,reg8
        ADC reg16,mem
        ADC reg16,reg16
        ADC reg32,mem
        ADC reg32,reg32
        ADC reg64,mem
        ADC reg64,reg64
        ADC rm16,imm8
        ADC rm32,imm8
        ADC rm64,imm8
        ADC reg_al,imm
        ADC reg_ax,sbyteword
        ADC reg_ax,imm
        ADC reg_eax,sbytedword
        ADC reg_eax,imm
        ADC reg_rax,sbytedword
        ADC reg_rax,imm
        ADC rm8,imm
        ADC rm16,sbyteword
        ADC rm16,imm
        ADC rm32,sbytedword
        ADC rm32,imm
        ADC rm64,sbytedword
        ADC rm64,imm
        ADC mem,imm8
        ADC mem,sbyteword16
        ADC mem,imm16
        ADC mem,sbytedword32
        ADC mem,imm32
        ADC rm8,imm
        ADD mem,reg8
        ADD reg8,reg8
        ADD mem,reg16
        ADD reg16,reg16
        ADD mem,reg32
        ADD reg32,reg32
        ADD mem,reg64
        ADD reg64,reg64
        ADD reg8,mem
        ADD reg8,reg8
        ADD reg16,mem
        ADD reg16,reg16
        ADD reg32,mem
        ADD reg32,reg32
        ADD reg64,mem
        ADD reg64,reg64
        ADD rm16,imm8
        ADD rm32,imm8
        ADD rm64,imm8
        ADD reg_al,imm
        ADD reg_ax,sbyteword
        ADD reg_ax,imm
        ADD reg_eax,sbytedword
        ADD reg_eax,imm
        ADD reg_rax,sbytedword
        ADD reg_rax,imm
        ADD rm8,imm
        ADD rm16,sbyteword
        ADD rm16,imm
        ADD rm32,sbytedword
        ADD rm32,imm
        ADD rm64,sbytedword
        ADD rm64,imm
        ADD mem,imm8
        ADD mem,sbyteword16
        ADD mem,imm16
        ADD mem,sbytedword32
        ADD mem,imm32
        ADD rm8,imm
        AND mem,reg8
        AND reg8,reg8
        AND mem,reg16
        AND reg16,reg16
        AND mem,reg32
        AND reg32,reg32
        AND mem,reg64
        AND reg64,reg64
        AND reg8,mem
        AND reg8,reg8
        AND reg16,mem
        AND reg16,reg16
        AND reg32,mem
        AND reg32,reg32
        AND reg64,mem
        AND reg64,reg64
        AND rm16,imm8
        AND rm32,imm8
        AND rm64,imm8
        AND reg_al,imm
        AND reg_ax,sbyteword
        AND reg_ax,imm
        AND reg_eax,sbytedword
        AND reg_eax,imm
        AND reg_rax,sbytedword
        AND reg_rax,imm
        AND rm8,imm
        AND rm16,sbyteword
        AND rm16,imm
        AND rm32,sbytedword
        AND rm32,imm
        AND rm64,sbytedword
        AND rm64,imm
        AND mem,imm8
        AND mem,sbyteword16
        AND mem,imm16
        AND mem,sbytedword32
        AND mem,imm32
        AND rm8,imm
        ARPL mem,reg16
        ARPL reg16,reg16
        BOUND reg16,mem
        BOUND reg32,mem
        BSF reg16,mem
        BSF reg16,reg16
        BSF reg32,mem
        BSF reg32,reg32
        BSF reg64,mem
        BSF reg64,reg64
        BSR reg16,mem
        BSR reg16,reg16
        BSR reg32,mem
        BSR reg32,reg32
        BSR reg64,mem
        BSR reg64,reg64
        BT mem,reg16
        BT reg16,reg16
        BT mem,reg32
        BT reg32,reg32
        BT mem,reg64
        BT reg64,reg64
        BT rm16,imm
        BT rm32,imm
        BT rm64,imm
        BTC mem,reg16
        BTC reg16,reg16
        BTC mem,reg32
        BTC reg32,reg32
        BTC mem,reg64
        BTC reg64,reg64
        BTC rm16,imm
        BTC rm32,imm
        BTC rm64,imm
        BTR mem,reg16
        BTR reg16,reg16
        BTR mem,reg32
        BTR reg32,reg32
        BTR mem,reg64
        BTR reg64,reg64
        BTR rm16,imm
        BTR rm32,imm
        BTR rm64,imm
        BTS mem,reg16
        BTS reg16,reg16
        BTS mem,reg32
        BTS reg32,reg32
        BTS mem,reg64
        BTS reg64,reg64
        BTS rm16,imm
        BTS rm32,imm
        BTS rm64,imm
        CALL imm
        // Call/jmp near imm/reg/mem is always 64-bit in long mode.
        CALL imm16
        CALL imm32
        CALL imm64
        CALL mem
        CALL rm16
        CALL rm32
        CALL rm64
        CBW void
        CDQ void
        CDQE void
        CLC void
        CLD void
        CLI void
        CLTS void
        CMC void
        CMP mem,reg8
        CMP reg8,reg8
        CMP mem,reg16
        CMP reg16,reg16
        CMP mem,reg32
        CMP reg32,reg32
        CMP mem,reg64
        CMP reg64,reg64
        CMP reg8,mem
        CMP reg8,reg8
        CMP reg16,mem
        CMP reg16,reg16
        CMP reg32,mem
        CMP reg32,reg32
        CMP reg64,mem
        CMP reg64,reg64
        CMP rm16,imm8
        CMP rm32,imm8
        CMP rm64,imm8
        CMP reg_al,imm
        CMP reg_ax,sbyteword
        CMP reg_ax,imm
        CMP reg_eax,sbytedword
        CMP reg_eax,imm
        CMP reg_rax,sbytedword
        CMP reg_rax,imm
        CMP rm8,imm
        CMP rm16,sbyteword
        CMP rm16,imm
        CMP rm32,sbytedword
        CMP rm32,imm
        CMP rm64,sbytedword
        CMP rm64,imm
        CMP mem,imm8
        CMP mem,sbyteword16
        CMP mem,imm16
        CMP mem,sbytedword32
        CMP mem,imm32
        CMP rm8,imm
        CMPSB void
        CMPSD void
        CMPSQ void
        CMPSW void
        CMPXCHG mem,reg64
        CMPXCHG reg64,reg64
        CMPXCHG486 mem,reg8
        CMPXCHG486 reg8,reg8
        CMPXCHG486 mem,reg16
        CMPXCHG486 reg16,reg16
        CMPXCHG486 mem,reg32
        CMPXCHG486 reg32,reg32
        CMPXCHG16B mem128
        CQO void
        CWD void
        CWDE void
        DAA void
        DAS void
        DEC rm8
        DEC rm16
        DEC rm32
        DEC rm64
        DIV rm8
        DIV rm16
        DIV rm32
        DIV rm64
        ENTER imm,imm
        F2XM1 void
        FABS void
        FADD mem32
        FADD mem64
        FADD void
        FADDP void
        FBLD mem80
        FBLD mem
        FBSTP mem80
        FBSTP mem
        FCHS void
        FCLEX void
        FCOM mem32
        FCOM mem64
        FCOM void
        FCOMP mem32
        FCOMP mem64
        FCOMP void
        FCOMPP void
        FCOS void
        FDECSTP void
        FDISI void
        FDIV mem32
        FDIV mem64
        FDIV void
        FDIVP void
        FDIVR mem32
        FDIVR mem64
        FDIVR void
        FDIVRP void
        FENI void
        FFREE void
        FFREEP void
        FIADD mem32
        FIADD mem16
        FICOM mem32
        FICOM mem16
        FICOMP mem32
        FICOMP mem16
        FIDIV mem32
        FIDIV mem16
        FIDIVR mem32
        FIDIVR mem16
        FILD mem32
        FILD mem16
        FILD mem64
        FIMUL mem32
        FIMUL mem16
        FINCSTP void
        FINIT void
        FIST mem32
        FIST mem16
        FISTP mem32
        FISTP mem16
        FISTP mem64
        FISUB mem32
        FISUB mem16
        FISUBR mem32
        FISUBR mem16
        FLD mem32
        FLD mem64
        FLD mem80
        FLD void
        FLD1 void
        FLDCW mem
        FLDENV mem
        FLDL2E void
        FLDL2T void
        FLDLG2 void
        FLDLN2 void
        FLDPI void
        FLDZ void
        FMUL mem32
        FMUL mem64
        FMUL void
        FMULP void
        FNCLEX void
        FNDISI void
        FNENI void
        FNINIT void
        FNOP void
        FNSAVE mem
        FNSTCW mem
        FNSTENV mem
        FNSTSW mem
        FNSTSW reg_ax
        FPATAN void
        FPREM void
        FPREM1 void
        FPTAN void
        FRNDINT void
        FRSTOR mem
        FSAVE mem
        FSCALE void
        FSETPM void
        FSIN void
        FSINCOS void
        FSQRT void
        FST mem32
        FST mem64
        FST void
        FSTCW mem
        FSTENV mem
        FSTP mem32
        FSTP mem64
        FSTP mem80
        FSTP void
        FSTSW mem
        FSTSW reg_ax
        FSUB mem32
        FSUB mem64
        FSUB void
        FSUBP void
        FSUBR mem32
        FSUBR mem64
        FSUBR void
        FSUBRP void
        FTST void
        FUCOM void
        FUCOMP void
        FUCOMPP void
        FXAM void
        FXCH void
        FXTRACT void
        FYL2X void
        FYL2XP1 void
        HLT void
        IBTS mem,reg16
        IBTS reg16,reg16
        IBTS mem,reg32
        IBTS reg32,reg32
        ICEBP void
        IDIV rm8
        IDIV rm16
        IDIV rm32
        IDIV rm64
        IMUL rm8
        IMUL rm16
        IMUL rm32
        IMUL rm64
        IMUL reg16,mem
        IMUL reg16,reg16
        IMUL reg32,mem
        IMUL reg32,reg32
        IMUL reg64,mem
        IMUL reg64,reg64
        IMUL reg16,mem,imm8
        IMUL reg16,mem,sbyteword
        IMUL reg16,mem,imm16
        IMUL reg16,mem,imm
        IMUL reg16,reg16,imm8
        IMUL reg16,reg16,sbyteword
        IMUL reg16,reg16,imm16
        IMUL reg16,reg16,imm
        IMUL reg32,mem,imm8
        IMUL reg32,mem,sbytedword
        IMUL reg32,mem,imm32
        IMUL reg32,mem,imm
        IMUL reg32,reg32,imm8
        IMUL reg32,reg32,sbytedword
        IMUL reg32,reg32,imm32
        IMUL reg32,reg32,imm
        IMUL reg64,mem,imm8
        IMUL reg64,mem,sbytedword
        IMUL reg64,mem,imm32
        IMUL reg64,mem,imm
        IMUL reg64,reg64,imm8
        IMUL reg64,reg64,sbytedword
        IMUL reg64,reg64,imm32
        IMUL reg64,reg64,imm
        IN reg_al,imm
        IN reg_ax,imm
        IN reg_eax,imm
        IN reg_al,reg_dx
        IN reg_ax,reg_dx
        IN reg_eax,reg_dx
        INC rm8
        INC rm16
        INC rm32
        INC rm64
        INSB void
        INSD void
        INSW void
        INT imm
        INT01 void
        INT1 void
        INT03 void
        INT3 void
        INTO void
        INVD void
        INVLPG mem
        INVLPGA reg_rax,reg_ecx
        IRET void
        IRETD void
        IRETQ void
        IRETW void
        JCXZ imm
        JECXZ imm
        JRCXZ imm
        JMP imm
        JMP imm
        // Call/jmp near imm/reg/mem is always 64-bit in long mode.
        JMP imm16
        JMP imm32
        JMP imm64
        JMP mem
        JMP rm16
        JMP rm32
        JMP rm64
        LAHF void
        LAR reg16,mem
        LAR reg16,reg16
        LAR reg16,reg32
        LAR reg16,reg64
        LAR reg32,mem
        LAR reg32,reg16
        LAR reg32,reg32
        LAR reg32,reg64
        LAR reg64,mem
        LAR reg64,reg16
        LAR reg64,reg32
        LAR reg64,reg64
        LDS reg16,mem
        LDS reg32,mem
        LEA reg16,mem
        LEA reg32,mem
        LEA reg64,mem
        LEA reg16,imm
        LEA reg32,imm
        LEA reg64,imm
        LEAVE void
        LES reg16,mem
        LES reg32,mem
        LFENCE void
        LFS reg16,mem
        LFS reg32,mem
        LFS reg64,mem
        LGDT mem
        LGS reg16,mem
        LGS reg32,mem
        LGS reg64,mem
        LIDT mem
        LLDT mem
        LLDT mem16
        LLDT reg16
        LMSW mem
        LMSW mem16
        LMSW reg16
        LOADALL void
        LOADALL286 void
        LODSB void
        LODSD void
        LODSQ void
        LODSW void
        LOOP imm
        LOOP imm,reg_cx
        LOOP imm,reg_ecx
        LOOP imm,reg_rcx
        LOOPE imm
        LOOPE imm,reg_cx
        LOOPE imm,reg_ecx
        LOOPE imm,reg_rcx
        LOOPNE imm
        LOOPNE imm,reg_cx
        LOOPNE imm,reg_ecx
        LOOPNE imm,reg_rcx
        LOOPNZ imm
        LOOPNZ imm,reg_cx
        LOOPNZ imm,reg_ecx
        LOOPNZ imm,reg_rcx
        LOOPZ imm
        LOOPZ imm,reg_cx
        LOOPZ imm,reg_ecx
        LOOPZ imm,reg_rcx
        LSL reg16,mem
        LSL reg16,reg16
        LSL reg16,reg32
        LSL reg16,reg64
        LSL reg32,mem
        LSL reg32,reg16
        LSL reg32,reg32
        LSL reg32,reg64
        LSL reg64,mem
        LSL reg64,reg16
        LSL reg64,reg32
        LSL reg64,reg64
        LSS reg16,mem
        LSS reg32,mem
        LSS reg64,mem
        LTR mem
        LTR mem16
        LTR reg16
        MFENCE void
        MONITOR reg_rax,reg_ecx,reg_edx
        MONITORX reg_rax,reg_ecx,reg_edx
        MOV mem,reg_sreg
        MOV reg16,reg_sreg
        MOV reg32,reg_sreg
        MOV reg64,reg_sreg
        MOV rm64,reg_sreg
        MOV reg_sreg,mem
        MOV reg_sreg,reg16
        MOV reg_sreg,reg32
        MOV reg_sreg,reg64
        MOV reg_sreg,reg16
        MOV reg_sreg,reg32
        MOV reg_sreg,rm64
        MOV reg_al,mem_offs
        MOV reg_ax,mem_offs
        MOV reg_eax,mem_offs
        MOV reg_rax,mem_offs
        MOV mem_offs,reg_al
        MOV mem_offs,reg_ax
        MOV mem_offs,reg_eax
        MOV mem_offs,reg_rax
        MOV reg64,reg_creg
        MOV reg_creg,reg64
        MOV reg32,reg_dreg
        MOV reg64,reg_dreg
        MOV reg_dreg,reg32
        MOV reg_dreg,reg64
        MOV reg32,reg_treg
        MOV reg_treg,reg32
        MOV mem,reg8
        MOV reg8,reg8
        MOV mem,reg16
        MOV reg16,reg16
        MOV mem,reg32
        MOV reg32,reg32
        MOV mem,reg64
        MOV reg64,reg64
        MOV reg8,mem
        MOV reg8,reg8
        MOV reg16,mem
        MOV reg16,reg16
        MOV reg32,mem
        MOV reg32,reg32
        MOV reg64,mem
        MOV reg64,reg64
        MOV reg64,sdword
        MOV rm8,imm
        MOV rm16,imm
        MOV rm32,imm
        MOV rm64,imm
        MOV rm64,imm32
        MOV mem,imm8
        MOV mem,imm16
        MOV mem,imm32
        MOVD mmxreg,rm64
        MOVD rm64,mmxreg
        MOVQ mmxreg,rm64
        MOVQ rm64,mmxreg
        MOVSB void
        MOVSD void
        MOVSQ void
        MOVSW void
        MOVSX reg16,mem
        MOVSX reg16,reg8
        MOVSX reg32,rm8
        MOVSX reg32,rm16
        MOVSX reg64,rm8
        MOVSX reg64,rm16
        MOVSXD reg64,rm32
        MOVSX reg64,rm32
        MOVZX reg16,mem
        MOVZX reg16,reg8
        MOVZX reg32,rm8
        MOVZX reg32,rm16
        MOVZX reg64,rm8
        MOVZX reg64,rm16
        MUL rm8
        MUL rm16
        MUL rm32
        MUL rm64
        NEG rm8
        NEG rm16
        NEG rm32
        NEG rm64
        NOP void
        NOP rm64
        NOT rm8
        NOT rm16
        NOT rm32
        NOT rm64
        OR mem,reg8
        OR reg8,reg8
        OR mem,reg16
        OR reg16,reg16
        OR mem,reg32
        OR reg32,reg32
        OR mem,reg64
        OR reg64,reg64
        OR reg8,mem
        OR reg8,reg8
        OR reg16,mem
        OR reg16,reg16
        OR reg32,mem
        OR reg32,reg32
        OR reg64,mem
        OR reg64,reg64
        OR rm16,imm8
        OR rm32,imm8
        OR rm64,imm8
        OR reg_al,imm
        OR reg_ax,sbyteword
        OR reg_ax,imm
        OR reg_eax,sbytedword
        OR reg_eax,imm
        OR reg_rax,sbytedword
        OR reg_rax,imm
        OR rm8,imm
        OR rm16,sbyteword
        OR rm16,imm
        OR rm32,sbytedword
        OR rm32,imm
        OR rm64,sbytedword
        OR rm64,imm
        OR mem,imm8
        OR mem,sbyteword16
        OR mem,imm16
        OR mem,sbytedword32
        OR mem,imm32
        OR rm8,imm
        OUT imm,reg_al
        OUT imm,reg_ax
        OUT imm,reg_eax
        OUT reg_dx,reg_al
        OUT reg_dx,reg_ax
        OUT reg_dx,reg_eax
        OUTSB void
        OUTSD void
        OUTSW void
        PAUSE void
        POP rm16
        POP rm32
        POP rm64
        POP reg_es
        POP reg_cs
        POP reg_ss
        POP reg_ds
        POP reg_fs
        POP reg_gs
        POPA void
        POPAD void
        POPAW void
        POPF void
        POPFD void
        POPFQ void
        POPFW void
        PUSH rm16
        PUSH rm32
        PUSH rm64
        PUSH reg_es
        PUSH reg_cs
        PUSH reg_ss
        PUSH reg_ds
        PUSH reg_fs
        PUSH reg_gs
        PUSH imm8
        PUSH sbyteword16
        PUSH imm16
        PUSH sbytedword32
        PUSH imm32
        PUSH sbytedword32
        PUSH imm32
        PUSH sbytedword64
        PUSH imm64
        PUSH sbytedword32
        PUSH imm32
        PUSHA void
        PUSHAD void
        PUSHAW void
        PUSHF void
        PUSHFD void
        PUSHFQ void
        PUSHFW void
        RCL rm8,unity
        RCL rm8,reg_cl
        RCL rm8,imm8
        RCL rm16,unity
        RCL rm16,reg_cl
        RCL rm16,imm8
        RCL rm32,unity
        RCL rm32,reg_cl
        RCL rm32,imm8
        RCL rm64,unity
        RCL rm64,reg_cl
        RCL rm64,imm8
        RCR rm8,unity
        RCR rm8,reg_cl
        RCR rm8,imm8
        RCR rm16,unity
        RCR rm16,reg_cl
        RCR rm16,imm8
        RCR rm32,unity
        RCR rm32,reg_cl
        RCR rm32,imm8
        RCR rm64,unity
        RCR rm64,reg_cl
        RCR rm64,imm8
        RET void
        RET imm
        RETF void
        RETF imm
        RETN void
        RETN imm
        RETW void
        RETW imm
        RETFW void
        RETFW imm
        RETNW void
        RETNW imm
        RETD void
        RETD imm
        RETFD void
        RETFD imm
        RETND void
        RETND imm
        RETQ void
        RETQ imm
        RETFQ void
        RETFQ imm
        RETNQ void
        RETNQ imm
        ROL rm8,unity
        ROL rm8,reg_cl
        ROL rm8,imm8
        ROL rm16,unity
        ROL rm16,reg_cl
        ROL rm16,imm8
        ROL rm32,unity
        ROL rm32,reg_cl
        ROL rm32,imm8
        ROL rm64,unity
        ROL rm64,reg_cl
        ROL rm64,imm8
        ROR rm8,unity
        ROR rm8,reg_cl
        ROR rm8,imm8
        ROR rm16,unity
        ROR rm16,reg_cl
        ROR rm16,imm8
        ROR rm32,unity
        ROR rm32,reg_cl
        ROR rm32,imm8
        ROR rm64,unity
        ROR rm64,reg_cl
        ROR rm64,imm8
        RSDC reg_sreg,mem80
        RSLDT mem80
        RSTS mem80
        SAHF void
        SAL rm8,unity
        SAL rm8,reg_cl
        SAL rm8,imm8
        SAL rm16,unity
        SAL rm16,reg_cl
        SAL rm16,imm8
        SAL rm32,unity
        SAL rm32,reg_cl
        SAL rm32,imm8
        SAL rm64,unity
        SAL rm64,reg_cl
        SAL rm64,imm8
        SALC void
        SAR rm8,unity
        SAR rm8,reg_cl
        SAR rm8,imm8
        SAR rm16,unity
        SAR rm16,reg_cl
        SAR rm16,imm8
        SAR rm32,unity
        SAR rm32,reg_cl
        SAR rm32,imm8
        SAR rm64,unity
        SAR rm64,reg_cl
        SAR rm64,imm8
        SBB mem,reg8
        SBB reg8,reg8
        SBB mem,reg16
        SBB reg16,reg16
        SBB mem,reg32
        SBB reg32,reg32
        SBB mem,reg64
        SBB reg64,reg64
        SBB reg8,mem
        SBB reg8,reg8
        SBB reg16,mem
        SBB reg16,reg16
        SBB reg32,mem
        SBB reg32,reg32
        SBB reg64,mem
        SBB reg64,reg64
        SBB rm16,imm8
        SBB rm32,imm8
        SBB rm64,imm8
        SBB reg_al,imm
        SBB reg_ax,sbyteword
        SBB reg_ax,imm
        SBB reg_eax,sbytedword
        SBB reg_eax,imm
        SBB reg_rax,sbytedword
        SBB reg_rax,imm
        SBB rm8,imm
        SBB rm16,sbyteword
        SBB rm16,imm
        SBB rm32,sbytedword
        SBB rm32,imm
        SBB rm64,sbytedword
        SBB rm64,imm
        SBB mem,imm8
        SBB mem,sbyteword16
        SBB mem,imm16
        SBB mem,sbytedword32
        SBB mem,imm32
        SBB rm8,imm
        SCASB void
        SCASD void
        SCASQ void
        SCASW void
        SFENCE void
        SGDT mem
        SHL rm8,unity
        SHL rm8,reg_cl
        SHL rm8,imm8
        SHL rm16,unity
        SHL rm16,reg_cl
        SHL rm16,imm8
        SHL rm32,unity
        SHL rm32,reg_cl
        SHL rm32,imm8
        SHL rm64,unity
        SHL rm64,reg_cl
        SHL rm64,imm8
        SHLD mem,reg16,imm
        SHLD reg16,reg16,imm
        SHLD mem,reg32,imm
        SHLD reg32,reg32,imm
        SHLD mem,reg64,imm
        SHLD reg64,reg64,imm
        SHLD mem,reg16,reg_cl
        SHLD reg16,reg16,reg_cl
        SHLD mem,reg32,reg_cl
        SHLD reg32,reg32,reg_cl
        SHLD mem,reg64,reg_cl
        SHLD reg64,reg64,reg_cl
        SHR rm8,unity
        SHR rm8,reg_cl
        SHR rm8,imm8
        SHR rm16,unity
        SHR rm16,reg_cl
        SHR rm16,imm8
        SHR rm32,unity
        SHR rm32,reg_cl
        SHR rm32,imm8
        SHR rm64,unity
        SHR rm64,reg_cl
        SHR rm64,imm8
        SHRD mem,reg16,imm
        SHRD reg16,reg16,imm
        SHRD mem,reg32,imm
        SHRD reg32,reg32,imm
        SHRD mem,reg64,imm
        SHRD reg64,reg64,imm
        SHRD mem,reg16,reg_cl
        SHRD reg16,reg16,reg_cl
        SHRD mem,reg32,reg_cl
        SHRD reg32,reg32,reg_cl
        SHRD mem,reg64,reg_cl
        SHRD reg64,reg64,reg_cl
        SIDT mem
        SLDT mem
        SLDT mem16
        SLDT reg16
        SLDT reg32
        SLDT reg64
        SLDT reg64
        SKINIT void
        SMI void
        // Older Cyrix chips had this; they had to move due to conflict with MMX
        SMINTOLD void
        SMSW mem
        SMSW mem16
        SMSW reg16
        SMSW reg32
        SMSW reg64
        STC void
        STD void
        STI void
        STOSB void
        STOSD void
        STOSQ void
        STOSW void
        STR mem
        STR mem16
        STR reg16
        STR reg32
        STR reg64
        SUB mem,reg8
        SUB reg8,reg8
        SUB mem,reg16
        SUB reg16,reg16
        SUB mem,reg32
        SUB reg32,reg32
        SUB mem,reg64
        SUB reg64,reg64
        SUB reg8,mem
        SUB reg8,reg8
        SUB reg16,mem
        SUB reg16,reg16
        SUB reg32,mem
        SUB reg32,reg32
        SUB reg64,mem
        SUB reg64,reg64
        SUB rm16,imm8
        SUB rm32,imm8
        SUB rm64,imm8
        SUB reg_al,imm
        SUB reg_ax,sbyteword
        SUB reg_ax,imm
        SUB reg_eax,sbytedword
        SUB reg_eax,imm
        SUB reg_rax,sbytedword
        SUB reg_rax,imm
        SUB rm8,imm
        SUB rm16,sbyteword
        SUB rm16,imm
        SUB rm32,sbytedword
        SUB rm32,imm
        SUB rm64,sbytedword
        SUB rm64,imm
        SUB mem,imm8
        SUB mem,sbyteword16
        SUB mem,imm16
        SUB mem,sbytedword32
        SUB mem,imm32
        SUB rm8,imm
        SVDC mem80,reg_sreg
        SVLDT mem80
        SVTS mem80
        SWAPGS void
        TEST mem,reg8
        TEST reg8,reg8
        TEST mem,reg16
        TEST reg16,reg16
        TEST mem,reg32
        TEST reg32,reg32
        TEST mem,reg64
        TEST reg64,reg64
        TEST reg8,mem
        TEST reg16,mem
        TEST reg32,mem
        TEST reg64,mem
        TEST reg_al,imm
        TEST reg_ax,imm
        TEST reg_eax,imm
        TEST reg_rax,imm
        TEST rm8,imm
        TEST rm16,imm
        TEST rm32,imm
        TEST rm64,imm
        TEST mem,imm8
        TEST mem,imm16
        TEST mem,imm32
        UD0 void
        UD0 reg16,rm16
        UD0 reg32,rm32
        UD0 reg64,rm64
        UD1 reg16,rm16
        UD1 reg32,rm32
        UD1 reg64,rm64
        UD1 void
        UD2B void
        UD2B reg16,rm16
        UD2B reg32,rm32
        UD2B reg64,rm64
        UD2 void
        UD2A void
        UMOV mem,reg8
        UMOV reg8,reg8
        UMOV mem,reg16
        UMOV reg16,reg16
        UMOV mem,reg32
        UMOV reg32,reg32
        UMOV reg8,mem
        UMOV reg8,reg8
        UMOV reg16,mem
        UMOV reg16,reg16
        UMOV reg32,mem
        UMOV reg32,reg32
        VERR mem
        VERR mem16
        VERR reg16
        VERW mem
        VERW mem16
        VERW reg16
        FWAIT void
        WBINVD void
        XADD mem,reg8
        XADD reg8,reg8
        XADD mem,reg16
        XADD reg16,reg16
        XADD mem,reg32
        XADD reg32,reg32
        XADD mem,reg64
        XADD reg64,reg64
        XBTS reg16,mem
        XBTS reg16,reg16
        XBTS reg32,mem
        XBTS reg32,reg32
        // This must be NOLONG since opcode 90 is NOP, and in 64-bit mode
        // "xchg eax,eax" is *not* a NOP.
        XCHG reg_eax,reg_eax
        XCHG reg8,mem
        XCHG reg8,reg8
        XCHG reg16,mem
        XCHG reg16,reg16
        XCHG reg32,mem
        XCHG reg32,reg32
        XCHG reg64,mem
        XCHG reg64,reg64
        XCHG mem,reg8
        XCHG reg8,reg8
        XCHG mem,reg16
        XCHG reg16,reg16
        XCHG mem,reg32
        XCHG reg32,reg32
        XCHG mem,reg64
        XCHG reg64,reg64
        XLATB void
        XLAT void
        XOR mem,reg8
        XOR reg8,reg8
        XOR mem,reg16
        XOR reg16,reg16
        XOR mem,reg32
        XOR reg32,reg32
        XOR mem,reg64
        XOR reg64,reg64
        XOR reg8,mem
        XOR reg8,reg8
        XOR reg16,mem
        XOR reg16,reg16
        XOR reg32,mem
        XOR reg32,reg32
        XOR reg64,mem
        XOR reg64,reg64
        XOR rm16,imm8
        XOR rm32,imm8
        XOR rm64,imm8
        XOR reg_al,imm
        XOR reg_ax,sbyteword
        XOR reg_ax,imm
        XOR reg_eax,sbytedword
        XOR reg_eax,imm
        XOR reg_rax,sbytedword
        XOR reg_rax,imm
        XOR rm8,imm
        XOR rm16,sbyteword
        XOR rm16,imm
        XOR rm32,sbytedword
        XOR rm32,imm
        XOR rm64,sbytedword
        XOR rm64,imm
        XOR mem,imm8
        XOR mem,sbyteword16
        XOR mem,imm16
        XOR mem,sbytedword32
        XOR mem,imm32
        XOR rm8,imm
        // CMPPS/CMPSS must come after the specific ops; that way the disassembler will find the
        // specific ops first and only disassemble illegal ones as cmpps/cmpss.
        CVTSI2SS xmmreg,rm64
        CVTSS2SI reg64,xmmreg
        CVTSS2SI reg64,mem
        CVTTSS2SI reg64,xmmrm
        MOVMSKPS reg64,xmmreg
        FXRSTOR64 mem
        FXSAVE64 mem
        // Introduced in late Penryn ... we really need to clean up the handling
        // of CPU feature bits.
        // These instructions are not SSE-specific; they are
        // and work even if CR4.OSFXFR == 0
        // PINSRW is documented as using a reg32, but it's really using only 16 bit
        // -- accept either, but be truthful in disassembly
        // CLFLUSH needs its own feature flag implemented one day
        MOVNTI mem,reg64
        MOVQ xmmreg,rm64
        MOVQ rm64,xmmreg
        PEXTRW reg64,xmmreg,imm
        PINSRW xmmreg,reg64,imm
        // CMPPD/CMPSD must come after the specific ops; that way the disassembler will find the
        // specific ops first and only disassemble illegal ones as cmppd/cmpsd.
        CVTSD2SI reg64,xmmreg
        CVTSD2SI reg64,mem
        CVTSI2SD xmmreg,rm64
        CVTTSD2SI reg64,xmmreg
        CVTTSD2SI reg64,mem
        MOVMSKPD reg64,xmmreg
        VMREAD rm64,reg64
        VMWRITE reg64,rm64
        LZCNT reg64,rm64
        EXTRACTPS reg64,xmmreg,imm
        PEXTRB reg64,xmmreg,imm
        PEXTRQ rm64,xmmreg,imm
        PEXTRW reg64,xmmreg,imm
        PINSRQ xmmreg,mem,imm
        PINSRQ xmmreg,rm64,imm
        CRC32 reg64,rm8
        CRC32 reg64,rm64
        POPCNT reg64,rm64
        // Is NEHALEM right here?
        // Intel VAES instructions
        // Intel VAES + AVX512VL instructions
        // Intel VAES + AVX512F instructions
        // Specific aliases first, then the generic version, to keep the disassembler happy...
        // Specific aliases first, then the generic version, to keep the disassembler happy...
        // Specific aliases first, then the generic version, to keep the disassembler happy...
        // Specific aliases first, then the generic version, to keep the disassembler happy...
        // These are officially documented as VMOVDQA, but VMOVQQA seems more logical to me...
        // These are officially documented as VMOVDQU, but VMOVQQU seems more logical to me...
        // Officially VMOVNTDQ, but VMOVNTQQ seems more logical to me...
        // Intel VPCLMULQDQ instructions
        // Intel VPCLMULQDQ + AVX512VL instructions
        // Intel VPCLMULQDQ + AVX512F instructions
        //
        // Per AVX spec revision 7, document 319433-007
        // Per AVX spec revision 13, document 319433-013
        // Per AVX spec revision 14, document 319433-014
        //
        // based on pub number 43724 revision 3.04 date August 2009
        //
        // updated to match draft from AMD developer (patch has been
        // sent to binutils
        // 2010-03-22 Quentin Neill <quentin.neill@amd.com>
        //        Sebastian Pop  <sebastian.pop@amd.com>
        //
        //
        // based on pub number 43479 revision 3.04 dated November 2009
        //
        //
        // fixed: spec mention imm[7:4] though it should be /is4 even in spec
        //
        // fixed: spec mention only 3 operands in mnemonics
        //
        // fixed: spec point wrong VPCOMB in mnemonic
        //
        // fixed: spec has ymmreg for l0
        //
        // fixed: spec has VPHADDUBWD
        //
        // fixed: opcode db
        //
        // fixed: spec has ymmreg for l0
        //
        // fixed: spec has d7 opcode
        //
        // fixed: spec has 97,9f opcodes here
        //
        // fixed: spec point xmmreg instead of reg/mem
        //
        // fixed: spec error /r is needed
        //
        // fixed: spec error /r is needed
        //
        // fixed: spec has ymmreg for l0
        //
        // based on pub number 319433-011 dated July 2011
        //
        //
        // based on pub number 319433-011 dated July 2011
        //
        // MJC PUBLIC END
        RDPKRU void
        WRPKRU void
        RDPID reg64
        RDPID reg32
        // This one was killed before it saw the light of day
        // AMD Zen v1
        CLZERO reg_rax
        PTWRITE rm64
        MOVDIRI mem64,reg64
        MOVDIR64B reg64,mem512
        UMONITOR reg64
        INCSSPQ reg64
        RDSSPQ reg64
        WRUSSQ mem,reg64
        WRSSQ mem,reg64
        // These should be last in the file
        HINT_NOP0 rm64
        HINT_NOP1 rm64
        HINT_NOP2 rm64
        HINT_NOP3 rm64
        HINT_NOP4 rm64
        HINT_NOP5 rm64
        HINT_NOP6 rm64
        HINT_NOP7 rm64
        HINT_NOP8 rm64
        HINT_NOP9 rm64
        HINT_NOP10 rm64
        HINT_NOP11 rm64
        HINT_NOP12 rm64
        HINT_NOP13 rm64
        HINT_NOP14 rm64
        HINT_NOP15 rm64
        HINT_NOP16 rm64
        HINT_NOP17 rm64
        HINT_NOP18 rm64
        HINT_NOP19 rm64
        HINT_NOP20 rm64
        HINT_NOP21 rm64
        HINT_NOP22 rm64
        HINT_NOP23 rm64
        HINT_NOP24 rm64
        HINT_NOP25 rm64
        HINT_NOP26 rm64
        HINT_NOP27 rm64
        HINT_NOP28 rm64
        HINT_NOP29 rm64
        HINT_NOP30 rm64
        HINT_NOP31 rm64
        HINT_NOP32 rm64
        HINT_NOP33 rm64
        HINT_NOP34 rm64
        HINT_NOP35 rm64
        HINT_NOP36 rm64
        HINT_NOP37 rm64
        HINT_NOP38 rm64
        HINT_NOP39 rm64
        HINT_NOP40 rm64
        HINT_NOP41 rm64
        HINT_NOP42 rm64
        HINT_NOP43 rm64
        HINT_NOP44 rm64
        HINT_NOP45 rm64
        HINT_NOP46 rm64
        HINT_NOP47 rm64
        HINT_NOP48 rm64
        HINT_NOP49 rm64
        HINT_NOP50 rm64
        HINT_NOP51 rm64
        HINT_NOP52 rm64
        HINT_NOP53 rm64
        HINT_NOP54 rm64
        HINT_NOP55 rm64
        HINT_NOP56 rm64
        HINT_NOP57 rm64
        HINT_NOP58 rm64
        HINT_NOP59 rm64
        HINT_NOP60 rm64
        HINT_NOP61 rm64
        HINT_NOP62 rm64
        HINT_NOP63 rm64
    };


    Register_Desc	g_register_desc_table[(size_t)Register::COUNT] = {
        {Operand::Size::NONE, 0},	// UNKNOWN,

    };


}
