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
