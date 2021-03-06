; Show that we know how to translate converting signed integer to double.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   --reg-use=s20 | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 --reg-use=s20  | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   --reg-use=s20 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 --reg-use=s20 | FileCheck %s --check-prefix=DIS

define internal double @SignedIntToDouble() {
; ASM-LABEL: SignedIntToDouble:
; DIS-LABEL: 00000000 <SignedIntToDouble>:
; IASM-LABEL: SignedIntToDouble:

entry:
; ASM: .LSignedIntToDouble$entry:
; IASM: .LSignedIntToDouble$entry:

  %v = sitofp i32 17 to double

; ASM:  vcvt.f64.s32    d0, s20
; DIS:   10:   eeb80bca
; IASM-NOT: vcvt

  ret double %v
}
