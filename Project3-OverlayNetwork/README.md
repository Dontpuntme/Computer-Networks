# Computer Networks - Project 3

Overlay Network - Connor Mclaughlin and David Martindale

## Usage
Note that this is the opposite order as indicated in the project rubric post-changes. (tldr VM IP before Overlay IP)
```
./overlay --router <vm-ip>:<overlay-ip>, <vm-ip2>:<overlay-ip2>
./overlay --host <router ip> <host ip> <ttl>
```

## Example:
```
sample router input:  ./overlay --router 1.2.3.4:10.0.0.2,5.6.7.8:10.0.0.3,9.10.11.12:10.0.0.4,13.14.15.16:10.0.0.5
sample endhost input:   ./overlay --host 35.170.186.28 1.1.1.1 2
```

Authorship document ~p3-authorship.jpeg