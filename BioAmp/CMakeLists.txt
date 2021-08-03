###########################################################################
## Authors: je_su@DESKTOP-T6C1FHS
## Description: Build information for the BioAmp source module
###########################################################################

# Use the bci2000_include macro if you need to link with frameworks from /src/extlib:
# bci2000_include( MATH )
# Set the project specific sources
BCI2000_INCLUDE( "SOURCEFILTER" )
bci2000_add_signal_source_module(
	BioAmp
	BioAmpADC.cpp
	bioAmpLib.cpp
	serialLib.cpp  
)
