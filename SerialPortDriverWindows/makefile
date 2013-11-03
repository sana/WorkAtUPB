!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else

DDK=C:\WinDDK\7600.16385.0

kbuild: 
	-del $(DDK)\build.dat  
	pushd . & $(DDK)\bin\setenv.bat $(DDK) chk wnet & @echo on & popd & build -cZ

!endif

clean:
	-rd /s /q objchk_wnet_x86
	-del /s /q buildchk_wnet_x86.log
	-del /s /q buildchk_wnet_x86.err
	-del /s /q buildchk_wnet_x86.wrn
