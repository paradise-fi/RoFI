# OSAL files.
OSALSRC += ${CHIBIOS}/os/hal/osal/freertos/osal.c

# Required include directories
OSALINC += ${CHIBIOS}/os/hal/osal/freertos

# Shared variables
ALLCSRC += $(OSALSRC)
ALLINC  += $(OSALINC)
