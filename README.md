# tnet

```bash
$ make dev
# cd /tnet
# ./scripts/set_up_tun_device.sh
# make
# ./bin/tnet
... new terminal, grab the container id ...
$ docker exec -it $THE_CONTAINER_ID
# ./scripts/set_up_ip.sh
# curl -I tap 192.168.2.2
```

### Questions

* Why do I need to set a default gateway to be able to curl?
* Can I send back a nonsense mac in the arp response and have the arp cache be populated?
