#
# Makefile for blink1control-tool
#

# On Mac:
# % wget http://curl.haxx.se/download/curl-7.37.1.tar.gz
# % tar xvzf curl-7.37.1.tar.gz
# % cd curl-7.37.1
# % ./configure --prefix `pwd`/../curl-mac --disable-shared --disable-ldap --without-zlib --without-libssh2 --without-ssl --disable-crypto-auth
# % make && make install
# % cd ..
# % gcc -static -o curl-simple curl-simple.c `curl-mac/bin/curl-config --cflags` `curl-mac/bin/curl-config --static-libs`
#
# On Win:
# % wget http://curl.haxx.se/download/curl-7.37.1.tar.gz
# % tar xvzf curl-7.37.1.tar.gz
# % cd curl-7.37.1
# # In the file libcurl.pc.in add -DCURL_STATICLIB to Cflags.
# % ./configure --prefix `pwd`/../curl-win --disable-shared --disable-ldap --without-zlib --without-libssh2
# % make && make install
# % cd ..
# % gcc -static -o curl-simple curl-simple.c `curl-win2/bin/curl-config --cflags` `curl-win2/bin/curl-config --static-libs`



# try to do some autodetecting
UNAME := $(shell uname -s)

ifeq "$(UNAME)" "Darwin"
	OS=macosx
endif

ifeq "$(OS)" "Windows_NT"
	OS=windows
endif

ifeq "$(UNAME)" "Linux"
	OS=linux
endif

ifeq "$(UNAME)" "FreeBSD"
	OS=freebsd
endif

# allow overriding of GIT_TAG & BLINK1_VERSION on commandline for automated builds

MACH_TYPE:="$(strip $(shell uname -m))"
GIT_TAG?="$(strip $(shell git tag 2>&1 | tail -1 | cut -f1 -d' '))"
# deal with case of no git or no git tags, check for presence of "v" (i.e. "v1.93")
ifneq ($(findstring v,$(GIT_TAG)), v)
  GIT_TAG:="v0"
endif

BLINK1_VERSION="$(GIT_TAG)-$(OS)-$(MACH_TYPE)"

PKGOS = $(BLINK1_VERSION)

# force this?
CC=gcc

#################  Mac OS X  ##################################################
ifeq "$(OS)" "macosx"

CURL_CFLAGS = -arch x86_64 -arch arm64
CFLAGS += `curl-$(OS)/bin/curl-config --static-libs` `curl-$(OS)/bin/curl-config --cflags`
CFLAGS += $(CURL_CFLAGS)

EXE=

endif

#################  Windows  ##################################################
ifeq "$(OS)" "windows"

CFLAGS += `curl-$(OS)/bin/curl-config --static-libs` `curl-$(OS)/bin/curl-config --cflags`

EXE=.exe

endif

#################  Linux  ####################################################
ifeq "$(OS)" "linux"

CFLAGS += `curl-$(OS)/bin/curl-config --static-libs` `curl-$(OS)/bin/curl-config --cflags`


EXE=

#INSTALL = install -D
#EXELOCATION ?= /usr/local/bin
#LIBLOCATION ?= /usr/local/lib
#INCLOCATION ?= /usr/local/include

endif


#####################  Common  ###############################################

CFLAGS += -Wall
#CFLAGS += -Werror
CFLAGS += -std=gnu99
#CFLAGS += -std=c99
CFLAGS += -g

CFLAGS += -DBLINK1_VERSION=\"$(BLINK1_VERSION)\"

CFLAGS += -lm
# to fix usleep() not being found on Ubuntu14
CFLAGS += -D_BSD_SOURCE

CFLAGS += -Wno-pointer-to-int-cast
CFLAGS +=  -I json-parser
JSFILES = json-parser/json.c

CURL_DIR=curl-7.78.0

#$(OBJS): %.o: %.c
#	$(CC) $(CFLAGS) -c $< -o $@

all: msg depcheck blink1control-tool

msg:
#	@echo "Be sure to 'make curl-setup' and 'make json-parser-setup' if you have not already"
	@echo "Building blink1control-tool for OS=$(OS) BLINK1_VERSION=$(BLINK1_VERSION)"

curl-setup:
	@echo "setting up curl... $(CURL_VER)"
	tar xzf $(CURL_DIR).tar.gz
	cd $(CURL_DIR) && CFLAGS="$(CURL_CFLAGS)" ./configure --prefix="`pwd`/../curl-$(OS)" \
		--disable-shared --disable-ftp --disable-file --disable-ldap \
		--disable-ldaps --disable-rtsp --disable-telnet --disable-tftp \
		--disable-pop3 --disable-imap --disable-smb --disable-smtp \
		--disable-gopher --disable-mqtt --disable-manual \
		--disable-crypto-auth --disable-verbose \
		--without-zlib --without-brotli --without-zstd --without-librtmp \
		--without-libssh2 --without-ssl --without-quiche \
		--without-zsh-functions-dir --without-fish-functions-dir \
		&& $(MAKE) && $(MAKE) install 2>&1 || echo "curl-setup: probable non-critical error"

json-parser-setup:
	@echo "setting up json-parser"
	unzip json-parser.zip

depcheck:
	@echo "Checking if curl and json-parser are setup"
	@if [ ! -d $(CURL_DIR) ] ; then \
		$(MAKE) curl-setup; \
	else \
	  echo "  curl set up"; \
	fi

	@if [ ! -d json-parser ] ; then \
		$(MAKE) json-parser-setup; \
	else \
	  echo "  json-parser set up"; \
	fi


blink1control-tool: blink1control-tool.c
	$(CC) -o blink1control-tool$(EXE)  blink1control-tool.c $(JSFILES) $(CFLAGS)

package: blink1control-tool
	@echo "Packaging up blink1control-tool for '$(PKGOS)'"
	zip blink1control-tool-$(PKGOS).zip blink1control-tool$(EXE)
	@#mkdir -f ../builds && cp blink1control-tool-$(PKGOKS).zip ../builds

clean:
	rm -f $(OBJS)
	rm -f *.o
	rm -f blink1control-tool$(EXE)

distclean:
	$(MAKE) clean
	rm -rf $(CURL_DIR)
	rm -rf json-parser
	rm -rf curl-$(OS)

FORCE:
