all:
	cd manager && make && cp httpdctl ..;
	cd proxy && make && cp httpd ..;

clean:
	rm httpd httpdctl