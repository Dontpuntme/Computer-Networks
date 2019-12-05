# Test input for program


## Using AWS Server as router, sending data back to self

On EC2 Instance:

`./overlay --router <your-real-host-ip>:<some-fake-overlay-ip>`
Example: `./overlay --router 192.168.1.71:1.0.0.0`


On Local Machine:

`./router --host <router-machine-ip> <some-fake-overlay-ip> <ttl>`
Example: `./router --host 35.170.186.28 1.0.0.0 12`

