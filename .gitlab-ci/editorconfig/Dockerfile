################################################################################
#
# SPDX-License-Identifier: MIT-0
# scspell-id: 0eae6960-f75f-11ee-be2e-80ee73e9b8e7
#
# Copyright (c) 2018-2024 Jeffrey H. Johnson <trnsz@pobox.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the “Software”),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
# USE OR OTHER DEALINGS IN THE SOFTWARE.
#
################################################################################

FROM registry.gitlab.com/dps8m/dps8m/base:latest

USER 0

WORKDIR /

RUN set -x && \
    dnf -y --setopt=install_weak_deps=True --setopt=keepcache=True install golang \
    export PATH="${HOME:?}/go/bin:${PATH:?}" && \
    sudo -s -u user sh -xc \
      'go install github.com/editorconfig-checker/editorconfig-checker/v2/cmd/editorconfig-checker@latest' && \
    sudo -s -u user sh -xc 'go clean -modcache -testcache -cache 2>/dev/null || true' && \
    dnf -y clean all && \
    sudo -s -u user sh -xc 'rm -rf ~/.cache > /dev/null 2>&1 || true' && \
    rm -rf ~/.cache > /dev/null 2>&1 && \
    rm -rf /var/tmp/* > /dev/null 2>&1 && \
    rm -rf /var/log/dnf* > /dev/null 2>&1

USER 10000

RUN printf '\n%s\n' "BUILD SUCCESS"
