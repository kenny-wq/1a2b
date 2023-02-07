#!/bin/bash
sudo mount -t efs -o tls fs-084fd736cf4eacaf0:/ efs
mkdir build
make