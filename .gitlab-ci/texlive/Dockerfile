################################################################################
#
# SPDX-License-Identifier: MIT-0
# scspell-id: 35d7d846-f79b-11ee-9c8d-80ee73e9b8e7
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
  dnf -y --setopt=install_weak_deps=True --setopt=keepcache=True install \
    adobe-source-code-pro-fonts \
    adobe-source-sans-pro-fonts \
    biber \
    ghostscript \
    graphviz \
    jq \
    lout \
    pandoc \
    pdftk-java \
    php-cli \
    poppler-utils \
    python3-pygments \
    qpdf \
    texlive-scheme-full \
&& \
  dnf -y clean all && \
  rm -rf ~/.cache && \
  rm -rf /var/tmp/* && \
  rm -rf /var/log/dnf* && \
  touch /OK

USER 10000

RUN printf '\n%s\n' "BUILD SUCCESS"
