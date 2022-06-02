# This is a Dockerfile to ease local documentation builds
FROM fedora:36
WORKDIR /builds
RUN \
dnf -y --refresh update && \
dnf -y groupupdate Core && \
dnf -y --allowerasing --best update && \
dnf -y install dnf-utils && \
dnf -y install \
adobe-source-code-pro-fonts \
adobe-source-sans-pro-fonts \
ansifilter \
autoconf \
autoconf-archive \
bash \
bc \
biber \
bzip2 \
ccache \
curl \
dash \
diffutils \
dos2unix \
expect \
gcc \
ghostscript \
git \
gnupg2 \
libfaketime \
libtool \
libuv-devel \
lzip \
make \
mksh \
moreutils \
nmap \
pandoc \
pbzip2 \
pdftk-java \
perl-core \
pigz \
poppler-utils \
psmisc \
python3-pygments \
qpdf \
tcsh \
telnet \
texlive-adjustbox \
texlive-babel-german \
texlive-background \
texlive-bidi \
texlive-collectbox \
texlive-collection-xetex \
texlive-csquotes \
texlive-enumitem-zref \
texlive-everypage \
texlive-filehook \
texlive-footmisc \
texlive-footnotebackref \
texlive-framed \
texlive-fvextra \
texlive-letltxmacro \
texlive-ly1 \
texlive-mdframed \
texlive-mweights \
texlive-needspace \
texlive-pagecolor \
texlive-pygmentex \
texlive-scheme-basic \
texlive-sourcecodepro \
texlive-sourcesanspro \
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
wget \
zip \
rpmconf && \
rpmconf --unattended -a && \
rpmconf --unattended -ac && \
mktexfmt xelatex.fmt 2>&1
