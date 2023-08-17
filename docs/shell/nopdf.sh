#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT
# scspell-id: f788dd21-3e56-11ed-ad69-80ee73e9b8e7
# Copyright (c) 2022-2023 The DPS8M Development Team

################################################################################
# Requires GNU sed.
################################################################################

sed '/<!-- start nopdf -->/,/<!-- stop nopdf -->/{//!d}' -

################################################################################
