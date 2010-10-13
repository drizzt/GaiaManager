SUBDIRS=libftphack

all:
	@for x in $(SUBDIRS); do \
		cd $$x ; \
		{ [ -f configure ] && ./configure ;} && $(MAKE) ; \
	done

clean:
	-@for x in $(SUBDIRS); do \
		cd $$x ; \
		$(MAKE) distclean || $(MAKE) clean ; \
	done
