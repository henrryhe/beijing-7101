# file: sttuner/tests/makefile
#
# makes: sat_test.lku
#        cab_test.lku
#        ter_test.lku
#        aut_test.lku
#
# history:(d/m/y)
#          8/8/2001 - Modified from 2.0.3 test makefile (bypassed 3.0.0)
#                     to support testing of STTUNER 3.1.0
#
#         22/8/2001 - Added STV0399 support
#
#         10/9/2001 - change makefile to support all services.
#
#        10/12/2003 - added options for automatic test harness for STTUNER
#
# guide:
# make all  - make all test lku's
# make ?    - make specific lku where ? is sat cab or ter
# make ?run - run specific lku where ? is sat cab or ter
# make ?log - run specific lku and create log file where ? is sat cab or ter
# log files: sat.log, cab.log or ter.log


# satellite
sattest:
	$(MAKE) -C sat sat_test.lku

satrun:
	$(MAKE) -C sat run

satlog:
	$(MAKE) -C sat runlog

satclean:
	$(MAKE) -C sat clean

# cable	
cabtest:
	$(MAKE) -C cab

cabrun:
	$(MAKE) -C cab run

cablog:
	$(MAKE) -C cab runlog

cabclean:
	$(MAKE) -C cab clean

# terrestrial	
tertest:
	$(MAKE) -C ter

terrun:
	$(MAKE) -C ter run

terlog:
	$(MAKE) -C ter runlog

terclean:
	$(MAKE) -C ter clean

# Automatic test harness
autotest:
	$(MAKE) -C auto aut_test.lku

autorun:
	$(MAKE) -C auto run

autolog:
	$(MAKE) -C auto runlog

autoclean:
	$(MAKE) -C auto clean


all:
	$(MAKE) -C sat
	$(MAKE) -C cab
	$(MAKE) -C ter

# remove all build files and logs.
clean:
	$(RM) *.lib
	$(RM) *.lku
	$(RM) *.bin
	$(RM) *.hex
	$(RM) *.tco
	$(RM) *.dbg
	$(RM) *.map
	$(RM) *.log
	$(MAKE) satclean
	$(MAKE) cabclean
	$(MAKE) terclean
	$(MAKE) autoclean

#EOF
