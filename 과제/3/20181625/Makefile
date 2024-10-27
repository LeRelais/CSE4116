default:
	$(MAKE) -C ./module
	$(MAKE) -C ./app

push: default
	$(MAKE) -C ./module push
	$(MAKE) -C ./app push

clean:
	$(MAKE) -C ./module clean
	$(MAKE) -C ./app clean
