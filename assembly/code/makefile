EXE = exam.exe
OBJS = exam.obj
SOURCE = exam.asm
FORMAT = win64

$(EXE) :$(OBJS)
    cd Debug
    cl.exe $(OBJS) /link /LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib" libcmt.lib /ENTRY:_MAIN /SUBSYSTEM:CONSOLE

.asm.obj:
    @echo "make general:"
    nasm -f $(FORMAT) -o Debug\$*.obj $<

all:
    @echo "make all:"
    clean
    nasm -f $(FORMAT) -o Debug\$*.obj $<

clean: 
    @echo "make clean:"
    del *.obj
    del *.exe