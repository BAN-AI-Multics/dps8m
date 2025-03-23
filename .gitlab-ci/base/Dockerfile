################################################################################
#
# SPDX-License-Identifier: MIT
# scspell-id: f63fa7d0-c973-11ef-9e09-80ee73e9b8e7
#
# Copyright (c) 2018-2025 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2024-2024 The DPS8M Development Team
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

FROM quay.io/fedora/fedora:42

USER 0

WORKDIR /

RUN set -x && \
  dnf -y --refresh update && \
  dnf -y install dnf-plugins-core dnf-utils && \
  dnf -y install -y 'dnf-command(config-manager)' && \
  dnf -y --refresh update && \
  dnf -y install rpmconf && \
  (rpmconf --all --unattended use_maintainer || true) && \
  (rpmconf --all --unattended use_maintainer --clean || true) && \
  dnf -y --refresh update && \
  dnf -y --setopt=install_weak_deps=True --setopt=keepcache=True install \
      adobe-source-code-pro-fonts \
      adobe-source-sans-pro-fonts \
      advancecomp \
      ansifilter \
      aria2 \
      autoconf \
      autoconf-archive \
      automake \
      bash \
      bc \
      bear \
      biber \
      brotli \
      brotli-devel \
      bzip2 \
      ca-certificates \
      clang \
      clang-analyzer \
      clang-devel \
      clang-tools-extra \
      cmake \
      compat-guile18 \
      compiler-rt.i686 \
      compiler-rt.x86_64 \
      copr-cli \
      cppcheck \
      cppcheck-htmlreport \
      cppi \
      curl \
      diffutils \
      dnf-utils \
      dos2unix \
      elfutils-debuginfod \
      elfutils-debuginfod-client \
      expat \
      expat-devel \
      expect \
      fakeroot \
      file \
      flawfinder \
      g++ \
      gawk \
      gcc \
      gc-devel \
      gcovr \
      ghostscript \
      git \
      glibc-devel \
      glibc-devel.i686 \
      glibc-devel.x86_64 \
      glibc-headers-x86 \
      glibc-static \
      global \
      gnupg2 \
      golang \
      graphviz \
      graphviz-devil \
      graphviz-gd \
      graphviz-lua \
      gtk3 \
      gtk3-devel \
      guile \
      guile22-devel \
      guile-devel \
      hugo \
      hwloc-devel \
      hwloc-libs \
      imake \
      isl \
      jq \
      keyutils-libs-devel \
      krb5-devel \
      libasan \
      libasan.i686 \
      libasan.x86_64 \
      libbrotli \
      libbsd \
      libbsd-devel \
      libcom_err \
      libcurl \
      libcurl-devel \
      libevent-devel \
      libfaketime \
      libffi \
      libffi-devel \
      libgcc gmp-devel \
      libidn \
      libidn2-devel \
      libidn-devel \
      libmd \
      libmd-devel \
      libnsl \
      libnsl2 \
      libnsl2.i686 \
      libnsl2.x86_64 \
      libnsl.i686 \
      libnsl.x86_64 \
      libselinux-devel \
      libssh2-devel \
      libssh-devel \
      libtirpc \
      libtirpc.i686 \
      libtirpc.x86_64 \
      libtool \
      libtool-ltdl-devel \
      libubsan \
      libubsan.i686 \
      libubsan.x86_64 \
      libunistring \
      libuv-devel \
      libxcrypt \
      libxml2-devel \
      libxslt-devel \
      libzstd \
      libzstd-devel \
      lld \
      lld-devel \
      llvm \
      llvm-devel \
      lua \
      lua-devel \
      lzip \
      m4 \
      make \
      mingw32-binutils \
      mingw32-cpp \
      mingw32-crt \
      mingw32-dlfcn \
      mingw32-dlfcn-static \
      mingw32-filesystem \
      mingw32-filesystem mingw32-gcc \
      mingw32-gcc \
      mingw32-gcc-c++ \
      mingw32-headers \
      mingw32-nsis \
      mingw32-nsiswrapper \
      mingw32-srvany \
      mingw32-winpthreads \
      mingw32-winpthreads-static \
      mingw32-xz-libs \
      mingw32-xz-libs-static \
      mingw64-binutils \
      mingw64-cpp \
      mingw64-crt \
      mingw64-dlfcn \
      mingw64-dlfcn-static \
      mingw64-filesystem \
      mingw64-gcc \
      mingw64-gcc-c++ \
      mingw64-headers \
      mingw64-winpthreads \
      mingw64-winpthreads-static \
      mingw64-xz-libs \
      mingw64-xz-libs-static \
      mingw-binutils-generic \
      mingw-filesystem-base \
      mingw-w64-tools \
      mksh \
      moreutils \
      mpfr-devel \
      msgpack-devel \
      musl-devel \
      musl-gcc \
      musl-libc \
      musl-libc-static \
      ncurses-devel \
      nmap \
      openldap-devel \
      openssl \
      openssl-devel \
      p7zip \
      p7zip-plugins \
      pandoc \
      pcre2-devel \
      pdftk-java \
      perl \
      perl-core \
      perl-devel \
      pigz \
      pkgconf \
      poppler-utils \
      procps-ng \
      psmisc \
      python3 \
      python3-devel \
      python3-dnf \
      python3-pygments \
      python3-setuptools \
      python-setuptools \
      qpdf \
      reuse \
      sudo \
      symlinks \
      tar \
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
      ucrt64-binutils \
      ucrt64-cpp \
      ucrt64-crt \
      ucrt64-dlfcn \
      ucrt64-dlfcn-static \
      ucrt64-filesystem \
      ucrt64-gcc \
      ucrt64-gcc-c++ \
      ucrt64-headers \
      ucrt64-libgcc \
      ucrt64-libstdc++ \
      ucrt64-winpthreads \
      ucrt64-winpthreads-static \
      unifdef \
      unzip \
      util-linux \
      vim \
      wget \
      xxhash-devel \
      xz \
      xz-devel \
      xz-libs \
      zip \
      zlib \
      zlib-devel \
      zpaq \
      zstd \
&& \
  dnf -y --setopt=install_weak_deps=True --setopt=keepcache=True --best group install \
      development-tools \
      c-development \
&& \
  dnf -y --refresh update && \
  (rpmconf --all --unattended use_maintainer || true) && \
  (rpmconf --all --unattended use_maintainer --clean || true) \
&& \
  dnf -y clean all && \
  rm -rf ~/.cache && \
  rm -rf /var/tmp/* && \
  rm -rf /var/log/dnf* && \
  adduser -m -U -G wheel --uid 10000 -c "CI/CD User" user

RUN set -x && \
  go install github.com/editorconfig-checker/editorconfig-checker/v2/cmd/editorconfig-checker@latest && \
  go clean -modcache -testcache -cache 2>/dev/null || true && \
  dnf -y remove golang && \
  dnf -y clean all && \
  rm -rf ~/.cache && \
  rm -rf /var/tmp/* && \
  rm -rf /var/log/dnf*

WORKDIR /opt

RUN set -x && \
  curl -fsSL "https://github.com/johnsonjh/ndk/releases/download/r26c/android-ndk-r28.tar.lz" | \
    lzip -dc | tar xvf -

WORKDIR /

RUN set -x && \
  printf '%s\n' \
       "[viva64]" \
       "name=Viva64 - PVS-Studio - Stable" \
       "baseurl=https://cdn.pvs-studio.com/rpm" \
       "gpgcheck=0" \
       "skip_if_unavailable=True" \
       "enabled=1" > /etc/yum.repos.d/viva64.repo && \
  (dnf -y --setopt=install_weak_deps=True --setopt=keepcache=True install \
        pvs-studio) || \
    (dnf -y --setopt=install_weak_deps=True --setopt=keepcache=True install \
          "http://192.168.1.187:8088/~jhj/cache/pvs-studio.x86_64.rpm") && \
  printf '%s\n' \
       "[viva64-beta]" \
       "name=Viva64 - PVS-Studio - Beta" \
       "baseurl=http://cdn.pvs-studio.com/beta/rpm" \
       "gpgcheck=0" \
       "enabled=1" \
       "skip_if_unavailable=True" > /etc/yum.repos.d/viva64-beta.repo && \
  (dnf -y --refresh --setopt=install_weak_deps=True \
     --setopt=keepcache=True upgrade pvs-studio || true) && \
  dnf -y clean all && \
  rm -rf ~/.cache && \
  rm -rf /var/tmp/* && \
  rm -rf /var/log/dnf*

RUN set -x && \
  curl -fsSL "https://github.com/johnsonjh/osc126/releases/download/osc126/126-15.tar.gz" | \
    pigz -dc | \
    tar xvf - && \
  rm -rf /opt/oracle/developerstudio12.6/share/man && \
  rm -rf /opt/oracle/developerstudio12.6/OIC && \
  rm -rf /opt/oracle/developerstudio12.6/lib/dbxtool && \
  rm -rf /opt/oracle/developerstudio12.6/lib/devstudio && \
  rm -rf /opt/oracle/developerstudio12.6/lib/netbeans

RUN touch /.OK

RUN printf '\n%s\n' "BUILD SUCCESS"
