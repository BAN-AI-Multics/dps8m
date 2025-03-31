################################################################################
#
# SPDX-License-Identifier: MIT
# scspell-id: d4aff604-0e6c-11f0-9e67-80ee73e9b8e7
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

FROM registry.gitlab.com/dps8m/dps8m/base:latest

USER 0

WORKDIR /

RUN set -x && \
    dnf --refresh -y install libfl2 --setopt=install_weak_deps=True && \
    dnf -y clean all && \
    rm -rf ~/.cache && \
    rm -rf /var/tmp/* && \
    rm -rf /var/log/dnf* && \
    curl -fsSL http://192.168.1.187:8088/~jhj/cache/sp/1.27/1.27.21/cross-sp-rel-1.27.21.e2k-v3.5.10_64.tgz | gzip -dc | tar xvf - && \
    curl -fsSL http://192.168.1.187:8088/~jhj/cache/sp/1.27/1.27.21/cross-sp-rel-1.27.21.e2k-v4.1c+.5.10_64.tgz | gzip -dc | tar xvf - && \
    curl -fsSL http://192.168.1.187:8088/~jhj/cache/sp/1.27/1.27.21/cross-sp-rel-1.27.21.e2k-v4.5.10_64.tgz | gzip -dc | tar xvf - && \
    curl -fsSL http://192.168.1.187:8088/~jhj/cache/sp/1.27/1.27.21/cross-sp-rel-1.27.21.e2k-v5.5.10_64.tgz | gzip -dc | tar xvf - && \
    curl -fsSL http://192.168.1.187:8088/~jhj/cache/sp/1.27/1.27.21/cross-sp-rel-1.27.21.e2k-v6.5.10-e2c3_64.tgz | gzip -dc | tar xvf - && \
    curl -fsSL http://192.168.1.187:8088/~jhj/cache/sp/1.27/1.27.21/cross-sp-rel-1.27.21.e2k-v6.5.10-e16c_64.tgz | gzip -dc | tar xvf -

RUN printf '\n%s\n' "BUILD SUCCESS"
