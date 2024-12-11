#!/bin/bash

iptables -t nat -A PREROUTING -p tcp -s 172.20.10.4 --dport 8000 -d 172.20.10.2 -j DNAT --to-destination 172.20.10.2:8080
