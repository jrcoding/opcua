#!/bin/sh
set -e -x

CURDIR="$PWD"

cat << EOF > $CURDIR/configure/CONFIG_SITE.local
UASDK = $HOME/.source/sdk
UASDK_USE_CRYPTO = YES
UASDK_USE_XMLPARSER = YES
EOF

install -d "$HOME/.source"
cd "$HOME/.source"

uasdkcc=gcc4.7.2

if [ "$CMPLR" = "gcc-6" ]
then
  uasdkcc=gcc6.3.0
  wget http://mirrors.edge.kernel.org/ubuntu/pool/main/o/openssl/libssl1.1_1.1.0g-2ubuntu4_amd64.deb
  sudo dpkg -i libssl1.1_1.1.0g-2ubuntu4_amd64.deb
fi

git clone --quiet --depth 5 https://github.com/ralphlange/opcua-client-libs.git opcua-client-libs
(cd opcua-client-libs && git log -n1 )

openssl aes-256-cbc -K $encrypted_178ee45b7f75_key -iv $encrypted_178ee45b7f75_iv \
-in opcua-client-libs/uasdk-x86_64-${uasdkcc}-${UASDK}.tar.gz.enc -out ./sdk.tar.gz -d
tar xzf ./sdk.tar.gz

if [ -e $CURDIR/configure/CONFIG_SITE.local ]
then
  cat $CURDIR/configure/CONFIG_SITE.local
fi
