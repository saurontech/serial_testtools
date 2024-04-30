all: mctrl openclose poll qfsize stress reconn timeout tiocmiwait rxstopstart roundtrip stress_plat

mctrl: mctrl.c
	gcc -Wall $^ -o $@
openclose: openclose.c
	gcc -Wall $^ -o $@
poll: poll.c
	gcc -Wall $^ -o $@
qfsize: qfsize.c
	gcc -Wall $^ -o $@
stress: stress.c
	gcc -Wall $^ -o $@
reconn: reconn.c
	gcc -Wall $^ -o $@
timeout: timeout.c
	gcc -Wall $^ -o $@
tiocmiwait: tiocmiwait.c
	gcc -Wall $^ -o $@

rxstopstart: rxstopstart.c
	gcc -Wall $^ -o $@

roundtrip: roundtrip.c
	gcc -Wall $^ -o $@

stress_plat: stress_plat.c
	gcc -Wall $^ -o $@ -lpthread

clean:
	rm -f mctrl
	rm -f openclose
	rm -f poll
	rm -f qfsize
	rm -f stress
	rm -f reconn
	rm -f timeout
	rm -f tiocmiwait
	rm -f rxstopstart
	rm -f roundtrip
	rm -f stress_plat
