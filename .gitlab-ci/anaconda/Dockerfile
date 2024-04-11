# scspell-id: 66d20b70-f79a-11ee-9737-80ee73e9b8e7
################################################################################
#
# SPDX-License-Identifier: MIT-0
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
    cd /tmp && \
    wget -q https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh && \
    chmod a+x Miniconda3-latest-Linux-x86_64.sh && \
    export PATH="/opt/anaconda/bin:/opt/anaconda/sbin:${PATH:-}" && \
    ./Miniconda3-latest-Linux-x86_64.sh -b -t -p /opt/anaconda -t && \
    rm -f ./Miniconda3-latest-Linux-x86_64.sh && \
    conda build purge && \
    conda config --add channels "conda-forge" && \
    conda config --set channel_priority "strict" && \
    conda update --update-all --yes && \
    dnf -y clean all && \
    rm -rf ~/.cache && \
    rm -rf /var/tmp/* && \
    rm -rf /var/log/dnf* && \
    cd

USER 10000

RUN printf '\n%s\n' "BUILD SUCCESS"
