SUBDIRS=libftphack

all:
	@for x in $(SUBDIRS); do \
		cd $$x ; \
		[ -x configure -a ! -f Makefile ] && ./configure || true ; \
		$(MAKE) ; \
	done

clean:
	-@for x in $(SUBDIRS); do \
		cd $$x ; \
		$(MAKE) distclean || $(MAKE) clean ; \
	done
