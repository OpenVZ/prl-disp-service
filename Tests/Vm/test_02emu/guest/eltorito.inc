ORG 0x7c00

cli
jmp 0x0000:boot

times 8-($-$$) db 0

bi_PrimaryVolumeDescriptor  resd  1    ; LBA of the Primary Volume Descriptor
bi_BootFileLocation         resd  1    ; LBA of the Boot File
bi_BootFileLength           resd  1    ; Length of the boot file in bytes
bi_Checksum                 resd  1    ; 32 bit checksum
bi_Reserved                 resb  40   ; Reserved 'for future standardization'

boot:
