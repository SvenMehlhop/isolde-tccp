# General Information
freq=10MHz
period=100ns
# legend
## Observer
ID(ITI)=0
ID(TIP)=1
ID(CAM)=2
ID(platon_ros)=3
ID(platon_hw)=4

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
| 1 | 0 | 3 | 0x6b (noLabel) | 0 (noLabel) | 30000000ns/300000cycles | 70000000ns/700000cycles |
| 2 | 1 | 3 | 0x6b (noLabel) | 0x6a (noLabel) | 0 | 5000000ns/50000cycles |
| 3 | 1 | 3 | 0x6a (noLabel) | 0x6d (noLabel) | 0 | 3000000ns/30000cycles |
| 4 | 1 | 3 | 0x6d (noLabel) | 0x65 (noLabel) | 0 | 25000000ns/250000cycles |
| 5 | 1 | 3 | 0x6b (noLabel) | 0x6c (noLabel) | 0 | 10000000ns/100000cycles |
| 6 | 0 | 4 | 0xa (noLabel) | 0 (noLabel) | 8000000ns/80000cycles | 12000000ns/120000cycles |
| 7 | 4 | 4 | 0 | 30 | 1 | 2 |
| 8 | 4 | 4 | 2700 | 3300 | 3 | 2 |
| 9 | 1 | 3 | 0x6a (noLabel) | 0x6d (noLabel) | 0 | 200000ns/2000cycles |
| 10 | 1 | 3 | 0x6d (noLabel) | 0x65 (noLabel) | 0 | 3500000ns/35000cycles |
| 11 | 1 | 3 | 0x6b (noLabel) | 0x6c (noLabel) | 0 | 4500000ns/45000cycles |

# Table HEX
| ID  | Monitor | Observer | Event/ Val a      | Event/ Val b      | ValInt Start | ValInt Stop  |
|:---:|:-------:|:--------:|:-----------------:|:-----------------:|:------------:|:------------:|
| 1 | 0 | 3 | 0x6b (noLabel) | 0x0 (noLabel) | 0x493e0 | 0xaae60 |
| 2 | 1 | 3 | 0x6b (noLabel) | 0x6a (noLabel) | 0x0 | 0xc350 |
| 3 | 1 | 3 | 0x6a (noLabel) | 0x6d (noLabel) | 0x0 | 0x7530 |
| 4 | 1 | 3 | 0x6d (noLabel) | 0x65 (noLabel) | 0x0 | 0x3d090 |
| 5 | 1 | 3 | 0x6b (noLabel) | 0x6c (noLabel) | 0x0 | 0x186a0 |
| 6 | 0 | 4 | 0xa (noLabel) | 0x0 (noLabel) | 0x13880 | 0x1d4c0 |
| 7 | 4 | 4 | 0x0 | 0x1e | 0x1 | 0x2 |
| 8 | 4 | 4 | 0xa8c | 0xce4 | 0x3 | 0x2 |
| 9 | 1 | 3 | 0x6a (noLabel) | 0x6d (noLabel) | 0x0 | 0x7d0 |
| 10 | 1 | 3 | 0x6d (noLabel) | 0x65 (noLabel) | 0x0 | 0x88b8 |
| 11 | 1 | 3 | 0x6b (noLabel) | 0x6c (noLabel) | 0x0 | 0xafc8 |
