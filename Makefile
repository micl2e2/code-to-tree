
unameall = $(shell uname -a)
is_mingw = $(or $(findstring MINGW,$(unameall)))
is_darwin = $(or $(findstring Darwin,$(unameall)))

ifneq (,$(is_mingw))
CFLAGS += -I/usr/local/include
CFLAGS += -L/usr/local/lib
CFLAGS += -std=c23
CFLAGS += -DMCPC_C23PTCH_UCHAR2
endif
ifneq (,$(is_darwin))
CFLAGS += -std=c17
CFLAGS += -DMCPC_C23PTCH_KW1
CFLAGS += -DMCPC_C23PTCH_CKD1
CFLAGS += -DMCPC_C23PTCH_UCHAR1
endif

ifneq (,$(is_mingw))
LDFLAGS += -Wl,-Bstatic
LDFLAGS += -lmcpc
LDFLAGS += -lws2_32
LDFLAGS += -ltree-sitter
LDFLAGS += -ltree-sitter-c
LDFLAGS += -ltree-sitter-cpp
LDFLAGS += -ltree-sitter-rust
LDFLAGS += -ltree-sitter-go
LDFLAGS += -ltree-sitter-python
LDFLAGS += -ltree-sitter-ruby
LDFLAGS += -ltree-sitter-java
LDFLAGS += -Wl,-Bdynamic
endif

ifneq (,$(is_darwin))
LDFLAGS += /usr/local/lib/libmcpc.a
LDFLAGS += /usr/local/lib/libtree-sitter.a
LDFLAGS += /usr/local/lib/libtree-sitter-c.a
LDFLAGS += /usr/local/lib/libtree-sitter-cpp.a
LDFLAGS += /usr/local/lib/libtree-sitter-rust.a
LDFLAGS += /usr/local/lib/libtree-sitter-go.a
LDFLAGS += /usr/local/lib/libtree-sitter-python.a
LDFLAGS += /usr/local/lib/libtree-sitter-ruby.a
LDFLAGS += /usr/local/lib/libtree-sitter-java.a
endif

.PHONY: code-to-tree

all: code-to-tree

code-to-tree:
	$(CC) $(CFLAGS) code-to-tree.c $(LDFLAGS) -o $@

