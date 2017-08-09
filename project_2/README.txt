Vaggelis Tsiatouras
1115201200185
Nikos Sofras
1115201200168

Shmeia pou mporei na mperdepsoun ton exetasth:

H logikh ths splitLeafBlock einai diaforetikh apo thn splitIndexBlock.
Dhladh h splitLeafBlock perimenei na eisax8ei to record pou den xwraei
gia na kanei split, enw h splitIndexBlock kanei split molis gemisei to
block. Auto den ephreazei thn swsth ektelesh tou programmatos, einai apla
mia paradoxh.

Entolh metaglwttishs(x64): gcc -o db main1.c AM.c AM.h BF.h BF_64.a defn.h index.h
Entolh metaglwttishs(x86): gcc -o db main1.c AM.c AM.h BF.h BF_32.a defn.h index.h