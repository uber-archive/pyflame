SUBDIRS = src
EXTRA_DIST = autogen.sh LICENSE README.md utils/gen-gitcommit.sh src/frob.cc utils/flame-chart-json docs/pyflame.man
ACLOCAL_AMFLAGS = -I m4

man1_MANS = docs/pyflame.man

.PHONY: clean-local
clean-local:
	rm -f core.* pyflame

.PHONY: check
check:
	./runtests.sh -v
