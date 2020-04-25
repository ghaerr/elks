LZSS_URL	= https://github.com/marcin-laszewski/lzss/archive/master.zip
LZSS_ZIP	= lzss-master.zip
LZSS_DIR	= lzss-master
LZSS_OUT_FILES	= lzss-c lzss-d
LZSS_OUTS	= $(addprefix $(LZSS_DIR)/,$(LZSS_OUT_FILES))
LZSS_INST	= $(addprefix $(DESTDIR)/bin/,$(LZSS_OUT_FILES))

all:	$(LZSS_OUTS)

install: $(LZSS_INST)

$(DESTDIR)/bin/%: $(LZSS_DIR)/%
	@echo '[INST]	$@'
	@cp $^ $@

$(LZSS_OUTS): $(LZSS_DIR)/makefile LZSS-FORCE
	@echo '[MAKE]	$(dir $@)	$(notdir $@)'
	@CC='$(CC)' CFLAGS='$(CFLAGS)' LDFLAGS='$(LDFLAGS)' \
		make -s --no-print-directory -C $(dir $@) $(notdir $@)


$(LZSS_DIR)/makefile: $(LZSS_ZIP)
	@echo '[UNZIP]	$@'
	$(RM) -r $(LZSS_DIR)
	@unzip -o $<
	@touch $@

$(LZSS_ZIP):
	@echo '[DOWN]	$@	$(LZSS_URL)'
	@wget -O $@ $(LZSS_URL)

distclean: clean
	$(RM) -r $(LZSS_DIR) $(LZSS_ZIP)

clean: lzss-clean

lzss-clean: LZSS-FORCE
	if ! make -s --no-print-directory -C $(LZSS_DIR) clean; then true; fi
	$(RM) $(LZSS_OUTS)

LZSS-FORCE:
