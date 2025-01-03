################################################################################
#
# SPDX-License-Identifier: MIT
# scspell-id: f9fae952-c973-11ef-a22d-80ee73e9b8e7
#
# Copyright (c) 2018-2024 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2024-2025 The DPS8M Development Team
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
################################################################################

FROM mstorsjo/llvm-mingw:latest

USER 0

RUN set -x && \
    apt -y update && \
    apt -y upgrade && \
    apt -y full-upgrade && \
    apt -y install \
        dos2unix \
        git \
        screen \
        tmux \
        zip && \
    apt -y update && \
    apt -y upgrade && \
    apt -y full-upgrade && \
    git clone https://gitlab.com/saalen/ansifilter.git && \
      cd ansifilter && \
      make && \
      make install && \
      cd .. && \
      rm -rf ansifilter && \
    ansifilter --version && \
    apt -y clean all && \
    useradd -m -U -G 0 --uid 10000 -c "CI/CD User" user

WORKDIR /

RUN printf '\n%s\n' "BUILD SUCCESS"
