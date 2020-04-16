# CS336 - Compression Detection Project

Malik Owens and Isaiah Jenkins

## Compression Detection Client/Server Application

### On the client system

Compile:

gcc -o compdetect_client compdetect_client.c

Run:

sudo ./compdetect_client myconfig.json

### On the server system


Compile:

gcc -o compdetect_server compdetect_server.c

Run:

sudo ./compdetect_server myconfig.json


## Compression Detection Standalone Application

#### On any system (the standalone program)

Compile:

gcc compdetect.c -o compdetect

Run:

sudo ./compdetect myconfig.json
