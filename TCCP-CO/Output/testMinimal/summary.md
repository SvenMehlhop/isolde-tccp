# General Information
freq=150MHz
period=6.67ns
# legend
## Observer
ID(ITI)=0
ID(TIP)=1
ID(CAM)=2

## Label

## Contract type
0: Periodic
1: Reaction
2: Aging
3: TSBC time
4: TSBC PC

# Table Readable
| ID  | Monitor | Observer | Event/ Val a      | Event/ Val b      | ValInt Start | ValInt Stop  |
|:---:|:-------:|:--------:|:-----------------:|:-----------------:|:------------:|:------------:|
| 1 | 0 | 0 | 0x80000018 (noLabel) | 0 (noLabel) | 5ns/1cycles | 20ns/3cycles |
| 2 | 1 | 0 | 0x80000028 (noLabel) | 0x80000030 (noLabel) | 4ns/1cycles | 16ns/2cycles |
| 3 | 2 | 0 | 0x80000030 (noLabel) | 0x80000018 (noLabel) | 4ns/1cycles | 12ns/2cycles |
| 4 | 3 | 2 | 3000ns/450cycles | 3100ns/465cycles | 1 | 1023 |
| 5 | 4 | 2 | 1 | 14 | 700 | 900 |

# Table HEX
| ID  | Monitor | Observer | Event/ Val a      | Event/ Val b      | ValInt Start | ValInt Stop  |
|:---:|:-------:|:--------:|:-----------------:|:-----------------:|:------------:|:------------:|
| 1 | 0 | 0 | 0x80000018 (noLabel) | 0x0 (noLabel) | 0x1 | 0x3 |
| 2 | 1 | 0 | 0x80000028 (noLabel) | 0x80000030 (noLabel) | 0x1 | 0x2 |
| 3 | 2 | 0 | 0x80000030 (noLabel) | 0x80000018 (noLabel) | 0x1 | 0x2 |
| 4 | 3 | 2 | 0x1c2 | 0x1d1 | 0x1 | 0x3ff |
| 5 | 4 | 2 | 0x1 | 0xe | 0x2bc | 0x384 |
