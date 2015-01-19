#./memcached -p 11211 -t 2 | grep lxp
printf "set mykey0 0 1000 6\r\nresul0\r\n" |nc localhost 11211
printf "set mykey1 0 1000 6\r\nresul0\r\n" |nc localhost 11211
printf "set mykey2 0 1000 6\r\nresul0\r\n" |nc localhost 11211
printf "set mykey3 0 1000 6\r\nresul0\r\n" |nc localhost 11211
printf "set mykey4 0 1000 6\r\nresul0\r\n" |nc localhost 11211
printf "set mykey5 0 1000 6\r\nresul0\r\n" |nc localhost 11211
printf "set mykey6 0 1000 6\r\nresul0\r\n" |nc localhost 11211
printf "set mykey7 0 1000 6\r\nresul0\r\n" |nc localhost 11211
printf "set mykey8 0 1000 6\r\nresul0\r\n" |nc localhost 11211
printf "set mykey9 0 1000 6\r\nresul0\r\n" |nc localhost 11211
