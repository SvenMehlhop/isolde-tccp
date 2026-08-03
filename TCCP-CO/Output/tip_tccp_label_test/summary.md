# General Information
freq=10MHz
period=100ns
# legend
## Observer
ID(platon_hw)=0

## Label
0x80000000=_start
0x80000020=B
0x80000028=A
0x80000034=block_3

## Contract type
0: Periodic
1: Reaction
2: Aging
3: TSBC time
4: TSBC PC

# Table Readable
| ID  | Monitor | Observer | Event/ Val a      | Event/ Val b      | ValInt Start | ValInt Stop  |
|:---:|:-------:|:--------:|:-----------------:|:-----------------:|:------------:|:------------:|
| 1 | 0 | 0 | 0x80000028 A | 0 (noLabel) | 10000000ns/100000cycles | 20000000ns/200000cycles |
| 2 | 1 | 0 | 0x80000028 A | 0x80000020 B | 0 | 3000000ns/30000cycles |
| 3 | 2 | 0 | 0x80000028 A | 0x80000020 B | 0 | 5000000ns/50000cycles |
| 4 | 4 | 0 | 0 | 30 | 2147483688 | 2147483680 |

# Table HEX
| ID  | Monitor | Observer | Event/ Val a      | Event/ Val b      | ValInt Start | ValInt Stop  |
|:---:|:-------:|:--------:|:-----------------:|:-----------------:|:------------:|:------------:|
| 1 | 0 | 0 | 0x80000028 A | 0x0 (noLabel) | 0x186a0 | 0x30d40 |
| 2 | 1 | 0 | 0x80000028 A | 0x80000020 B | 0x0 | 0x7530 |
| 3 | 2 | 0 | 0x80000028 A | 0x80000020 B | 0x0 | 0xc350 |
| 4 | 4 | 0 | 0x0 | 0x1e | 0x80000028 | 0x80000020 |
