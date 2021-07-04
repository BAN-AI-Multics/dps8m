# DPS/8M simulator: src/Makefile
# vim: filetype=make:tabstop=4:tw=76
#
###############################################################################
#
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021 The DPS8M Development Team
#
# All rights reserved.
#
# This software is made available under the terms of the ICU
# License, version 1.8.1 or later.  For more details, see the
# LICENSE file at the top-level directory of this distribution.
#
###############################################################################

GNUMAKE?= gmake

all:
	@printf '%s\n' \
		"*** GNU Make is required; trying \"${GNUMAKE} $@\""
	@${GNUMAKE} $@

.DEFAULT:
	@printf '%s\n' \
		"*** GNU Make is required; trying \"${GNUMAKE} $@\""
	@${GNUMAKE} $@

.PHONY: all

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
