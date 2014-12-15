digenv: digenv.c
	gcc -Wall -o digenv digenv.c

clean:
	rm digenv
	rm *.tst.*
test:
	./digenv > 1.tst.ex; printenv| sort |cat > 1.tst.shell; diff 1.tst.ex 1.tst.shell
	./digenv PATH > 1.tst.ex; printenv| grep PATH | sort |cat > 1.tst.shell; diff 1.tst.ex 1.tst.shell
