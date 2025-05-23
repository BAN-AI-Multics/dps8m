# SPDX-License-Identifier: MIT-0
# scspell-id: e8f6be7f-f631-11ec-942a-80ee73e9b8e7
# Copyright (c) 2022-2025 The DPS8M Development Team

#############################################################################

FROM quay.io/fedora/fedora:##RELEASEVER##
WORKDIR /builds
RUN set -x; \
dnf -y --setopt=keepcache=True --setopt=install_weak_deps=True \
 --refresh update && printf %s\\n "" && \
dnf -y --setopt=keepcache=True --setopt=install_weak_deps=True \
 install dnf-utils && printf %s\\n "" && \
dnf -y --setopt=keepcache=True --setopt=install_weak_deps=True \
 update && printf %s\\n "" && \
dnf -y --setopt=keepcache=True --setopt=install_weak_deps=True \
 install \
adobe-source-code-pro-fonts \
adobe-source-sans-pro-fonts \
advancecomp \
ansifilter \
autoconf \
autoconf-archive \
bash \
bc \
biber \
bzip2 \
ccache \
cflow \
clang \
cppi \
cscope \
ctags \
curl \
dash \
diffutils \
dos2unix \
emacs \
expect \
file \
gawk \
gcc \
ghostscript \
git \
global \
gnupg2 \
graphviz \
graphviz-devil \
graphviz-gd \
graphviz-lua \
imake \
jq \
libfaketime \
libtool \
libuv-devel \
lzip \
make \
mksh \
moreutils \
ncurses-devel \
nmap \
pandoc \
pbzip2 \
pdftk-java \
perl-core \
pigz \
poppler-utils \
psmisc \
python3 \
python3-pygments \
qpdf \
rpmconf \
symlinks \
tcsh \
telnet \
texlive-adjustbox \
texlive-babel-german \
texlive-background \
texlive-bidi \
texlive-bookmark \
texlive-collectbox \
texlive-collection-xetex \
texlive-cprotect \
texlive-csquotes \
texlive-enumitem-zref \
texlive-everypage \
texlive-filehook \
texlive-footmisc \
texlive-footnotebackref \
texlive-footnotehyper \
texlive-footnoterange \
texlive-framed \
texlive-fvextra \
texlive-globalvals \
texlive-graphviz \
texlive-hardwrap \
texlive-letltxmacro \
texlive-listings \
texlive-listingsutf8 \
texlive-ly1 \
texlive-mdframed \
texlive-microtype \
texlive-minted \
texlive-mweights \
texlive-needspace \
texlive-pagecolor \
texlive-parskip \
texlive-pygmentex \
texlive-scheme-basic \
texlive-sourcecodepro \
texlive-sourcesanspro \
texlive-tcolorbox \
texlive-tikzfill \
texlive-titling \
texlive-ucharcat \
texlive-ulem \
texlive-unicode-math \
texlive-upquote \
texlive-xecjk \
texlive-xurl \
texlive-zref \
time \
tmux \
unzip \
util-linux \
vim-enhanced \
wget \
zip && printf %s\\n "" && \
rpmconf --unattended -a 2>&1 && \
rpmconf --unattended -ac 2>&1 && \
mktexlsr 2>&1 && printf %s\\n "" && \
mktexfmt xelatex.fmt 2>&1 && printf %s\\n "" && \
( symlinks -r -d /usr 2>&1 | grep dangling || true ) && \
ln -fs /usr/bin/python3 /usr/bin/python && \
printf %s\\n "" && \
dnf -y clean all && \
printf %s\\n 'Finishing ...'
