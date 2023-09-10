#Userlib directories
USERLIB = ./userlib

# List of all the Userlib files
USERSRC =  $(USERLIB)/src/rf.c  \
		   $(USERLIB)/src/defines.c
          
# Required include directories
USERINC =  $(USERLIB) \
           $(USERLIB)/include 

# Shared variables
ALLCSRC += $(USERSRC)
ALLINC  += $(USERINC)