
include ../../../Makefile.def
OBJS = Inerter.o \
	LinearElasticSpring.o \
	TwoNodeLink.o \
	TwoNodeLinkSection.o

# Compilation control

all:         $(OBJS)

# Miscellaneous
tidy:	
	@$(RM) $(RMFLAGS) Makefile.bak *~ #*# core

clean: tidy
	@$(RM) $(RMFLAGS) $(OBJS) *.o

spotless: clean

wipe: spotless

# DO NOT DELETE THIS LINE -- make depend depends on it.
