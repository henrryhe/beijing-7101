/*
Because STAPI makefiles suppose that a librarian can create a lib without objects. The WCE makefile fails. WCE do not create a .lib if the there is 0  OBJS. To solve the problem. We create an obj without code. This passive object will be included in all 
Library in order to make sure that the Lib.exe to create a ..a for each project.

The toolset.mak force to add the dummy.o    in the DEFINE

define BUILD_LIBRARY
$(AR) $(AROPTS) /out:$@ "$(DVD_ROOT)/dvdbr-prj-stcommon/wince_os/DummyObj/dummy.o" $(filter %.o %.lib,$^)
endef

*/