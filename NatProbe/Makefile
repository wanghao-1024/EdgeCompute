all:nat_probe_client nat_probe_server

nat_probe_client:clean
	gcc -o nat_probe_client -g -O0 -W -Wall -Werror nat_probe_client.c log.c nat_probe.c cjson/cJSON.c -lrt -lm
nat_probe_server:clean
	gcc -o nat_probe_server -g -O0 -W -Wall -Werror nat_probe_server.c log.c nat_probe.c cjson/cJSON.c -levent -lm
clean : 
	-rm -f nat_probe_client
	-rm -f nat_probe_server
