MAKEFLAGS+=-r
SHELL:=/bin/bash
.ONESHELL:
.SECONDEXPANSION:

# This is a gcc specific makefile.
# This makefile expects to find the sources in the parent directory.
srcdir:=..
vpath %.c $(srcdir)
vpath %.h $(srcdir)

BITNESS?=64
libdir=/usr/lib
ifeq ($(BITNESS),64)
libdir=/usr/lib64
endif

lib:=libsa.so
obj:=libsa.o
test:=libsa.t.tsk
testobj:=libsa.t.o
headers:=libsa.h
dfiles:=$(obj:.o=.d)
dfiles+=$(testobj:.o=.d)
.SECONDARY: $(obj)

asan_flags:=-fsanitize=address -fsanitize=pointer-compare -fsanitize=leak\
  -fsanitize=undefined -fsanitize=pointer-subtract
all_ldflags:=-Wl,--hash-style=gnu -Wl,-rpath=. -m$(BITNESS) $(asan_flags) $(LDFLAGS)
all: $(lib)
$(lib): $(obj)
	$(CC) -shared -o $@ $(all_ldflags) $^

test: $(test)
$(test): $(testobj) $(lib)
	$(CC) -o $@ $(all_ldflags) $^

# no-omit-frame-pointer to have proper backtrace.
# no-common to let asan instrument global variables.
# The options are gcc specific.
# The expected format of the generated .d files is the one used by gcc.
all_cppflags:=-I$(srcdir) $(CPPFLAGS)
all_cflags:=-Wall -Wextra -Werror -ggdb -O0 -m$(BITNESS) -fPIC\
  -fno-omit-frame-pointer\
  -fno-common\
  $(asan_flags) $(CFLAGS)

$(obj) $(testobj): %.o: %.c %.d $$(file <%.d)
	$(CC) $(all_cppflags) $(all_cflags) -MMD -MF $*.td -o $@ -c $< || exit 1
	read obj src headers <$*.td; echo "$$headers" >$*.d || exit 1
	touch -c $@

$(dfiles):;
%.h:;

install: install-lib install-headers

install-lib: $(lib)
	rsync -t --chmod=u=rw,go=r -- $< $(libdir)

install-headers: $(headers)
	rsync -t --chmod=u=rw,go=r -- $^ /usr/include/

uninstall:
	rm -f -- $(libdir)/$(lib)
	rm -f -- $(addprefix /usr/include/, $(headers))

asanopts:=detect_stack_use_after_return=1 detect_invalid_pointer_pairs=2 abort_on_error=1 disable_coredump=0 unmap_shadow_on_exit=1
check: test
	ASAN_OPTIONS='$(asanopts)' ./$(test)

clean:
	rm -f $(lib) $(test) $(obj) $(testobj) $(dfiles) $(dfiles:.d=.td)

print-%: force
	$(info $*=$($*))

.PHONY: all test clean force check install install-lib install-headers uninstall
$(srcdir)/makefile::;
