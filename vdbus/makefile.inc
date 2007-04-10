LNG=$(LANGUAGE)
_INX=.
STAMP=$(STAMPINF_PATH) -f $@ -a $(_BUILDARCH) -v 1.0.0.0

$(OBJ_PATH)\$(O)\$(INF_NAME).inf: $(_INX)\$(INF_NAME).inx 
    copy $(_INX)\$(@B).inx $@
    $(STAMP)

mofcomp: vdbus.bmf

vdbus.bmf: vdbus.mof
        mofcomp -B:$(OBJ_PATH)\$O\vdbus.bmf vdbus.mof
        wmimofck $(OBJ_PATH)\$O\vdbus.bmf





