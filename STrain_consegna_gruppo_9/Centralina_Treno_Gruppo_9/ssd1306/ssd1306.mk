SSDLIB_DIR = ./ssd1306
SSDLIB_SRCS = $(SSDLIB_DIR)/ssd1306.c \
			  #$(SSDLIB_DIR)/image.c
SSDLIB_INCS = $(SSDLIB_DIR)

ALLCSRC += $(SSDLIB_SRCS)
ALLINC += $(SSDLIB_INCS)